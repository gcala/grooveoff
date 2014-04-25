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
DownloadItem::DownloadItem(const PlaylistItemPtr &playlistItemPtr, QWidget *parent, GrooveOff::Context context)
    : QWidget(parent)
    , ui(new Ui::DownloadItem)
    , m_playlistItem(playlistItemPtr)
    , m_oneShot(true)
    , m_context(context)
{
    ui->setupUi(this);
    connect(m_playlistItem.data(), SIGNAL(reloadCover()), this, SLOT(loadCover()));
    connect(m_playlistItem.data(), SIGNAL(stateChanged(Phonon::State)), this, SLOT(setPlayerState(Phonon::State)));

    setupUi();

    setupConnections();

    if(m_context == GrooveOff::Track) {
        m_downloadState = GrooveOff::FinishedState;
    } else {
        if(QFile::exists(m_playlistItem->path() + m_playlistItem->fileName())) {
            downloadFinished(true);
        }
        else {
            m_downloadState = GrooveOff::QueuedState;
            // qDebug() << "GrooveOff ::" << "Queued download of" << m_playlistItem->song()->songName();
        }
    }

    m_playerState = Phonon::StoppedState;
    stateChanged();
}

/*!
  \brief DownloadItem: this is the DownloadItem destructor.
*/
DownloadItem::~DownloadItem()
{
    if(ui->multiFuncButton->isCountdownStarted()) {
        ui->multiFuncButton->stopCountdown();
        if(m_context == GrooveOff::Track)
            emit removeMeFromSession(m_playlistItem->song()->songID());
        else
            removeSong();
    }

    if(m_downloadState == GrooveOff::DownloadingState) {
        m_downloader->stopDownload();
    }

    if(m_context == GrooveOff::Download)
        removeEmptyFolder(QFileInfo(m_playlistItem->path() + m_playlistItem->fileName()).absoluteDir());
}

/*!
  \brief setupUi: setup widgets
  \return void
*/
void DownloadItem::setupUi()
{
    ui->coverLabel->setScaledContents(true);
    ui->coverLabel->setFixedSize(QSize(Utility::coverSize,Utility::coverSize));
    loadCover();

    ui->coverLabel->setToolTip(m_playlistItem->fileName());

    QGraphicsDropShadowEffect *coverShadow = new QGraphicsDropShadowEffect(this);
    coverShadow->setBlurRadius(10.0);
    coverShadow->setColor(palette().color(QPalette::Shadow));
    coverShadow->setOffset(0.0);

    ui->coverLabel->setGraphicsEffect(coverShadow);

    ui->titleLabel->setFont(Utility::font(QFont::Bold));
    ui->titleLabel->setText(m_playlistItem->song()->songName());
    ui->titleLabel->setToolTip(m_playlistItem->song()->songName());
    ui->titleLabel->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred); // fix hidden label

    ui->artist_albumLabel->setText(m_playlistItem->song()->artistName() + " - " + m_playlistItem->song()->albumName());
    ui->artist_albumLabel->setToolTip(m_playlistItem->song()->songName());
    ui->artist_albumLabel->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred); // fix hidden label

    ui->multiFuncButton->setFixedSize(QSize(Utility::buttonSize,Utility::buttonSize));

    if(m_context == GrooveOff::Track) {
        ui->playWidget->setVisible(false);
        ui->progressBar->setVisible(false);
    } else {
        ui->playButton->setButtonEnabled(false);
        ui->playButton->setPlaying(false);

        ui->barsWidget->setFixedSize(QSize(Utility::buttonSize,Utility::buttonSize));

        ui->progressBar->setMinimum(0);
        ui->progressBar->setValue(0);
        ui->progressBar->setFixedWidth(100);
        ui->progressBar->setFixedHeight(22);
    }

    // seems that gtk DEs use different default values than Qt...
    ui->mainLayout->setContentsMargins(4,4,4,4);
    ui->multiFuncLayout->setContentsMargins(0,4,0,4);
    ui->infoIconLayout->setContentsMargins(0,4,0,4);
    ui->infoMessageLayout->setContentsMargins(0,4,0,4);
    ui->openFolderLayout->setContentsMargins(0,4,0,4);
    ui->progressLayout->setContentsMargins(0,4,0,4);
    ui->songWidgetLayout->setContentsMargins(1,1,0,2);
    ui->songWidgetLayout->setHorizontalSpacing(5);
    ui->infoIcon->setFixedSize(QSize(Utility::buttonSize,Utility::buttonSize));
    ui->multiFuncButton->setIconSize(QSize(16,16));
    ui->openFolderButton->setIconSize(QSize(16,16));
    ui->openFolderButton->setIcon(QIcon::fromTheme(QLatin1String("folder-open")));
    ui->openFolderButton->setToolTip(trUtf8("Open folder"));
    ui->openFolderButton->setFixedSize(QSize(Utility::buttonSize,Utility::buttonSize));
    ui->playButton->setFixedSize(QSize(16,16));
}

/*!
  \brief setupConnections: setup widgets connections
  \return void
*/
void DownloadItem::setupConnections()
{
    if(m_context == GrooveOff::Download) {
        connect(ui->playButton, SIGNAL(playButtonClicked()), this, SLOT(playSong()));
        connect(ui->openFolderButton, SIGNAL(clicked()), this, SLOT(openFolder()));
    }

    connect(ui->multiFuncButton, SIGNAL(clicked()), this, SLOT(multiFuncBtnClicked()));
    connect(ui->multiFuncButton, SIGNAL(countdownFinished()), this, SLOT(removeSong()));
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


    switch(m_downloadState) {
        case GrooveOff::QueuedState:
            ui->multiFuncWidget->setVisible(false);
            ui->multiFuncButton->setIcon(QIcon::fromTheme(QLatin1String("dialog-cancel"), QIcon(QLatin1String(":/resources/dialog-cancel.png"))));
            ui->multiFuncButton->setToolTip(trUtf8("Remove from queue"));
            ui->animationWidget->setVisible(false);
            ui->barsWidget->stopAnimation();
            ui->playWidget->setVisible(false);
            ui->progressWidget->setVisible(false);
            ui->openFolderWidget->setVisible(false);
            if(QIcon::hasThemeIcon(QLatin1String("download-later")))
                ui->infoIcon->setPixmap(QIcon::fromTheme(QLatin1String("download-later")).pixmap(16,16));
            else
                ui->infoIcon->setPixmap(QIcon::fromTheme(QLatin1String("appointment-soon"),
                                         QIcon(QLatin1String(":/resources/download-later.png"))).pixmap(16,16));
            ui->infoIconWidget->setVisible(true);
            ui->infoMessageWidget->setVisible(false);
            ui->infoMessage->setText(trUtf8("Queued"));
            break;
        case GrooveOff::DownloadingState:
            ui->multiFuncWidget->setVisible(true);
            ui->multiFuncButton->setIcon(QIcon::fromTheme(QLatin1String("process-stop"),
                                          QIcon(QLatin1String(":/resources/process-stop.png"))));
            ui->multiFuncButton->setToolTip(trUtf8("Stop download"));
            if(m_playerState == Phonon::StoppedState) {
                ui->animationWidget->setVisible(false);
                ui->barsWidget->stopAnimation();
                ui->playWidget->setVisible(true);
            } else if(m_playerState == Phonon::PlayingState) {
                ui->playWidget->setVisible(false);
                ui->animationWidget->setVisible(true);
                ui->barsWidget->startAnimation();
            } else {
                ui->playWidget->setVisible(false);
                ui->animationWidget->setVisible(true);
                ui->barsWidget->stopAnimation();
            }
            ui->progressWidget->setVisible(true);
            ui->infoIconWidget->setVisible(false);
            ui->infoMessageWidget->setVisible(false);
            ui->openFolderWidget->setVisible(false);
            break;
        case GrooveOff::FinishedState:
            if(m_context == GrooveOff::Track) {
                ui->playWidget->setVisible(false);
                ui->animationWidget->setVisible(false);
            } else {
                ui->playButton->setButtonEnabled(true);
                if(m_playerState == Phonon::StoppedState) {
                    ui->playWidget->setVisible(true);
                    ui->animationWidget->setVisible(false);
                    ui->barsWidget->stopAnimation();
                } else if(m_playerState == Phonon::PlayingState) {
                    ui->playWidget->setVisible(false);
                    ui->animationWidget->setVisible(true);
                    ui->barsWidget->startAnimation();
                } else {
                    ui->playWidget->setVisible(false);
                    ui->animationWidget->setVisible(true);
                    ui->barsWidget->stopAnimation();
                }
            }
            ui->multiFuncWidget->setVisible(false);
            ui->multiFuncButton->setIcon(QIcon::fromTheme(QLatin1String("user-trash"),
                                          QIcon(QLatin1String(":/resources/user-trash.png"))));
            if(m_context == GrooveOff::Track)
                ui->multiFuncButton->setToolTip(trUtf8("Delete from Session"));
            else
                ui->multiFuncButton->setToolTip(trUtf8("Delete song"));
            ui->progressWidget->setVisible(false);
            ui->infoIconWidget->setVisible(false);
            ui->infoMessageWidget->setVisible(false);
            ui->openFolderWidget->setVisible(false);
            break;
        case GrooveOff::AbortedState:
            ui->playWidget->setVisible(false);
            ui->animationWidget->setVisible(false);
            ui->barsWidget->stopAnimation();
            ui->progressWidget->setVisible(false);
            if(QIcon::hasThemeIcon(QLatin1String("task-reject")))
                ui->infoIcon->setPixmap(QIcon::fromTheme(QLatin1String("task-reject")).pixmap(16,16));
            else
                ui->infoIcon->setPixmap(QIcon::fromTheme(QLatin1String("emblem-unreadable"),
                                         QIcon(QLatin1String(":/resources/task-reject.png"))).pixmap(16,16));
            ui->infoIconWidget->setVisible(true);
            ui->infoMessageWidget->setVisible(false);
            ui->infoMessage->setText(trUtf8("Aborted"));
            ui->multiFuncWidget->setVisible(false);
            ui->multiFuncButton->setIcon(QIcon::fromTheme(QLatin1String("view-refresh"),
                                          QIcon(QLatin1String(":/resources/view-refresh.png"))));
            ui->multiFuncButton->setToolTip(trUtf8("Redownload selected"));
            ui->openFolderWidget->setVisible(false);
            break;
        case GrooveOff::DeletedState:
            ui->multiFuncWidget->setVisible(false);
            ui->playWidget->setVisible(false);
            ui->animationWidget->setVisible(false);
            ui->barsWidget->stopAnimation();
            ui->progressWidget->setVisible(false);
            ui->infoIcon->setPixmap(QIcon::fromTheme(QLatin1String("user-trash-full"),
                                     QIcon(QLatin1String(":/resources/user-trash-full.png"))).pixmap(16,16));
            ui->infoIconWidget->setVisible(true);
            ui->infoMessageWidget->setVisible(false);
            ui->infoMessage->setText(trUtf8("Deleted"));
            ui->openFolderWidget->setVisible(false);
            emit reloadPlaylist();
            break;
        default:
            ui->playWidget->setVisible(false);
            ui->animationWidget->setVisible(false);
            ui->barsWidget->stopAnimation();
            ui->progressWidget->setVisible(false);
            ui->infoIcon->setPixmap(QIcon::fromTheme(QLatin1String("dialog-warning"),
                                     QIcon(QLatin1String(":/resources/dialog-warning.png"))).pixmap(16,16));
            ui->infoIconWidget->setVisible(true);
            ui->infoMessageWidget->setVisible(false);
            ui->infoMessage->setText(trUtf8("Network or Server error"));
            ui->multiFuncWidget->setVisible(false);
            ui->multiFuncButton->setIcon(QIcon::fromTheme(QLatin1String("view-refresh"),
                                          QIcon(QLatin1String(":/resources/view-refresh.png"))));
            ui->multiFuncButton->setToolTip(trUtf8("Redownload selected"));
            ui->openFolderWidget->setVisible(false);
            break;
    }
    
    update();
}

/*!
  \brief startDownload: start song download
  \return void
*/
void DownloadItem::startDownload()
{
    m_oneShot = true;
    m_downloadState = GrooveOff::DownloadingState;
    stateChanged();
    m_downloader = ApiRequest::instance()->downloadSong(m_playlistItem->path(),
                                                       m_playlistItem->fileName(),
                                                       m_playlistItem->song()->songID(),
                                                       Utility::token);

    connect(m_downloader.data(), SIGNAL(progress(qint64,qint64)),
            this, SLOT(setProgress(qint64,qint64)));

    connect(m_downloader.data(), SIGNAL(downloadCompleted(bool)),
            this, SLOT(downloadFinished(bool)));

//    qDebug() << "GrooveOff ::" << "Started download of" << m_playlistItem->song()->songName();
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
        m_downloadState = GrooveOff::FinishedState;
//        qDebug() << "GrooveOff ::" << "Finished download of" << m_playlistItem->song()->songName();
        emit reloadPlaylist();
    } else {
        removeEmptyFolder(QFileInfo(m_playlistItem->path() + m_playlistItem->fileName()).absoluteDir());
        m_downloadState = GrooveOff::ErrorState;
        qDebug() << "GrooveOff :: Error downloading" << m_playlistItem->song()->songName() << "::" << m_downloader->errorString();
    }

    stateChanged();

    emit downloadFinished();
    if( The::mainWindow() )
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
    if(m_oneShot) {
        if( The::mainWindow() )
            The::mainWindow()->reloadItemsDownloadButtons();
        m_oneShot = false;
    }
    // enable play button if downloaded at least 1MiB
    if(bytesReceived > 1024*1024)
        ui->playButton->setButtonEnabled(true);

    ui->progressBar->setValue(bytesReceived);
    ui->progressBar->setMaximum(bytesTotal);
}

/*!
  \brief playSong: play song
  \return void
*/
void DownloadItem::playSong()
{
    The::audioEngine()->playItem(m_playlistItem);
}

/*!
  \brief startRemoving: prepare to remove a song
  \return void
*/
void DownloadItem::multiFuncBtnClicked()
{
    switch(m_downloadState) {
        case GrooveOff::FinishedState:
            if(m_context == GrooveOff::Download) {
                //check if file exists before starting countdown
                if(!QFile::exists(songFile())) {
                    qDebug() << "GrooveOff ::"  << songFile() << "not found";
                    m_downloadState = GrooveOff::DeletedState;
                    stateChanged();
                    return;
                }
            }

            if(!ui->multiFuncButton->isCountdownStarted()) {
                ui->multiFuncButton->setToolTip(trUtf8("Abort deletion"));
                ui->multiFuncButton->startCountdown();
            } else {
                ui->multiFuncButton->setToolTip(trUtf8("Delete this song"));
                ui->multiFuncButton->stopCountdown();
                ui->multiFuncButton->setIcon(QIcon::fromTheme(QLatin1String("user-trash"),
                                              QIcon(QLatin1String(":/resources/user-trash.png"))));
            }
            break;
        case GrooveOff::AbortedState:
        case GrooveOff::ErrorState:
            m_downloadState = GrooveOff::QueuedState;
            stateChanged();
            emit addToQueue(this);
            break;
        case GrooveOff::DownloadingState:
            m_downloader->stopDownload();
            ui->progressBar->setValue(0);
            m_downloadState = GrooveOff::AbortedState;
            stateChanged();
            break;
        case GrooveOff::QueuedState:
            m_downloadState = GrooveOff::AbortedState;
            stateChanged();
            break;
        default:
            // do nothing
            break;
    }
}

bool DownloadItem::operator==(DownloadItem& right) const
{
    if(m_playlistItem->song()->songID() == right.playlistItem()->song()->songID())
        return true;
    return false;
}


/*!
  \brief removeSong: remove a song
  \return void
*/
void DownloadItem::removeSong()
{
    if(m_context == GrooveOff::Track) {
        emit removeMeFromSession(m_playlistItem->song()->songID());
        return;
    }

    The::audioEngine()->removingTrack(m_playlistItem);
    The::playlist()->removeItem(m_playlistItem);

    if(QFile::exists(songFile())) {
        if(!QFile::remove(songFile()))
            qDebug() << "GrooveOff ::"  << "Error removing" << songFile();
    } else {
        qDebug() << "GrooveOff ::"  << songFile() << "not found";
    }

    removeEmptyFolder(QFileInfo(m_playlistItem->path() + m_playlistItem->fileName()).absoluteDir());

    m_downloadState = GrooveOff::DeletedState;
//    qDebug() << "GrooveOff ::" << songFile() << "removed";
    stateChanged();
    if( The::mainWindow() )
        The::mainWindow()->reloadItemsDownloadButtons();
}

/*!
  \brief songFile: return absolute song file name
  \return string
*/
QString DownloadItem::songFile()
{
    return m_playlistItem->path() + m_playlistItem->fileName();
}

/*!
  \brief enterEvent: manage mouse movement
  \return void
*/
void DownloadItem::enterEvent(QEvent* event)
{
    switch(m_downloadState) {
        case GrooveOff::FinishedState:
            ui->multiFuncWidget->setVisible(true);
            if(m_context == GrooveOff::Download)
                ui->openFolderWidget->setVisible(true);
            break;
        case GrooveOff::DeletedState:
            ui->infoMessageWidget->setVisible(true);
            break;
        case GrooveOff::ErrorState:
        case GrooveOff::AbortedState:
        case GrooveOff::QueuedState:
            ui->infoMessageWidget->setVisible(true);
            if(!Utility::token.isEmpty())
                ui->multiFuncWidget->setVisible(true);
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
    switch(m_downloadState) {
        case GrooveOff::FinishedState:
            if(!ui->multiFuncButton->isCountdownStarted())
                ui->multiFuncWidget->setVisible(false);
            ui->openFolderWidget->setVisible(false);
            break;
        case GrooveOff::DeletedState:
            ui->infoMessageWidget->setVisible(false);
            break;
        case GrooveOff::ErrorState:
        case GrooveOff::AbortedState:
        case GrooveOff::QueuedState:
            ui->infoMessageWidget->setVisible(false);
            ui->multiFuncWidget->setVisible(false);
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
    m_playerState = state;
    stateChanged();
}

/*!
  \brief openFolder: open folder containing downloaded song
  \return void
*/
void DownloadItem::openFolder()
{
    QFileInfo fileInfo(m_playlistItem->path() + m_playlistItem->fileName());
    QDesktopServices::openUrl(QUrl("file://" + fileInfo.absolutePath(), QUrl::TolerantMode));
}

void DownloadItem::loadCover()
{
    if(!m_playlistItem->song()->coverArtFilename().isEmpty()
        && QFile::exists(Utility::coversCachePath + m_playlistItem->song()->coverArtFilename())
        && m_playlistItem->song()->coverArtFilename() != "0")
        ui->coverLabel->setPixmap(QPixmap(Utility::coversCachePath + m_playlistItem->song()->coverArtFilename()));
    else
        ui->coverLabel->setPixmap(QIcon::fromTheme(QLatin1String("media-optical"),
                                   QIcon(QLatin1String(":/resources/media-optical.png"))).pixmap(Utility::coverSize));
}

void DownloadItem::abortDownload()
{
    if(m_downloadState == GrooveOff::DownloadingState) {
        m_downloader->stopDownload();
        ui->progressBar->setValue(0);
    }

    m_downloadState = GrooveOff::AbortedState;
    stateChanged();

    // FIXME: seems that file remove is slower than this call
    // and the icon is updated as expected
    if( The::mainWindow() )
        The::mainWindow()->reloadItemsDownloadButtons();

    removeEmptyFolder(QFileInfo(m_playlistItem->path() + m_playlistItem->fileName()).absoluteDir());
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
