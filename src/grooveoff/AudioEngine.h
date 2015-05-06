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


#ifndef AUDIOENGINE_H
#define AUDIOENGINE_H

#include "PlaylistItem.h"

#include <QObject>
#include <phonon/MediaObject>
#include <phonon/AudioOutput>
#include <phonon/AudioDataOutput>

class AudioEngine;

namespace The {
    AudioEngine* audioEngine();
}

class AudioEngine : public QObject
{
    Q_OBJECT
public:
    friend AudioEngine* The::audioEngine();
    ~AudioEngine();

    void playPause();
    void play();
    void pause();
    void stop( Phonon::ErrorType errorCode = Phonon::NoError );

    Phonon::State state() const;
    bool isPlaying() const;
    bool isPaused() const;
    bool isStopped() const;

    qint64 currentTime() const;
    qint64 currentTrackTotalTime() const;
    qint64 remainingTime() const;

    PlaylistItemPtr currentTrack() const;

    void previous();
    void next();
    bool canGoPrevious();
    bool canGoNext();
    void seek( qint64 ms );
    void seek( int ms ); // for compatibility with seekbar in audiocontrols
    void playItem(const PlaylistItemPtr &track);
    void removingTrack(const PlaylistItemPtr &track);
    inline Phonon::MediaObject * mediaObject() const {
        return m_mediaObject;
        
    }
    bool isSeekable() const;

    /**
     * @return @c true if sound output is disabled, @false otherwise
     */
    bool isMuted() const;

    /**
     * Gets the volume
     * @return the volume as a percentage
     */
    int volume() const;

public Q_SLOTS:
    void setVolume( int percentage );
    void setMuted(bool);

Q_SIGNALS:
    /*
     * Called when audio track changes
     */
    void sourceChanged();
    
    /*
     * Called when slider moved to new position
     */
    void seeked( qint64 ms, bool userSeek );
    
    /*
     * Called when player state changes
     */
    void stateChanged( Phonon::State );
    
    /*
     * Called when volume changes
     */
    void volumeChanged( int volume /* in percent */ );
    
    /*
     * Called when the current playing track is removed 
     */
    void removedPlayingTrack();

    /**
     * Called when audio output was enabled or disabled
     *
     * NB: if setMute() was called on the engine controller, but it didn't change the
     * mute state, this will not be called
     */
    void muteStateChanged( bool mute );

    /**
     * Called when the seekable value was changed
     */
    void seekableChanged( bool seekable );

    /**
       Called when the track length changes, typically because the track has changed but
       also when phonon manages to determine the full track length.
     */
    void trackLengthChanged( qint64 milliseconds );

private Q_SLOTS:
    void timerTriggered( qint64 time );
    void onStateChanged( Phonon::State newState, Phonon::State oldState );
    void sourceChanged( Phonon::MediaSource );
    void onFinished();

    /**
     * For volume/mute changes from the phonon side
     */
    void onVolumeChanged( qreal volume );
    void onMutedChanged( bool );

    void onSeekableChanged( bool );
    void onTrackLengthChanged( qint64 );

private:
    AudioEngine();

    PlaylistItemPtr m_currentTrack;
    PlaylistItemPtr m_oldTrack;
    Phonon::State   m_state;

    Phonon::MediaObject* m_mediaObject;
    Phonon::AudioOutput* m_audioOutput;

    int m_volume;
    
    void setupConnections();
};

#endif // AUDIOENGINE_H
