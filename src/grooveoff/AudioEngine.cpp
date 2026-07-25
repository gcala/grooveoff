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

#include <QAudio>
#include <QAudioOutput>
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
    : m_state( GrooveOff::PlayerState::Stopped )
    , m_volume( 0 )
{
    m_mediaPlayer = new QMediaPlayer( this );

    m_audioOutput = new QAudioOutput( this );
    m_mediaPlayer->setAudioOutput( m_audioOutput );

    setupConnections();

    setVolume( 50 );
}

AudioEngine::~AudioEngine()
{
}

void AudioEngine::setupConnections()
{
    connect( m_mediaPlayer, &QMediaPlayer::positionChanged, this, &AudioEngine::onPositionChanged );
    connect( m_mediaPlayer, &QMediaPlayer::mediaStatusChanged, this, &AudioEngine::onMediaStatusChanged );
    connect( m_mediaPlayer, &QMediaPlayer::sourceChanged, this, &AudioEngine::onSourceChanged );
    connect( m_mediaPlayer, &QMediaPlayer::seekableChanged, this, &AudioEngine::onSeekableChanged );
    connect( m_mediaPlayer, &QMediaPlayer::durationChanged, this, &AudioEngine::onTrackLengthChanged );

    // the player state is derived from these three
    connect( m_mediaPlayer, &QMediaPlayer::playbackStateChanged, this, &AudioEngine::updateState );
    connect( m_mediaPlayer, &QMediaPlayer::mediaStatusChanged, this, &AudioEngine::updateState );
    connect( m_mediaPlayer, &QMediaPlayer::errorOccurred, this, &AudioEngine::updateState );

    connect( m_audioOutput, &QAudioOutput::volumeChanged, this, &AudioEngine::onVolumeChanged );
    connect( m_audioOutput, &QAudioOutput::mutedChanged, this, &AudioEngine::onMutedChanged );
}

void AudioEngine::onPositionChanged( qint64 time )
{
    emit seeked( time, false );
}

void AudioEngine::onSourceChanged()
{
    emit sourceChanged();
}

void AudioEngine::onMediaStatusChanged( QMediaPlayer::MediaStatus status )
{
    if( status == QMediaPlayer::EndOfMedia )
        onFinished();
}

void AudioEngine::onFinished()
{
    if( m_currentTrack )
        m_currentTrack->setState( GrooveOff::PlayerState::Stopped );
    next();
}

/*!
  \brief updateState: translates the backend state into the application one.

  QtMultimedia splits what Phonon used to report as a single state across
  playbackState(), mediaStatus() and error(), so all three are folded back here.
*/
void AudioEngine::updateState()
{
    GrooveOff::PlayerState newState;

    if( m_mediaPlayer->error() != QMediaPlayer::NoError ) {
        qDebug() << "GrooveOff :: " << "Error: " << m_mediaPlayer->errorString();
        newState = GrooveOff::PlayerState::Error;
    } else if( m_mediaPlayer->mediaStatus() == QMediaPlayer::StalledMedia ) {
        newState = GrooveOff::PlayerState::Buffering;
    } else {
        switch( m_mediaPlayer->playbackState() ) {
            case QMediaPlayer::PlayingState:
                newState = GrooveOff::PlayerState::Playing;
                break;
            case QMediaPlayer::PausedState:
                newState = GrooveOff::PlayerState::Paused;
                break;
            case QMediaPlayer::StoppedState:
            default:
                newState = GrooveOff::PlayerState::Stopped;
                break;
        }
    }

    setState( newState );
}

void AudioEngine::setState( GrooveOff::PlayerState state )
{
    if( m_state == state )
        return;

    m_state = state;

    if( m_currentTrack )
        m_currentTrack->setState( state );

    emit stateChanged( state );
}

void AudioEngine::playItem( const PlaylistItemPtr &track)
{
    // this was added to handle triggers from playlist menu
    if(m_currentTrack == track) {
        m_currentTrack->isPlaying() ? pause() : play();
        return;
    }

    if( m_currentTrack )
        m_currentTrack->setState( GrooveOff::PlayerState::Stopped );

    m_currentTrack = track;
    m_oldTrack = track;
    m_mediaPlayer->setSource( QUrl::fromLocalFile( m_currentTrack->path() + QDir::separator() + m_currentTrack->fileName() ) );
    play();
}

void AudioEngine::playPause()
{
    isPlaying() ? pause() : play();
}

void AudioEngine::pause()
{
    m_mediaPlayer->pause();
}

void AudioEngine::play()
{
    if( m_mediaPlayer->source().isEmpty() ) {
        if( The::playlist()->count() > 0 ) {
            playItem( The::playlist()->item( 0 ) );
            return;
        }
    }
    m_mediaPlayer->play();
}

void AudioEngine::stop()
{
    m_mediaPlayer->stop();
}


bool AudioEngine::isPlaying() const
{
    return m_state == GrooveOff::PlayerState::Playing;
}

bool AudioEngine::isPaused() const
{
    return m_state == GrooveOff::PlayerState::Paused;
}

bool AudioEngine::isStopped() const
{
    return m_state == GrooveOff::PlayerState::Stopped;
}

GrooveOff::PlayerState AudioEngine::state() const
{
    return m_state;
}

qint64 AudioEngine::currentTime() const
{
    return m_mediaPlayer->position();
}

qint64 AudioEngine::currentTrackTotalTime() const
{
    return m_mediaPlayer->duration();
}

PlaylistItemPtr AudioEngine::currentTrack() const
{
    return m_currentTrack;
}

qint64 AudioEngine::remainingTime() const
{
    return m_mediaPlayer->duration() - m_mediaPlayer->position();
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
        playItem(track);
    }
}

void AudioEngine::previous()
{
    if( canGoPrevious() ) {
        PlaylistItemPtr track = The::playlist()->item( The::playlist()->row( m_currentTrack ) - 1 );
        playItem(track);
    }
}

void AudioEngine::removingTrack( const PlaylistItemPtr &track )
{
    if( m_currentTrack == track ) {
        emit removedPlayingTrack();
        m_mediaPlayer->stop();
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
        m_mediaPlayer->setPosition( ms );
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

/*!
  \brief setVolume: sets the output volume

  QAudioOutput works on a linear scale while the volume we store and display is
  perceptual, hence the conversion: without it the dial would feel wrong at the
  low end and volumes saved by previous versions would play back louder.
*/
void AudioEngine::setVolume( int percentage )
{
    percentage = qBound( 0, percentage, 100 );
    m_volume = percentage;

    const float volume = QtAudio::convertVolume( percentage / 100.0f,
                                                 QtAudio::LogarithmicVolumeScale,
                                                 QtAudio::LinearVolumeScale );

    if ( !qFuzzyCompare( m_audioOutput->volume(), volume ) )
    {
        m_audioOutput->setVolume( volume );
        emit volumeChanged( percentage );
    }
}

void AudioEngine::onVolumeChanged( float volume )
{
    const float perceptual = QtAudio::convertVolume( volume,
                                                     QtAudio::LinearVolumeScale,
                                                     QtAudio::LogarithmicVolumeScale );
    int percent = qBound( 0, qRound( perceptual * 100 ), 100 );

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
    if( m_mediaPlayer )
        return m_mediaPlayer->isSeekable();
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
