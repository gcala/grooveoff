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


#include "PlayerWidget.h"
#include "ui_PlayerWidget.h"
#include "Utility.h"
#include "AudioEngine.h"
#include "Playlist.h"

#include <QDir>
#include <QTime>
#include <QTimer>
#include <QGraphicsDropShadowEffect>

// Taglib
#include <taglib/fileref.h>
#include <taglib/audioproperties.h>


/*!
  \brief PlayerWidget: this is the PlayerWidget constructor.
  \param parent: The Parent Widget
*/
PlayerWidget::PlayerWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::PlayerWidget)
{
    ui->setupUi(this);
    setupUi();

    m_timer = new QTimer(this);
    connect(m_timer, SIGNAL(timeout()), this, SLOT(switchPage()));

    // Supply with the MediaObject object seekSlider should control
    ui->seekSlider->setMediaObject(The::audioEngine()->mediaObject());

    m_playedRemoved = false;

    connect( ui->timeLabel, SIGNAL(clicked()),
                            SLOT(toggleTimeLabel()));

    connect( The::audioEngine(), SIGNAL(seeked(qint64, bool)),
                                 SLOT(tick(qint64, bool)));

    connect( The::audioEngine(), SIGNAL(stateChanged(Phonon::State)),
                                 SLOT(stateChanged(Phonon::State)));

    connect( The::audioEngine(), SIGNAL(sourceChanged()),
                                 SLOT(sourceChanged()));

    connect( The::audioEngine(), SIGNAL(removedPlayingTrack()),
                                 SLOT(removedPlayingTrack()));

    connect( The::audioEngine(), SIGNAL(volumeChanged(int)),
             ui->volume,         SLOT(setValue(int)) );

    connect( The::audioEngine(), SIGNAL(muteStateChanged(bool)),
                                 SLOT(muteStateChanged(bool)) );

    connect( The::playlist(), SIGNAL(playlistChanged()),
                              SLOT(reloadPreviousNextButtons()));

    connect( ui->volume,         SIGNAL(valueChanged(int)),
             The::audioEngine(), SLOT(setVolume(int)) );

    connect( ui->volume,         SIGNAL(muteToggled(bool)),
             The::audioEngine(), SLOT(setMuted(bool)) );
}

/*!
  \brief ~PlayerWidget: this is the PlayerWidget destructor.
*/
PlayerWidget::~PlayerWidget()
{
    The::audioEngine()->stop(Phonon::NoError);
}

/*!
  \brief setupUi: setup ui elements
  \return void
*/
void PlayerWidget::setupUi()
{
    QGraphicsDropShadowEffect *coverShadow = new QGraphicsDropShadowEffect(this);
    coverShadow->setBlurRadius(10.0);
    coverShadow->setColor(palette().color(QPalette::Highlight));
    coverShadow->setOffset(0.0);

    ui->coverLabel->setWindowFlags(Qt::FramelessWindowHint);
    ui->coverLabel->setAttribute(Qt::WA_TranslucentBackground);
    ui->coverLabel->setGraphicsEffect(coverShadow);

    ui->coverLabel->setScaledContents(true);
    ui->titleLabel->setFont(Utility::font(QFont::Bold));
    ui->album_authorLabel->setFont(Utility::font(QFont::Bold));
    ui->stackedWidget->setBackgroundRole(QPalette::AlternateBase);
    ui->messageLabel->setFont(Utility::font(QFont::Bold,1));

    ui->previousButton->setButtonEnabled(false);

    connect(ui->previousButton, SIGNAL(previousButtonClicked()),
                                SLOT(playPrevious()));

    ui->playPauseButton->setButtonEnabled(false);
    ui->playPauseButton->setPlaying(false);

    connect(ui->playPauseButton, SIGNAL(playButtonClicked()),
                                 SLOT(pauseResumePlaying()));

    ui->nextButton->setButtonEnabled(false);
    connect(ui->nextButton, SIGNAL(nextButtonClicked()),
                            SLOT(playNext()));

    ui->timeLabel->setText( QLatin1String( "00:00" ) );
    ui->timeLabel->setMinimumSize(QSize(50,0));
    ui->timeLabel->setFont(Utility::monoFont());

    ui->elapsedTimeLabel->setText( QLatin1String( "00:00") );
    ui->elapsedTimeLabel->setMinimumSize(QSize(50,0));
    ui->elapsedTimeLabel->setFont(Utility::monoFont());

    ui->volume->setFixedSize(QSize(48,48));
    ui->volume->setValue(50);

    ui->bitrateLabel->setToolTip(trUtf8("Bit Rate"));
    ui->samplerateLabel->setToolTip(trUtf8("Sample Rate"));
//     ui->channelsLabel->setToolTip();

    ui->bitrateLabel->setFont(Utility::font(QFont::Bold,-3));
    ui->samplerateLabel->setFont(Utility::font(QFont::Bold,-3));
    ui->channelsLabel->setFont(Utility::font(QFont::Bold,-3));
}

/*!
  \brief showElapsedTimerLabel: show/hide elapsedTimeLabel
  \param ok: bool
  \return void
*/
void PlayerWidget::showElapsedTimerLabel(bool ok)
{
    ui->elapsedTimeLabel->setVisible(ok);
}

/*!
  \brief stateChanged: monitor state changes
  \param newState: new state
  \param oldState: old state
  \return void
*/
void PlayerWidget::stateChanged(Phonon::State state)
{
    switch (state) {
    case Phonon::ErrorState:
        ui->stackedWidget->setCurrentIndex(0);
        ui->playPauseButton->setPlaying(false);
        break;
    case Phonon::PlayingState:
        m_playedRemoved = false;
        ui->stackedWidget->setCurrentIndex(1);
        ui->playPauseButton->setButtonEnabled(true);
        ui->playPauseButton->setPlaying(true);
        break;
    case Phonon::StoppedState:
        if(m_playedRemoved)
            ui->stackedWidget->setCurrentIndex(0);
        else
            ui->stackedWidget->setCurrentIndex(1);
        if(m_playedRemoved) {
            if(!The::playlist()->count())
                ui->playPauseButton->setButtonEnabled(false);
        }
        ui->playPauseButton->setPlaying(false);
        ui->timeLabel->setText( QLatin1String( "00:00" ) );
        ui->elapsedTimeLabel->setText( QLatin1String( "00:00" ) );
        break;
    case Phonon::PausedState:
        ui->stackedWidget->setCurrentIndex(1);
        ui->playPauseButton->setPlaying(false);
        break;
    default:
        /* do nothing */
        break;
    }
}

/*!
  \brief tick: change in playback position
  \param time: current play time
  \return void
*/
void PlayerWidget::tick(qint64 elapsedTime, bool userSeek)
{
    Q_UNUSED(userSeek)

    if(The::audioEngine()->state() == Phonon::StoppedState)
        return;

    quint64 remainingTime = The::audioEngine()->remainingTime();

    if(ui->elapsedTimeLabel->isVisible()) {
        ui->elapsedTimeLabel->setText(QTime(0, (elapsedTime / 60000) % 60, (elapsedTime / 1000) % 60).toString("mm:ss"));
        ui->timeLabel->setText("-" + QTime(0, (remainingTime / 60000) % 60, (remainingTime / 1000) % 60).toString("mm:ss"));
    } else {
        switch(m_timerState) {
            case GrooveOff::ElapsedState:
                ui->timeLabel->setText(QTime(0, (elapsedTime / 60000) % 60, (elapsedTime / 1000) % 60).toString("mm:ss"));
                break;
            case GrooveOff::RemainingState:
                ui->timeLabel->setText("-" + QTime(0, (remainingTime / 60000) % 60, (remainingTime / 1000) % 60).toString("mm:ss"));
                break;
        }
    }
}

/*!
  \brief pauseResumePlaying: controls pause/resume
  \return void
*/
void PlayerWidget::pauseResumePlaying()
{
    m_playedRemoved = false;
    if(The::audioEngine()->currentTrack())
        The::audioEngine()->playPause();
    else {
        if(The::playlist()->count() > 0)
            The::audioEngine()->playItem(The::playlist()->item(0));
    }
}

void PlayerWidget::toggleTimeLabel()
{
    m_timerState = (GrooveOff::TimerState)!m_timerState;
}

void PlayerWidget::showMessage(const QString& message)
{
    if(The::audioEngine()->state() == Phonon::PlayingState || The::audioEngine()->state() == Phonon::PausedState) {
        m_timer->setSingleShot(true);
        m_timer->start(3000);
    }
    ui->stackedWidget->setCurrentIndex(0);
    ui->messageLabel->setText(message);
}

void PlayerWidget::sourceChanged()
{
    PlaylistItemPtr track = The::audioEngine()->currentTrack();
    reloadPreviousNextButtons();
    QString title = track->song()->songName();
    QString artist = track->song()->artistName();
    QString album = track->song()->albumName();
    QString coverName = track->song()->coverArtFilename();

    ui->titleLabel->setText(title);
    ui->titleLabel->setToolTip(title);

    ui->album_authorLabel->setText(artist + " - " + album);

    if(!coverName.isEmpty()
        && QFile::exists(Utility::coversCachePath + coverName)
        && coverName != "0")
        ui->coverLabel->setPixmap(QPixmap(Utility::coversCachePath + coverName));
    else
        ui->coverLabel->setPixmap(QIcon::fromTheme(QLatin1String("media-optical"),
                                   QIcon(QLatin1String(":/resources/media-optical.png"))).pixmap(ui->coverLabel->size()));

    // Audio Properties
    TagLib::FileRef f(QString(track->path() + track->fileName()).toLatin1(), true, TagLib::AudioProperties::Average);
    if(f.isNull()) {
        qDebug() << track->path() + track->fileName() << "is null";
        return;
    }

    int bitrate = f.audioProperties()->bitrate();        // in kb/s
    int channels =  f.audioProperties()->channels();
    int sampleRate =  f.audioProperties()->sampleRate(); // in Hz

    ui->bitrateLabel->setText(QString::number(bitrate) + QLatin1String(" kb/s"));
    ui->samplerateLabel->setText(QString::number(sampleRate) + QLatin1String(" Hz"));
    ui->channelsLabel->setText(channels >= 2 ? QLatin1String("STEREO") : QLatin1String("MONO"));
}

void PlayerWidget::playNext()
{
    The::audioEngine()->next();
}

void PlayerWidget::playPrevious()
{
    The::audioEngine()->previous();
}

void PlayerWidget::reloadPreviousNextButtons()
{
    if(The::playlist()->count() == 0)
        ui->stackedWidget->setCurrentIndex(0);
    else {
        ui->playPauseButton->setButtonEnabled(true);
        if(The::audioEngine()->canGoNext()) {
            int currentRow = The::playlist()->row(The::audioEngine()->currentTrack());
            ui->nextButton->setToolTip(The::playlist()->item(currentRow + 1)->song()->songName());
        } else {
            ui->nextButton->setToolTip("");
        }

        if(The::audioEngine()->canGoPrevious()) {
            int currentRow = The::playlist()->row(The::audioEngine()->currentTrack());
            ui->previousButton->setToolTip(The::playlist()->item(currentRow - 1)->song()->songName());
        } else {
            ui->previousButton->setToolTip("");
        }

        ui->nextButton->setButtonEnabled(The::audioEngine()->canGoNext());
        ui->previousButton->setButtonEnabled(The::audioEngine()->canGoPrevious());
    }
}

void PlayerWidget::removedPlayingTrack()
{
    m_playedRemoved = true;
}

void PlayerWidget::switchPage()
{
    ui->stackedWidget->setCurrentIndex(1);
}

void PlayerWidget::muteStateChanged(bool mute)
{
    ui->volume->setMuted(mute);
}

