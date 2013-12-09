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


#include "audioplayer.h"
#include "ui_audioplayer.h"
#include "utility.h"

#include <QDir>
#include <QTime>

/*!
  \brief AudioPlayer: this is the AudioPlayer constructor.
  \param parent: The Parent Widget
*/
AudioPlayer::AudioPlayer(QWidget *parent) :
    QWidget(parent),
    ui_(new Ui::AudioPlayer)
{
    ui_->setupUi(this);
    setupUi();

    item = NULL;

    // The AudioOutput class is used to send data to audio output devices
    audioOutput_ = new Phonon::AudioOutput(Phonon::MusicCategory, this);

    // The media object knows how to playback multimedia
    mediaObject_ = new Phonon::MediaObject(this);

    // Get the meta information from the music files
    metaInformationResolver_ = new Phonon::MediaObject(this);

    // Supply with the MediaObject object seekSlider should control
    ui_->seekSlider->setMediaObject(mediaObject_);

    mediaObject_->setTickInterval(1000);

    // Phonon is a graph based framework, i.e., its objects are nodes that
    // can be connected by paths. Objects are connected using the createPath()
    // function, which is part of the Phonon namespace.
    Phonon::createPath(mediaObject_, audioOutput_);

    connect(mediaObject_, SIGNAL(stateChanged(Phonon::State,Phonon::State)),
            this, SLOT(stateChanged(Phonon::State,Phonon::State)));

    connect(mediaObject_, SIGNAL(tick(qint64)), this, SLOT(tick(qint64)));

    connect(mediaObject_, SIGNAL(aboutToFinish()), this, SLOT(aboutToFinish()));

    updateState_ = true;
}

/*!
  \brief ~AudioPlayer: this is the AudioPlayer destructor.
*/
AudioPlayer::~AudioPlayer()
{
    mediaObject_->stop();
}

/*!
  \brief setupUi: setup ui elements
  \return void
*/
void AudioPlayer::setupUi()
{
    ui_->labelsContainerWidget->setVisible(false);

    // Now playing %title by %artist from album %album
    ui_->label->setText(trUtf8("Now playing:"));
    ui_->label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);

    ui_->titleLabel->setFont(Utility::font(QFont::Bold));
    ui_->titleLabel->setWordWrap(true);

    ui_->playPauseButton->setIcon(QIcon::fromTheme(QLatin1String("media-playback-start"), QIcon(QLatin1String(":/resources/media-playback-start.png"))));
    ui_->playPauseButton->setFlat(true);
    ui_->playPauseButton->setToolTip(trUtf8("Play"));
    ui_->playPauseButton->setMaximumSize(QSize(24,24));
    ui_->playPauseButton->setEnabled(false);
    connect(ui_->playPauseButton, SIGNAL(clicked(bool)), this, SLOT(pauseResumePlaying()));

    ui_->stopButton->setIcon(QIcon::fromTheme(QLatin1String("media-playback-stop"), QIcon(QLatin1String(":/resources/media-playback-stop.png"))));
    ui_->stopButton->setToolTip(trUtf8("Stop "));
    ui_->stopButton->setFlat(true);
    ui_->stopButton->setMaximumSize(QSize(24,24));
    connect(ui_->stopButton, SIGNAL(clicked(bool)), this, SLOT(stopPlaying()));

    ui_->timeLabel->setText("00:00");
    ui_->timeLabel->setMinimumSize(QSize(50,0));
    ui_->timeLabel->setFont(Utility::monoFont());
}

/*!
  \brief play: play a file
  \param i: item
  \return void
*/
void AudioPlayer::playItem(DownloadItem *i)
{
    updateState_ = true;

    if(item)
        item->setPlayerState(Phonon::StoppedState);

    item = i;

    QString song = item->path() + QDir::separator() + QString(item->fileName()).replace('/','-');

    QFileInfo fi(song);

    ui_->titleLabel->setText(fi.fileName());
    ui_->titleLabel->setToolTip(fi.fileName());

    // very minimal playlist: only one song
    // first stop current media
    mediaObject_->stop();

    // clear media sources list
    audioSources_.clear();

    currentSongFileName_ = song;

    // define a new media source and append
    Phonon::MediaSource source(currentSongFileName_ + ".mp3");
    audioSources_.append(source);

    // set metaInformer, mediaobject and play
    metaInformationResolver_->setCurrentSource(audioSources_.at(0));
    mediaObject_->setCurrentSource(audioSources_.at(0));
    mediaObject_->play();

    // ui setup
    ui_->stopButton->setIcon(QIcon::fromTheme(QLatin1String("media-playback-stop"), QIcon(QLatin1String(":/resources/media-playback-stop.png"))));
    ui_->playPauseButton->setEnabled(true);
    ui_->playPauseButton->setIcon(QIcon::fromTheme(QLatin1String("media-playback-pause"), QIcon(QLatin1String(":/resources/media-playback-pause.png"))));
    ui_->playPauseButton->setToolTip(trUtf8("Pause"));
}

/*!
  \brief remove: remove a song
  \param song: file name
  \return void
*/
void AudioPlayer::removeItem(DownloadItem *i)
{
    // this slot is activated from a signal in downloadlist widget
    // when there you want to remove a song you have to alert the media player
    // who stops playing if removed song is current playing song
    if(item == i) {
        item->setPlayerState(Phonon::StoppedState);
        mediaObject_->stop();
        ui_->playPauseButton->setEnabled(false);
        ui_->playPauseButton->setIcon(QIcon::fromTheme(QLatin1String("media-playback-start"), QIcon(QLatin1String(":/resources/media-playback-start.png"))));
        audioSources_.clear();
        item = NULL;
    }
}

/*!
  \brief stateChanged: monitor state changes
  \param newState: new state
  \param oldState: old state
  \return void
*/
void AudioPlayer::stateChanged(Phonon::State newState, Phonon::State oldState)
{
    Q_UNUSED(oldState)

    switch (newState) {
    case Phonon::ErrorState:
        if (mediaObject_->errorType() == Phonon::FatalError) {
            qDebug() << "GrooveOff :: "  << "Fatal Error: " << mediaObject_->errorString();
        } else {
            qDebug() << "GrooveOff :: "  << "Error: " << mediaObject_->errorString();
        }
        break;
    case Phonon::PlayingState:
        ui_->labelsContainerWidget->setVisible(true);
        ui_->playPauseButton->setIcon(QIcon::fromTheme(QLatin1String("media-playback-pause"), QIcon(QLatin1String(":/resources/media-playback-pause.png"))));
            if(item)
                item->setPlayerState(Phonon::PlayingState);
        break;
    case Phonon::StoppedState:
        ui_->labelsContainerWidget->setVisible(false);
        ui_->playPauseButton->setIcon(QIcon::fromTheme(QLatin1String("media-playback-start"), QIcon(QLatin1String(":/resources/media-playback-start.png"))));
        ui_->timeLabel->setText("00:00");
            if(item)
                item->setPlayerState(Phonon::StoppedState);
        break;
    case Phonon::PausedState:
        ui_->labelsContainerWidget->setVisible(true);
        ui_->playPauseButton->setIcon(QIcon::fromTheme(QLatin1String("media-playback-start"), QIcon(QLatin1String(":/resources/media-playback-start.png"))));
            if(item)
                item->setPlayerState(Phonon::PausedState);
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
void AudioPlayer::tick(qint64 time)
{
    QTime displayTime(0, (time / 60000) % 60, (time / 1000) % 60);
    ui_->timeLabel->setText(displayTime.toString("mm:ss"));
}

/*!
  \brief aboutToFinish: guaranteed to be emitted while there is still time to enqueue another file for playback.
  \return void
*/
void AudioPlayer::aboutToFinish()
{
    int index = audioSources_.indexOf(mediaObject_->currentSource()) + 1;
    if (audioSources_.size() > index) {
        mediaObject_->enqueue(audioSources_.at(index));
    }
}

/*!
  \brief stopPlaying: stop playing
  \return void
*/
void AudioPlayer::stopPlaying()
{
    mediaObject_->stop();
}

/*!
  \brief pauseResumePlaying: controls pause/resume
  \return void
*/
void AudioPlayer::pauseResumePlaying()
{
    if(mediaObject_->state() == Phonon::PlayingState) {
        mediaObject_->pause();
        ui_->playPauseButton->setIcon(QIcon::fromTheme(QLatin1String("media-playback-start"), QIcon(QLatin1String(":/resources/media-playback-start.png"))));
        ui_->playPauseButton->setToolTip(trUtf8("Play"));
    } else{
        mediaObject_->play();
        ui_->labelsContainerWidget->setVisible(true);
        ui_->playPauseButton->setIcon(QIcon::fromTheme(QLatin1String("media-playback-pause"), QIcon(QLatin1String(":/resources/media-playback-pause.png"))));
        ui_->playPauseButton->setToolTip(trUtf8("Pause"));
    }
}

#include "audioplayer.moc"
