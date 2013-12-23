/*
 * GrooveOff - Offline Grooveshark.com music
 * Copyright (C) 2013  Giuseppe Calà <Giuseppe.Cala-1973@poste.it>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "audioengine.h"
#include "playlist.h"

#include <QMetaType>
#include <QDir>
#include <QDebug>

bool AudioEngine::instanceFlag = false;
AudioEngine* AudioEngine::audioEngine = NULL;

AudioEngine* AudioEngine::instance()
{
    if(! instanceFlag)
    {
        audioEngine = new AudioEngine();
        instanceFlag = true;
        return audioEngine;
    }
    else
    {
        return audioEngine;
    }
}

AudioEngine::AudioEngine()
{
    qRegisterMetaType< AudioErrorCode >("AudioErrorCode");
    qRegisterMetaType< AudioState >("AudioState");

    // The media object knows how to playback multimedia
    mediaObject_ = new Phonon::MediaObject( this );

    // The AudioOutput class is used to send data to audio output devices
    audioOutput_ = new Phonon::AudioOutput(Phonon::MusicCategory, this);

    // Get the meta information from the music files
    metaInformationResolver_ = new Phonon::MediaObject(this);

    // Supply with the MediaObject object seekSlider should control
//    ui_->seekSlider->setMediaObject(mediaObject_);

    mediaObject_->setTickInterval(1000);

    state_ = Phonon::StoppedState;

    // Phonon is a graph based framework, i.e., its objects are nodes that
    // can be connected by paths. Objects are connected using the createPath()
    // function, which is part of the Phonon namespace.
    Phonon::createPath(mediaObject_, audioOutput_);

    connect( mediaObject_, SIGNAL( stateChanged( Phonon::State, Phonon::State ) ), SLOT( onStateChanged( Phonon::State, Phonon::State ) ) );
    connect( mediaObject_, SIGNAL( tick( qint64 ) ), SLOT( timerTriggered( qint64 ) ) );
    connect( mediaObject_, SIGNAL( finished() ), SLOT( onFinished() ) );
    connect(mediaObject_, SIGNAL(currentSourceChanged(Phonon::MediaSource)),
            this, SLOT(sourceChanged(Phonon::MediaSource)));
}

AudioEngine::~AudioEngine()
{
    instanceFlag = false;
}

void AudioEngine::timerTriggered( qint64 time )
{
    emit seeked(time);
}

void AudioEngine::onFinished()
{
    currentTrack_->setState(Phonon::StoppedState);
    next();
}

void AudioEngine::playItem(PlaylistItemPtr track)
{
    if(currentTrack_)
        currentTrack_->setState(Phonon::StoppedState);

    currentTrack_ = track;
    mediaObject_->setCurrentSource(QUrl::fromLocalFile(currentTrack_->path() + QDir::separator() + currentTrack_->fileName()));
    play();
}

void AudioEngine::onStateChanged(Phonon::State newState, Phonon::State oldState)
{
    if ( newState == Phonon::ErrorState )
    {
        if (mediaObject_->errorType() == Phonon::FatalError) {
            qDebug() << "GrooveOff :: "  << "Fatal Error: " << mediaObject_->errorString();
        } else {
            qDebug() << "GrooveOff :: "  << "Error: " << mediaObject_->errorString();
        }

        mediaObject_->stop();
    }

    state_ = newState;

    emit stateChanged(newState, oldState);
}

void AudioEngine::playPause()
{
    if ( isPlaying() )
        pause();
    else
        play();
}

void AudioEngine::pause()
{
    mediaObject_->pause();
    currentTrack_->setState(Phonon::PausedState);
}

void AudioEngine::play()
{
    mediaObject_->play();
    currentTrack_->setState(Phonon::PlayingState);
}

void AudioEngine::stop(Phonon::ErrorType errorCode)
{
    if(state_ == Phonon::PlayingState || state_ == Phonon::PausedState) {
        mediaObject_->stop();
        currentTrack_->setState(Phonon::StoppedState);
    }
}


bool AudioEngine::isPlaying() const
{
    return state_ == Phonon::PlayingState;
}

bool AudioEngine::isPaused() const
{
    return state_ == Phonon::PausedState;
}

bool AudioEngine::isStopped() const
{
    return state_ == Phonon::StoppedState;
}

Phonon::State AudioEngine::state() const
{
    return state_;
}

qint64 AudioEngine::currentTime() const
{
    mediaObject_->currentTime();
}

qint64 AudioEngine::currentTrackTotalTime() const
{
    mediaObject_->totalTime();
}

PlaylistItemPtr AudioEngine::currentTrack() const
{
    return currentTrack_;
}

void AudioEngine::sourceChanged(Phonon::MediaSource)
{
    emit sourceChanged();
}

qint64 AudioEngine::remainingTime() const
{
    return mediaObject_->remainingTime();
}

bool AudioEngine::canGoNext()
{
    if(currentTrack_) {
        int currentIndex = Playlist::instance()->row(currentTrack_);
        if(currentIndex >= 0) {
            if(currentIndex < (Playlist::instance()->count() - 1))
                return true;
        }
        return false;
    } else {
        return false;
    }
}

bool AudioEngine::canGoPrevious()
{
    if(currentTrack_) {
        int currentIndex = Playlist::instance()->row(currentTrack_);
        if(currentIndex > 0)
            return true;
        return false;
    } else {
        return false;
    }
}

void AudioEngine::next()
{
    if(canGoNext()) {
        PlaylistItemPtr track = Playlist::instance()->item(Playlist::instance()->row(currentTrack_) + 1);
        if(currentTrack_)
            currentTrack_->setState(Phonon::StoppedState);

        currentTrack_ = track;
        mediaObject_->setCurrentSource(QUrl::fromLocalFile(currentTrack_->path() + QDir::separator() + currentTrack_->fileName()));
        play();
    }
}

void AudioEngine::previous()
{
    if(canGoPrevious()) {
        PlaylistItemPtr track = Playlist::instance()->item(Playlist::instance()->row(currentTrack_) - 1);
        if(currentTrack_)
            currentTrack_->setState(Phonon::StoppedState);

        currentTrack_ = track;
        mediaObject_->setCurrentSource(QUrl::fromLocalFile(currentTrack_->path() + QDir::separator() + currentTrack_->fileName()));
        play();
    }
}

void AudioEngine::removingTrack(PlaylistItemPtr track)
{
    if(currentTrack_ == track) {
        emit removedPlayingTrack();
        mediaObject_->stop();
        next();
    }
}

