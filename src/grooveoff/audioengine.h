/*
 * GrooveOff - Offline Grooveshark.com music
 * Copyright (C) 2013  Giuseppe Calà <jiveaxe@gmail.com>
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

#ifndef AUDIOENGINE_H
#define AUDIOENGINE_H

#include "playlistitem.h"

#include <QObject>
#include <phonon/MediaObject>
#include <phonon/AudioOutput>
#include <phonon/AudioDataOutput>

class AudioEngine : public QObject
{
    Q_OBJECT
public:
    static AudioEngine * instance();
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
    bool canSeek();
    void seek( qint64 ms );
    void seek( int ms ); // for compatibility with seekbar in audiocontrols
//     void lowerVolume();
//     void raiseVolume();
    void playItem(PlaylistItemPtr track);
    void removingTrack(PlaylistItemPtr track);
//     void setPlaylist();
//     void setQueue();
//     void setStopAfterTrack();
//     void setRepeatMode();
//     void setShuffled( bool enabled );
    Phonon::MediaObject * mediaObject() { return mediaObject_; }

signals:
    void loading();
    void started();
    void finished();
    void stopped();
    void paused();
    void resumed();
    void sourceChanged();

    //void audioDataReady( QMap< AudioEngine::AudioChannel, QVector<qint16> > data );
    void stopAfterTrackChanged();

    void seeked( qint64 ms );

    void shuffleModeChanged( bool enabled );
    void repeatModeChanged();
    void controlStateChanged();
//    void stateChanged( AudioState newState, AudioState oldState );
    void stateChanged( Phonon::State, Phonon::State );
    void volumeChanged( int volume /* in percent */ );

    void timerMilliSeconds( qint64 msElapsed );
    void timerSeconds( unsigned int secondsElapsed );
    void timerPercentage( unsigned int percentage );

    void playlistChanged();
    void currentTrackPlaylistChanged();

    void removedPlayingTrack();

private slots:
    void timerTriggered( qint64 time );
    void onStateChanged( Phonon::State newState, Phonon::State oldState );
    void sourceChanged(Phonon::MediaSource);
    void onFinished();
    void onVolumeChanged( qreal volume );
    void setVolume( int percentage );
    void setMuted(bool);

private:
    static bool instanceFlag;
    static AudioEngine *audioEngine;
    AudioEngine();

    PlaylistItemPtr currentTrack_;
    PlaylistItemPtr oldTrack_;
    Phonon::State state_;

    Phonon::MediaObject* mediaObject_;
    Phonon::AudioOutput* audioOutput_;
    Phonon::Path audioPath_;
};

#endif // AUDIOENGINE_H
