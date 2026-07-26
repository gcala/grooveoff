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

#include "GrooveOffNamespace.h"
#include "PlaylistItem.h"

#include <QObject>
#include <QMediaPlayer>

class QAudioOutput;

class AudioEngine;

namespace The {
    AudioEngine* audioEngine();
}

/**
  * @class AudioEngine
  * @short The audio playback engine.
  *
  * This is the only class aware of the multimedia framework in use: it exposes
  * playback through GrooveOff::PlayerState so that the rest of the application
  * never depends on QtMultimedia types.
  */
class AudioEngine : public QObject
{
    Q_OBJECT
public:
    friend AudioEngine* The::audioEngine();
    ~AudioEngine();

    void playPause();
    void play();
    void pause();
    void stop();

    GrooveOff::PlayerState state() const;
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
    void stateChanged( GrooveOff::PlayerState );

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
       also when the backend manages to determine the full track length.
     */
    void trackLengthChanged( qint64 milliseconds );

private Q_SLOTS:
    void onPositionChanged( qint64 time );
    void onMediaStatusChanged( QMediaPlayer::MediaStatus status );
    void onSourceChanged();
    void onFinished();

    /**
     * Recomputes the player state out of playback state, media status and errors
     */
    void updateState();

    /**
     * For volume/mute changes coming from the audio output
     */
    void onVolumeChanged( float volume );
    void onMutedChanged( bool );

    void onSeekableChanged( bool );
    void onTrackLengthChanged( qint64 );

private:
    AudioEngine();

    void setState( GrooveOff::PlayerState state );

    /**
     * Points the player at the local stream server. Fails when the torrent is
     * not in the session yet, which is the normal state of most rows after a
     * start up.
     */
    bool openStream( const PlaylistItemPtr &track );

    /** hands the torrent of @p track to the session and remembers to play it */
    void wakeTorrentFor( const PlaylistItemPtr &track );

    /**
     * Once the current track is past its middle, brings up the beginning of
     * the next one with whatever bandwidth is left over. What this mostly
     * hides is not the transfer: it is the tens of seconds it takes to resolve
     * a magnet and find peers, which is why it is worth doing even when the
     * next track lives in another torrent.
     */
    void maybePrefetchNext();

    PlaylistItemPtr        m_currentTrack;
    PlaylistItemPtr        m_oldTrack;

    /** what to play as soon as the file list of its torrent comes back */
    PlaylistItemPtr        m_pendingTrack;

    /** what is being taken in the background, so that it is asked for once */
    PlaylistItemPtr        m_prefetchingTrack;

    GrooveOff::PlayerState m_state;

    QMediaPlayer* m_mediaPlayer;
    QAudioOutput* m_audioOutput;

    int m_volume;

    void setupConnections();
};

#endif // AUDIOENGINE_H
