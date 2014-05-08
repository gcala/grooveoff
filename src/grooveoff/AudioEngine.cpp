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


#include "AudioEngine.h"
#include "Playlist.h"

#include <QMetaType>
#include <QDir>
#include <QDebug>

namespace The {
    static AudioEngine* s_AudioEngine_instance = 0;

    AudioEngine* audioEngine()
    {
        if( !s_AudioEngine_instance )
            s_AudioEngine_instance = new AudioEngine();

        return s_AudioEngine_instance;
    }
}

AudioEngine::AudioEngine()
    : m_volume( 0 )
{
    // The media object knows how to playback multimedia
    m_mediaObject = new Phonon::MediaObject( this );

    // The AudioOutput class is used to send data to audio output devices
    m_audioOutput = new Phonon::AudioOutput( Phonon::MusicCategory, this );

    m_mediaObject->setTickInterval( 1000 );

    m_state = Phonon::StoppedState;

    // Phonon is a graph based framework, i.e., its objects are nodes that
    // can be connected by paths. Objects are connected using the createPath()
    // function, which is part of the Phonon namespace.
    Phonon::createPath(m_mediaObject, m_audioOutput);

    connect( m_mediaObject, SIGNAL(stateChanged(Phonon::State,Phonon::State)),
                            SLOT(onStateChanged(Phonon::State,Phonon::State))
           );
    connect( m_mediaObject, SIGNAL(tick(qint64)),
                            SLOT(timerTriggered(qint64))
           );
    connect( m_mediaObject, SIGNAL(finished()),
                            SLOT(onFinished())
           );
    connect( m_mediaObject, SIGNAL(currentSourceChanged(Phonon::MediaSource)),
                            SLOT(sourceChanged(Phonon::MediaSource))
           );
    connect( m_audioOutput, SIGNAL(volumeChanged(qreal)),
                            SLOT(onVolumeChanged(qreal))
           );
    connect( m_audioOutput, SIGNAL(mutedChanged(bool)),
                            SLOT(onMutedChanged(bool))
           );
    connect( m_mediaObject, SIGNAL(seekableChanged(bool)),
                            SLOT(onSeekableChanged(bool))
           );
    connect( m_mediaObject, SIGNAL(totalTimeChanged(qint64)),
                            SLOT(onTrackLengthChanged(qint64))
           );

    // Read the volume from phonon
    m_volume = qBound<qreal>( 0, qRound( m_audioOutput->volume() * 100 ), 100 );

    setVolume( 50 );
}

AudioEngine::~AudioEngine()
{
}

void AudioEngine::timerTriggered( qint64 time )
{
    emit seeked( time, false );
}

void AudioEngine::onFinished()
{
    m_currentTrack->setState( Phonon::StoppedState );
    next();
}

void AudioEngine::playItem( const PlaylistItemPtr &track)
{
    if( m_currentTrack )
        m_currentTrack->setState( Phonon::StoppedState );

    m_currentTrack = track;
    m_oldTrack = track;
    m_mediaObject->setCurrentSource( QUrl::fromLocalFile( m_currentTrack->path() + QDir::separator() + m_currentTrack->fileName() ) );
    play();
}

void AudioEngine::onStateChanged( Phonon::State newState, Phonon::State oldState )
{
    Q_UNUSED( oldState )

    if ( newState == Phonon::ErrorState )
    {
        if ( m_mediaObject->errorType() == Phonon::FatalError ) {
            qDebug() << "GrooveOff :: "  << "Fatal Error: " << m_mediaObject->errorString();
        } else {
            qDebug() << "GrooveOff :: "  << "Error: " << m_mediaObject->errorString();
        }

        m_mediaObject->stop();
    }

    m_state = newState;

    emit stateChanged( newState );
}

void AudioEngine::playPause()
{
    isPlaying() ? pause() : play();
}

void AudioEngine::pause()
{
    m_mediaObject->pause();
    m_currentTrack->setState( Phonon::PausedState );
}

void AudioEngine::play()
{
    if( m_mediaObject->currentSource().url().isEmpty() ) {
        if( The::playlist()->count() > 0 ) {
            playItem( The::playlist()->item( 0 ) );
            return;
        }
    }
    m_mediaObject->play();
    m_currentTrack->setState( Phonon::PlayingState );
}

void AudioEngine::stop( Phonon::ErrorType errorCode )
{
    if( m_state == Phonon::PlayingState || m_state == Phonon::PausedState ) {
        m_mediaObject->stop();
        m_currentTrack->setState( Phonon::StoppedState );
    }
}


bool AudioEngine::isPlaying() const
{
    return m_state == Phonon::PlayingState;
}

bool AudioEngine::isPaused() const
{
    return m_state == Phonon::PausedState;
}

bool AudioEngine::isStopped() const
{
    return m_state == Phonon::StoppedState;
}

Phonon::State AudioEngine::state() const
{
    return m_state;
}

qint64 AudioEngine::currentTime() const
{
    return m_mediaObject->currentTime();
}

qint64 AudioEngine::currentTrackTotalTime() const
{
    return m_mediaObject->totalTime();
}

PlaylistItemPtr AudioEngine::currentTrack() const
{
    return m_currentTrack;
}

void AudioEngine::sourceChanged( Phonon::MediaSource )
{
    emit sourceChanged();
}

qint64 AudioEngine::remainingTime() const
{
    return m_mediaObject->remainingTime();
}

bool AudioEngine::canGoNext()
{
    if( m_oldTrack ) {
        int currentIndex = The::playlist()->row( m_oldTrack );
        if( currentIndex >= 0 ) {
            if( currentIndex < ( The::playlist()->count() - 1 ) )
                return true;
        }
    }

    return false;
}

bool AudioEngine::canGoPrevious()
{
    if( m_oldTrack ) {
        int currentIndex = The::playlist()->row( m_oldTrack );
        if( currentIndex > 0 )
            return true;
    }

    return false;
}

void AudioEngine::next()
{
    if( canGoNext() ) {
        PlaylistItemPtr track = The::playlist()->item( The::playlist()->row( m_oldTrack ) + 1 );
        if( m_currentTrack )
            m_currentTrack->setState( Phonon::StoppedState );

        m_currentTrack = track;
        m_oldTrack = track;
        m_mediaObject->setCurrentSource( QUrl::fromLocalFile( m_currentTrack->path() + QDir::separator() + m_currentTrack->fileName() ) );
        play();
    }
}

void AudioEngine::previous()
{
    if( canGoPrevious() ) {
        PlaylistItemPtr track = The::playlist()->item( The::playlist()->row( m_currentTrack ) - 1 );
        if( m_currentTrack )
            m_currentTrack->setState( Phonon::StoppedState );

        m_currentTrack = track;
        m_oldTrack = track;
        m_mediaObject->setCurrentSource( QUrl::fromLocalFile( m_currentTrack->path() + QDir::separator() + m_currentTrack->fileName() ) );
        play();
    }
}

void AudioEngine::removingTrack( const PlaylistItemPtr &track )
{
    if( m_currentTrack == track ) {
        emit removedPlayingTrack();
        m_mediaObject->stop();
        m_currentTrack = PlaylistItemPtr();
        next();
    }
}

void AudioEngine::seek( qint64 ms )
{
    if ( !isSeekable() )
    {
        qDebug() << "Could not seek!";
        return;
    }

    if ( isPlaying() || isPaused() )
    {
        m_mediaObject->seek( ms );
        emit seeked( ms, true );
    }
}

void
AudioEngine::seek( int ms )
{
    seek( (qint64) ms );
}

void AudioEngine::setMuted( bool mute )
{
    m_audioOutput->setMuted( mute );
    if( !isMuted() )
        setVolume( m_volume );
    emit muteStateChanged( mute );
}

void AudioEngine::setVolume( int percentage )
{
    percentage = qBound( 0, percentage, 100 );
    m_volume = percentage;
    const qreal volume =  percentage / 100.0;

    if ( m_audioOutput->volume() != volume )
    {
        m_audioOutput->setVolume( volume );
        emit volumeChanged( percentage );
    }
}

void AudioEngine::onVolumeChanged( qreal volume )
{
    int percent = qBound<qreal>( 0, qRound( volume * 100 ), 100 );
    if ( m_volume != percent )
        emit volumeChanged( percent );
    m_volume = percent;
}

void AudioEngine::onMutedChanged( bool mute )
{
    emit muteStateChanged( mute );
}

bool AudioEngine::isMuted() const
{
    return m_audioOutput->isMuted();
}

int AudioEngine::volume() const
{
    return m_volume;
}

bool AudioEngine::isSeekable() const
{
    if( m_mediaObject )
        return m_mediaObject->isSeekable();
    return false;
}

void AudioEngine::onSeekableChanged( bool seekable )
{
    emit seekableChanged( seekable );
}

void AudioEngine::onTrackLengthChanged( qint64 milliseconds )
{
    emit trackLengthChanged( milliseconds );
}
