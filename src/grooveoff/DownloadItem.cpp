/*
    GrooveOff - Offline Grooveshark.com music
    Copyright (C) 2013-2014  Giuseppe Calà <jiveaxe@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/


#include "DownloadItem.h"
#include "widgets/TimerButton.h"
#include "Utility.h"
#include "ui_DownloadItem.h"
#include <../libgrooveshark/apirequest.h>
#include "AudioEngine.h"
#include "Playlist.h"
#include "MainWindow.h"

#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QHBoxLayout>
#include <QDir>
#include <QDebug>
#include <QDesktopServices>
#include <QGraphicsDropShadowEffect>

using namespace GrooveShark;

/*!
  \brief DownloadItem: this is the DownloadItem constructor.
  \param parent: The Parent Widget
*/
DownloadItem::DownloadItem(const PlaylistItemPtr &playlistItemPtr, QWidget *parent)
    : QWidget(parent)
    , ui_(new Ui::DownloadItem)
    , playlistItem_(playlistItemPtr)
    , oneShot_(true)
{
    ui_->setupUi(this);
    connect(playlistItem_.data(), SIGNAL(reloadCover()), this, SLOT(loadCover()));
    connect(playlistItem_.data(), SIGNAL(stateChanged(Phonon::State)), this, SLOT(setPlayerState(Phonon::State)));

    setupUi();

    setupConnections();

    if(QFile::exists(playlistItem_->path() + playlistItem_->fileName())) {
        downloadFinished(true);
    }
    else {
        downloadState_ = GrooveOff::QueuedState;
        qDebug() << "GrooveOff ::" << "Queued download of" << playlistItem_->song()->songName();
    }

    playerState_ = Phonon::StoppedState;
    stateChanged();
}

/*!
  \brief DownloadItem: this is the DownloadItem destructor.
*/
DownloadItem::~DownloadItem()
{
    if(ui_->multiFuncButton->isCountdownStarted()) {
        ui_->multiFuncButton->stopCountdown();
        removeSong();
    }

    if(downloadState_ == GrooveOff::DownloadingState) {
        downloader_->stopDownload();
    }

    removeEmptyFolder(QFileInfo(playlistItem_->path() + playlistItem_->fileName()).absoluteDir());
}

/*!
  \brief setupUi: setup widgets
  \return void
*/
void DownloadItem::setupUi()
{
    ui_->coverLabel->setScaledContents(true);
    ui_->coverLabel->setFixedSize(QSize(Utility::coverSize,Utility::coverSize));

    loadCover();

    ui_->coverLabel->setToolTip(playlistItem_->fileName());

    QGraphicsDropShadowEffect *coverShadow = new QGraphicsDropShadowEffect(this);
    coverShadow->setBlurRadius(10.0);
    coverShadow->setColor(palette().color(QPalette::Shadow));
    coverShadow->setOffset(0.0);

    ui_->coverLabel->setGraphicsEffect(coverShadow);

    ui_->titleLabel->setFont(Utility::font(QFont::Bold));
    ui_->titleLabel->setText(playlistItem_->song()->songName());
    ui_->titleLabel->setToolTip(playlistItem_->song()->songName());
    ui_->titleLabel->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred); // fix hidden label

    ui_->artist_albumLabel->setText(playlistItem_->song()->artistName() + " - " + playlistItem_->song()->albumName());
    ui_->artist_albumLabel->setToolTip(playlistItem_->song()->songName());
    ui_->artist_albumLabel->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred); // fix hidden label

    ui_->multiFuncButton->setFixedSize(QSize(Utility::buttonSize,Utility::buttonSize));

    ui_->playButton->setButtonEnabled(false);
    ui_->playButton->setPlaying(false);

    ui_->barsWidget->setFixedSize(QSize(Utility::buttonSize,Utility::buttonSize));

    ui_->progressBar->setMinimum(0);
    ui_->progressBar->setValue(0);
    ui_->progressBar->setFixedWidth(100);
    ui_->progressBar->setFixedHeight(22);

    // seems that gtk DEs use different default values than Qt...
    ui_->mainLayout->setContentsMargins(4,4,4,4);
    ui_->multiFuncLayout->setContentsMargins(0,4,0,4);
    ui_->infoIconLayout->setContentsMargins(0,4,0,4);
    ui_->infoMessageLayout->setContentsMargins(0,4,0,4);
    ui_->openFolderLayout->setContentsMargins(0,4,0,4);
    ui_->progressLayout->setContentsMargins(0,4,0,4);
    ui_->songWidgetLayout->setContentsMargins(1,1,0,2);
    ui_->songWidgetLayout->setHorizontalSpacing(5);
    ui_->infoIcon->setFixedSize(QSize(Utility::buttonSize,Utility::buttonSize));
    ui_->multiFuncButton->setIconSize(QSize(16,16));
    ui_->openFolderButton->setIconSize(QSize(16,16));
    ui_->openFolderButton->setIcon(QIcon::fromTheme(QLatin1String("folder-open")));
    ui_->openFolderButton->setToolTip(trUtf8("Open folder"));
    ui_->openFolderButton->setFixedSize(QSize(Utility::buttonSize,Utility::buttonSize));
    ui_->playButton->setFixedSize(QSize(16,16));
}

/*!
  \brief setupConnections: setup widgets connections
  \return void
*/
void DownloadItem::setupConnections()
{
    connect(ui_->playButton, SIGNAL(playButtonClicked()), this, SLOT(playSong()));
    connect(ui_->multiFuncButton, SIGNAL(clicked()), this, SLOT(multiFuncBtnClicked()));
    connect(ui_->multiFuncButton, SIGNAL(countdownFinished()), this, SLOT(removeSong()));
    connect(ui_->openFolderButton, SIGNAL(clicked()), this, SLOT(openFolder()));
}

/*!
  \brief stateChanged: handle state changes
  \return void
*/
void DownloadItem::stateChanged()
{
    emit stateChangedSignal();

    /*
     * multiFuncButton has 4 actions:
     * (A) abort current download
     * (B) remove song from queue
     * (C) delete a downloaded song
     * (D) re-download aborted
     *
     *
     * STATES and widgets' visibility
     *
     * item's widgets | multiFuncButton | progressBar | playButton | openFolderButton | info icon/message |
     * ---------------+-----------------+-------------+------------+------------------+-------------------+
     * Queue          |       (B)       |             |            |                  |                   |
     * Download       |       (A)       |      x      |      x     |                  |                   |
     * Finished       |       (C)       |             |      x     |         x        |                   |
     * Aborted        |       (D)       |             |            |                  |          x        |
     * Canceled       |                 |             |            |                  |          x        |
     *
     */


    switch(downloadState_) {
        case GrooveOff::QueuedState:
            ui_->multiFuncWidget->setVisible(false);
            ui_->multiFuncButton->setIcon(QIcon::fromTheme(QLatin1String("dialog-cancel"), QIcon(QLatin1String(":/resources/dialog-cancel.png"))));
            ui_->multiFuncButton->setToolTip(trUtf8("Remove from queue"));
            ui_->animationWidget->setVisible(false);
            ui_->barsWidget->stopAnimation();
            ui_->playWidget->setVisible(false);
            ui_->progressWidget->setVisible(false);
            ui_->openFolderWidget->setVisible(false);
            if(QIcon::hasThemeIcon(QLatin1String("download-later")))
                ui_->infoIcon->setPixmap(QIcon::fromTheme(QLatin1String("download-later")).pixmap(16,16));
            else
                ui_->infoIcon->setPixmap(QIcon::fromTheme(QLatin1String("appointment-soon"),
                                         QIcon(QLatin1String(":/resources/download-later.png"))).pixmap(16,16));
            ui_->infoIconWidget->setVisible(true);
            ui_->infoMessageWidget->setVisible(false);
            ui_->infoMessage->setText(trUtf8("Queued"));
            break;
        case GrooveOff::DownloadingState:
            ui_->multiFuncWidget->setVisible(true);
            ui_->multiFuncButton->setIcon(QIcon::fromTheme(QLatin1String("process-stop"),
                                          QIcon(QLatin1String(":/resources/process-stop.png"))));
            ui_->multiFuncButton->setToolTip(trUtf8("Stop download"));
            if(playerState_ == Phonon::StoppedState) {
                ui_->animationWidget->setVisible(false);
                ui_->barsWidget->stopAnimation();
                ui_->playWidget->setVisible(true);
            } else if(playerState_ == Phonon::PlayingState) {
                ui_->playWidget->setVisible(false);
                ui_->animationWidget->setVisible(true);
                ui_->barsWidget->startAnimation();
            } else {
                ui_->playWidget->setVisible(false);
                ui_->animationWidget->setVisible(true);
                ui_->barsWidget->stopAnimation();
            }
            ui_->progressWidget->setVisible(true);
            ui_->infoIconWidget->setVisible(false);
            ui_->infoMessageWidget->setVisible(false);
            ui_->openFolderWidget->setVisible(false);
            break;
        case GrooveOff::FinishedState:
            ui_->playButton->setButtonEnabled(true);
            ui_->multiFuncWidget->setVisible(false);
            ui_->multiFuncButton->setIcon(QIcon::fromTheme(QLatin1String("user-trash"),
                                          QIcon(QLatin1String(":/resources/user-trash.png"))));
            ui_->multiFuncButton->setToolTip(trUtf8("Delete this song"));
            if(playerState_ == Phonon::StoppedState) {
                ui_->playWidget->setVisible(true);
                ui_->animationWidget->setVisible(false);
                ui_->barsWidget->stopAnimation();
            } else if(playerState_ == Phonon::PlayingState) {
                ui_->playWidget->setVisible(false);
                ui_->animationWidget->setVisible(true);
                ui_->barsWidget->startAnimation();
            } else {
                ui_->playWidget->setVisible(false);
                ui_->animationWidget->setVisible(true);
                ui_->barsWidget->stopAnimation();
            }
            ui_->progressWidget->setVisible(false);
            ui_->infoIconWidget->setVisible(false);
            ui_->infoMessageWidget->setVisible(false);
            ui_->openFolderWidget->setVisible(false);
            break;
        case GrooveOff::AbortedState:
            ui_->playWidget->setVisible(false);
            ui_->animationWidget->setVisible(false);
            ui_->barsWidget->stopAnimation();
            ui_->progressWidget->setVisible(false);
            if(QIcon::hasThemeIcon(QLatin1String("task-reject")))
                ui_->infoIcon->setPixmap(QIcon::fromTheme(QLatin1String("task-reject")).pixmap(16,16));
            else
                ui_->infoIcon->setPixmap(QIcon::fromTheme(QLatin1String("emblem-unreadable"),
                                         QIcon(QLatin1String(":/resources/task-reject.png"))).pixmap(16,16));
            ui_->infoIconWidget->setVisible(true);
            ui_->infoMessageWidget->setVisible(false);
            ui_->infoMessage->setText(trUtf8("Aborted"));
            ui_->multiFuncWidget->setVisible(false);
            ui_->multiFuncButton->setIcon(QIcon::fromTheme(QLatin1String("view-refresh"),
                                          QIcon(QLatin1String(":/resources/view-refresh.png"))));
            ui_->multiFuncButton->setToolTip(trUtf8("Redownload selected"));
            ui_->openFolderWidget->setVisible(false);
            break;
        case GrooveOff::DeletedState:
            ui_->multiFuncWidget->setVisible(false);
            ui_->playWidget->setVisible(false);
            ui_->animationWidget->setVisible(false);
            ui_->barsWidget->stopAnimation();
            ui_->progressWidget->setVisible(false);
            ui_->infoIcon->setPixmap(QIcon::fromTheme(QLatin1String("user-trash-full"),
                                     QIcon(QLatin1String(":/resources/user-trash-full.png"))).pixmap(16,16));
            ui_->infoIconWidget->setVisible(true);
            ui_->infoMessageWidget->setVisible(false);
            ui_->infoMessage->setText(trUtf8("Deleted"));
            ui_->openFolderWidget->setVisible(false);
            emit reloadPlaylist();
            break;
        default:
            ui_->playWidget->setVisible(false);
            ui_->animationWidget->setVisible(false);
            ui_->barsWidget->stopAnimation();
            ui_->progressWidget->setVisible(false);
            ui_->infoIcon->setPixmap(QIcon::fromTheme(QLatin1String("dialog-warning"),
                                     QIcon(QLatin1String(":/resources/dialog-warning.png"))).pixmap(16,16));
            ui_->infoIconWidget->setVisible(true);
            ui_->infoMessageWidget->setVisible(false);
            ui_->infoMessage->setText(trUtf8("Network or Server error"));
            ui_->multiFuncWidget->setVisible(false);
            ui_->multiFuncButton->setIcon(QIcon::fromTheme(QLatin1String("view-refresh"),
                                          QIcon(QLatin1String(":/resources/view-refresh.png"))));
            ui_->multiFuncButton->setToolTip(trUtf8("Redownload selected"));
            ui_->openFolderWidget->setVisible(false);
            break;
    }
}

/*!
  \brief startDownload: start song download
  \return void
*/
void DownloadItem::startDownload()
{
    oneShot_ = true;
    downloadState_ = GrooveOff::DownloadingState;
    stateChanged();
    downloader_ = ApiRequest::instance()->downloadSong(playlistItem_->path(),
                                                       playlistItem_->fileName(),
                                                       playlistItem_->song()->songID(),
                                                       Utility::token);

    connect(downloader_.data(), SIGNAL(progress(qint64,qint64)),
            this, SLOT(setProgress(qint64,qint64)));

    connect(downloader_.data(), SIGNAL(downloadCompleted(bool)),
            this, SLOT(downloadFinished(bool)));

    qDebug() << "GrooveOff ::" << "Started download of" << playlistItem_->song()->songName();
}

/*!
  \brief downloadFinished: slot called once cover download finished
  \param ok: status of cover download
  \return void
*/
void DownloadItem::downloadFinished(bool ok)
{
    //...and show others if download was successful
    if(ok) {
        downloadState_ = GrooveOff::FinishedState;
        qDebug() << "GrooveOff ::" << "Finished download of" << playlistItem_->song()->songName();
        emit reloadPlaylist();
    } else {
        removeEmptyFolder(QFileInfo(playlistItem_->path() + playlistItem_->fileName()).absoluteDir());
        downloadState_ = GrooveOff::ErrorState;
        qDebug() << "GrooveOff :: Error downloading" << playlistItem_->song()->songName() << "::" << downloader_->errorString();
    }

    stateChanged();

    emit downloadFinished();
    The::mainWindow()->reloadItemsDownloadButtons();
}

/*!
  \brief setProgress: update progressbar
  \param bytesReceived: number of bytes received
  \param bytesTotal: number of total bytes
  \return void
*/
void DownloadItem::setProgress(const qint64 &bytesReceived, const qint64 &bytesTotal)
{
    if(oneShot_) {
        The::mainWindow()->reloadItemsDownloadButtons();
        oneShot_ = false;
    }
    // enable play button if downloaded at least 1MiB
    if(bytesReceived > 1024*1024)
        ui_->playButton->setButtonEnabled(true);

    ui_->progressBar->setValue(bytesReceived);
    ui_->progressBar->setMaximum(bytesTotal);
}

/*!
  \brief playSong: play song
  \return void
*/
void DownloadItem::playSong()
{
    The::audioEngine()->playItem(playlistItem_);
}

/*!
  \brief startRemoving: prepare to remove a song
  \return void
*/
void DownloadItem::multiFuncBtnClicked()
{
    switch(downloadState_) {
        case GrooveOff::FinishedState:
            //check if file exists before starting countdown
            if(!QFile::exists(songFile())) {
                qDebug() << "GrooveOff ::"  << songFile() << "not found";
                downloadState_ = GrooveOff::DeletedState;
                stateChanged();
                return;
            }

            if(!ui_->multiFuncButton->isCountdownStarted()) {
                ui_->multiFuncButton->setToolTip(trUtf8("Abort deletion"));
                ui_->multiFuncButton->startCountdown();
            } else {
                ui_->multiFuncButton->setToolTip(trUtf8("Delete this song"));
                ui_->multiFuncButton->stopCountdown();
                ui_->multiFuncButton->setIcon(QIcon::fromTheme(QLatin1String("user-trash"),
                                              QIcon(QLatin1String(":/resources/user-trash.png"))));
            }
            break;
        case GrooveOff::AbortedState:
        case GrooveOff::ErrorState:
            downloadState_ = GrooveOff::QueuedState;
            stateChanged();
            emit addToQueue(this);
            break;
        case GrooveOff::DownloadingState:
            downloader_->stopDownload();
            ui_->progressBar->setValue(0);
            downloadState_ = GrooveOff::AbortedState;
            stateChanged();
            break;
        case GrooveOff::QueuedState:
            downloadState_ = GrooveOff::AbortedState;
            stateChanged();
            break;
        default:
            // do nothing
            break;
    }
}

bool DownloadItem::operator==(DownloadItem& right) const
{
    if(playlistItem_->song()->songID() == right.playlistItem()->song()->songID())
        return true;
    return false;
}


/*!
  \brief removeSong: remove a song
  \return void
*/
void DownloadItem::removeSong()
{
    The::audioEngine()->removingTrack(playlistItem_);
    The::playlist()->removeItem(playlistItem_);

    if(QFile::exists(songFile())) {
        if(!QFile::remove(songFile()))
            qDebug() << "GrooveOff ::"  << "Error removing" << songFile();
    } else {
        qDebug() << "GrooveOff ::"  << songFile() << "not found";
    }

    removeEmptyFolder(QFileInfo(playlistItem_->path() + playlistItem_->fileName()).absoluteDir());

    downloadState_ = GrooveOff::DeletedState;
    qDebug() << "GrooveOff ::" << songFile() << "removed";
    stateChanged();
    The::mainWindow()->reloadItemsDownloadButtons();
}

/*!
  \brief songFile: return absolute song file name
  \return string
*/
QString DownloadItem::songFile()
{
    return playlistItem_->path() + playlistItem_->fileName();
}

/*!
  \brief enterEvent: manage mouse movement
  \return void
*/
void DownloadItem::enterEvent(QEvent* event)
{
    switch(downloadState_) {
        case GrooveOff::FinishedState:
            ui_->multiFuncWidget->setVisible(true);
            ui_->openFolderWidget->setVisible(true);
            break;
        case GrooveOff::DeletedState:
            ui_->infoMessageWidget->setVisible(true);
            break;
        case GrooveOff::ErrorState:
        case GrooveOff::AbortedState:
        case GrooveOff::QueuedState:
            ui_->infoMessageWidget->setVisible(true);
            ui_->multiFuncWidget->setVisible(true);
            break;
        default:
            // do nothing
            break;
    }

    QWidget::enterEvent(event);
}

/*!
  \brief leaveEvent: manage mouse movement
  \return void
*/
void DownloadItem::leaveEvent(QEvent* event)
{
    switch(downloadState_) {
        case GrooveOff::FinishedState:
            if(!ui_->multiFuncButton->isCountdownStarted())
                ui_->multiFuncWidget->setVisible(false);
            ui_->openFolderWidget->setVisible(false);
            break;
        case GrooveOff::DeletedState:
            ui_->infoMessageWidget->setVisible(false);
            break;
        case GrooveOff::ErrorState:
        case GrooveOff::AbortedState:
        case GrooveOff::QueuedState:
            ui_->infoMessageWidget->setVisible(false);
            ui_->multiFuncWidget->setVisible(false);
            break;
        default:
            // do nothing
            break;
    }

    QWidget::leaveEvent(event);
}

/*!
  \brief setPlayerState: this function sets the state of the player
         for icon syncronization
  \return void
*/
void DownloadItem::setPlayerState(Phonon::State state)
{
    playerState_ = state;
    stateChanged();
}

/*!
  \brief openFolder: open folder containing downloaded song
  \return void
*/
void DownloadItem::openFolder()
{
    QFileInfo fileInfo(playlistItem_->path() + playlistItem_->fileName());
    QDesktopServices::openUrl(QUrl("file://" + fileInfo.absolutePath(), QUrl::TolerantMode));
}

void DownloadItem::loadCover()
{
    if(!playlistItem_->song()->coverArtFilename().isEmpty()
        && QFile::exists(Utility::coversCachePath + playlistItem_->song()->coverArtFilename())
        && playlistItem_->song()->coverArtFilename() != "0")
        ui_->coverLabel->setPixmap(QPixmap(Utility::coversCachePath + playlistItem_->song()->coverArtFilename()));
    else
        ui_->coverLabel->setPixmap(QIcon::fromTheme(QLatin1String("media-optical"),
                                   QIcon(QLatin1String(":/resources/media-optical.png"))).pixmap(Utility::coverSize));
}

void DownloadItem::abortDownload()
{
    if(downloadState_ == GrooveOff::DownloadingState) {
        downloader_->stopDownload();
        ui_->progressBar->setValue(0);
    }

    downloadState_ = GrooveOff::AbortedState;
    stateChanged();

    // FIXME: seems that file remove is slower than this call
    // and the icon is updated as expected
    The::mainWindow()->reloadItemsDownloadButtons();

    removeEmptyFolder(QFileInfo(playlistItem_->path() + playlistItem_->fileName()).absoluteDir());
}

void DownloadItem::removeEmptyFolder(QDir folder)
{
    if(!folder.exists())
        return;

    if(folder.entryList(QDir::NoDotAndDotDot).count() == 0) {
        folder.rmdir(folder.path());
        if(folder.cdUp())
            removeEmptyFolder(folder);
    }
}

