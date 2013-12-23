/*
    GrooveOff - Offline Grooveshark.com music
    Copyright (C) 2013  Giuseppe Calà <jiveaxe@gmail.com>

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


#include "playerwidget.h"
#include "ui_playerwidget.h"
#include "utility.h"
#include "audioengine.h"
#include "playlist.h"

#include <QDir>
#include <QTime>
#include <QTimer>
#include <QGraphicsDropShadowEffect>

/*!
  \brief PlayerWidget: this is the PlayerWidget constructor.
  \param parent: The Parent Widget
*/
PlayerWidget::PlayerWidget(QWidget *parent) :
    QWidget(parent),
    ui_(new Ui::PlayerWidget)
{
    ui_->setupUi(this);
    setupUi();

    timer_ = new QTimer(this);
    connect(timer_, SIGNAL(timeout()), this, SLOT(switchPage()));

    // Supply with the MediaObject object seekSlider should control
    ui_->seekSlider->setMediaObject(AudioEngine::instance()->mediaObject());

    playedRemoved = false;

    connect(ui_->timeLabel, SIGNAL(clicked()),
            this, SLOT(toggleTimeLabel()));

    connect(AudioEngine::instance(), SIGNAL(seeked(qint64)),
            this, SLOT(tick(qint64)));
    connect(AudioEngine::instance(), SIGNAL(stateChanged(Phonon::State,Phonon::State)),
            this, SLOT(stateChanged(Phonon::State, Phonon::State)));
    connect(AudioEngine::instance(), SIGNAL(sourceChanged()),
            this, SLOT(sourceChanged()));
    connect(Playlist::instance(), SIGNAL(playlistChanged()),
            this, SLOT(reloadPreviousNextButtons()));
    connect(AudioEngine::instance(), SIGNAL(removedPlayingTrack()),
            this, SLOT(removedPlayingTrack()));
}

/*!
  \brief ~PlayerWidget: this is the PlayerWidget destructor.
*/
PlayerWidget::~PlayerWidget()
{
    AudioEngine::instance()->stop(Phonon::NoError);
}

/*!
  \brief setupUi: setup ui elements
  \return void
*/
void PlayerWidget::setupUi()
{
    QGraphicsDropShadowEffect *coverShadow = new QGraphicsDropShadowEffect(this);
    coverShadow->setBlurRadius(10.0);
    coverShadow->setColor(palette().color(QPalette::Shadow));
    coverShadow->setOffset(0.0);

    ui_->coverLabel->setWindowFlags(Qt::FramelessWindowHint);
    ui_->coverLabel->setAttribute(Qt::WA_TranslucentBackground);
    ui_->coverLabel->setGraphicsEffect(coverShadow);

    ui_->coverLabel->setScaledContents(true);
    ui_->titleLabel->setFont(Utility::font(QFont::Bold));
    ui_->album_authorLabel->setFont(Utility::font(QFont::Bold));
    ui_->stackedWidget->setBackgroundRole(QPalette::Base);
    ui_->messageLabel->setFont(Utility::font(QFont::Bold,1));

    ui_->previousButton->setIcon(QIcon::fromTheme(QLatin1String("media-seek-backward"),
                                 QIcon(QLatin1String(":/resources/media-seek-backward.png"))));
    ui_->previousButton->setFlat(true);
    ui_->previousButton->setStyleSheet("border: none; outline: none;");
    ui_->previousButton->setToolTip(trUtf8("Play Previous"));
    ui_->previousButton->setFixedSize(QSize(16,16));
    ui_->previousButton->setIconSize(QSize(16,16));
    ui_->previousButton->setEnabled(false);
    connect(ui_->previousButton, SIGNAL(clicked(bool)), this, SLOT(playPrevious()));

    ui_->playPauseButton->setIcon(QIcon::fromTheme(QLatin1String("media-playback-start"),
                                  QIcon(QLatin1String(":/resources/media-playback-start.png"))));
    ui_->playPauseButton->setFlat(true);
    ui_->playPauseButton->setStyleSheet("border: none; outline: none;");
    ui_->playPauseButton->setToolTip(trUtf8("Play"));
    ui_->playPauseButton->setFixedSize(QSize(32,32));
    ui_->playPauseButton->setIconSize(QSize(32,32));
    ui_->playPauseButton->setEnabled(false);
    connect(ui_->playPauseButton, SIGNAL(clicked(bool)),
            this, SLOT(pauseResumePlaying()));

    ui_->nextButton->setIcon(QIcon::fromTheme(QLatin1String("media-seek-forward"),
                             QIcon(QLatin1String(":/resources/media-seek-forward.png"))));
    ui_->nextButton->setFlat(true);
    ui_->nextButton->setStyleSheet("border: none; outline: none;");
    ui_->nextButton->setToolTip(trUtf8("Play Next"));
    ui_->nextButton->setFixedSize(QSize(16,16));
    ui_->nextButton->setIconSize(QSize(16,16));
    ui_->nextButton->setEnabled(false);
    connect(ui_->nextButton, SIGNAL(clicked(bool)),
            this, SLOT(playNext()));

    ui_->timeLabel->setText("00:00");
    ui_->timeLabel->setMinimumSize(QSize(50,0));
    ui_->timeLabel->setFont(Utility::monoFont());

    ui_->elapsedTimeLabel->setText("00:00");
    ui_->elapsedTimeLabel->setMinimumSize(QSize(50,0));
    ui_->elapsedTimeLabel->setFont(Utility::monoFont());
}

/*!
  \brief showElapsedTimerLabel: show/hide elapsedTimeLabel
  \param ok: bool
  \return void
*/
void PlayerWidget::showElapsedTimerLabel(bool ok)
{
    ui_->elapsedTimeLabel->setVisible(ok);
}

/*!
  \brief stateChanged: monitor state changes
  \param newState: new state
  \param oldState: old state
  \return void
*/
void PlayerWidget::stateChanged(Phonon::State newState, Phonon::State oldState)
{
    Q_UNUSED(oldState)

    switch (newState) {
    case Phonon::ErrorState:
        ui_->stackedWidget->setCurrentIndex(0);
        break;
    case Phonon::PlayingState:
        playedRemoved = false;
        ui_->stackedWidget->setCurrentIndex(1);
        ui_->playPauseButton->setEnabled(true);
        ui_->playPauseButton->setIcon(QIcon::fromTheme(QLatin1String("media-playback-pause"),
                                      QIcon(QLatin1String(":/resources/media-playback-pause.png"))));
        break;
    case Phonon::StoppedState:
        if(playedRemoved)
            ui_->stackedWidget->setCurrentIndex(0);
        else
            ui_->stackedWidget->setCurrentIndex(1);
        ui_->playPauseButton->setEnabled(false);
        ui_->playPauseButton->setIcon(QIcon::fromTheme(QLatin1String("media-playback-start"),
                                      QIcon(QLatin1String(":/resources/media-playback-start.png"))));
        ui_->timeLabel->setText("00:00");
        ui_->elapsedTimeLabel->setText("00:00");
        break;
    case Phonon::PausedState:
        ui_->stackedWidget->setCurrentIndex(1);
        ui_->playPauseButton->setIcon(QIcon::fromTheme(QLatin1String("media-playback-start"),
                                      QIcon(QLatin1String(":/resources/media-playback-start.png"))));
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
void PlayerWidget::tick(qint64 elapsedTime)
{
    if(AudioEngine::instance()->state() == Phonon::StoppedState)
        return;

    quint64 remainingTime = AudioEngine::instance()->remainingTime();

    if(ui_->elapsedTimeLabel->isVisible()) {
        ui_->elapsedTimeLabel->setText(QTime(0, (elapsedTime / 60000) % 60, (elapsedTime / 1000) % 60).toString("mm:ss"));
        ui_->timeLabel->setText("-" + QTime(0, (remainingTime / 60000) % 60, (remainingTime / 1000) % 60).toString("mm:ss"));
    } else {
        switch(timerState) {
            case GrooveOff::ElapsedState:
                ui_->timeLabel->setText(QTime(0, (elapsedTime / 60000) % 60, (elapsedTime / 1000) % 60).toString("mm:ss"));
                break;
            case GrooveOff::RemainingState:
                ui_->timeLabel->setText("-" + QTime(0, (remainingTime / 60000) % 60, (remainingTime / 1000) % 60).toString("mm:ss"));
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
    playedRemoved = false;
    AudioEngine::instance()->playPause();
}

void PlayerWidget::toggleTimeLabel()
{
    timerState = (GrooveOff::TimerState)!timerState;
}

void PlayerWidget::showMessage(const QString& message)
{
    if(AudioEngine::instance()->state() == Phonon::PlayingState || AudioEngine::instance()->state() == Phonon::PausedState) {
        timer_->setSingleShot(true);
        timer_->start(3000);
    }
    ui_->stackedWidget->setCurrentIndex(0);
    ui_->messageLabel->setText(message);
}

void PlayerWidget::sourceChanged()
{
    reloadPreviousNextButtons();
    QString title = AudioEngine::instance()->currentTrack()->song()->songName();
    QString artist = AudioEngine::instance()->currentTrack()->song()->artistName();
    QString album = AudioEngine::instance()->currentTrack()->song()->albumName();
    QString path = AudioEngine::instance()->currentTrack()->path();
    QString coverName = AudioEngine::instance()->currentTrack()->song()->coverArtFilename();

    ui_->titleLabel->setText(title);
    ui_->titleLabel->setToolTip(title);

    ui_->album_authorLabel->setText(artist + " - " + album);

    if(!coverName.isEmpty()
        && QFile::exists(Utility::coversCachePath + coverName)
        && coverName != "0")
        ui_->coverLabel->setPixmap(QPixmap(Utility::coversCachePath + coverName));
    else
        ui_->coverLabel->setPixmap(QIcon::fromTheme(QLatin1String("media-optical"),
                                   QIcon(QLatin1String(":/resources/media-optical.png"))).pixmap(ui_->coverLabel->size()));
}

void PlayerWidget::playNext()
{
    AudioEngine::instance()->next();
}

void PlayerWidget::playPrevious()
{
    AudioEngine::instance()->previous();
}

void PlayerWidget::reloadPreviousNextButtons()
{
    if(Playlist::instance()->count() == 0)
        ui_->stackedWidget->setCurrentIndex(0);
    ui_->previousButton->setEnabled(AudioEngine::instance()->canGoPrevious());
    ui_->nextButton->setEnabled(AudioEngine::instance()->canGoNext());
}

void PlayerWidget::removedPlayingTrack()
{
    playedRemoved = true;
}

void PlayerWidget::switchPage()
{
    ui_->stackedWidget->setCurrentIndex(1);
}

