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
    void playItem(PlaylistItemPtr track);
    void removingTrack(PlaylistItemPtr track);
    Phonon::MediaObject * mediaObject() { return mediaObject_; }
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
    void loading();
    void started();
    void finished();
    void stopped();
    void paused();
    void resumed();
    void sourceChanged();
    void stopAfterTrackChanged();
    void seeked( qint64 ms, bool userSeek );
    void shuffleModeChanged( bool enabled );
    void repeatModeChanged();
    void controlStateChanged();
    void stateChanged( Phonon::State );
    void volumeChanged( int volume /* in percent */ );

    void timerMilliSeconds( qint64 msElapsed );
    void timerSeconds( unsigned int secondsElapsed );
    void timerPercentage( unsigned int percentage );

    void playlistChanged();
    void currentTrackPlaylistChanged();

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
    void sourceChanged(Phonon::MediaSource);
    void onFinished();

    /**
     * For volume/mute changes from the phonon side
     */
    void onVolumeChanged( qreal volume );
    void slotMutedChanged( bool );

    void slotSeekableChanged( bool );
    void slotTrackLengthChanged( qint64 );

private:
    AudioEngine();

    PlaylistItemPtr currentTrack_;
    PlaylistItemPtr oldTrack_;
    Phonon::State state_;

    Phonon::MediaObject* mediaObject_;
    Phonon::AudioOutput* audioOutput_;

    int m_volume;
};

#endif // AUDIOENGINE_H