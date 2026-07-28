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


#ifndef GROOVEOFFNAMESPACE_H
#define GROOVEOFFNAMESPACE_H

namespace GrooveOff {
    /**
     * How far a track is from being playable.
     *
     * Replaces the old DownloadState: nothing is downloaded any more, a track
     * is buffered out of a torrent and thrown away again, so the states track
     * availability rather than a transfer.
     */
    enum TrackState {
         /**
          * Known, but its torrent has not been handed to the torrent session
          * yet. Most rows are in this state right after start up, because
          * torrents are reactivated lazily.
          */
         DormantState,
         /**
          * The torrent is active and nothing of this track is buffered yet.
          */
         IdleState,
         /**
          * Pieces are coming in.
          */
         BufferingState,
         /**
          * Enough is buffered to start playing.
          */
         ReadyState,
         /**
          * Buffering, but nothing has arrived for a while. Not an error: the
          * swarm may well wake up.
          */
         StalledState,
         /**
          * Something went wrong and waiting will not fix it.
          */
         ErrorState
    };

    /**
     * The state of the audio player.
     *
     * This is the application's own player state: only AudioEngine knows about
     * the multimedia framework underneath, everything else talks in these terms.
     * It is a scoped enum because DownloadState already defines an ErrorState.
     */
    enum class PlayerState {
         Stopped,
         Playing,
         Paused,
         Buffering,
         Error
    };

    /**
     * The player timer state
     */
    enum TimerState {
         /**
          * Elapsed time
          */
         ElapsedState,
         /**
          * Remaining time
          */
         RemainingState
    };

    enum MessageType {
         Error,
         Connected,
         Idle,
         Playing
    };

    /**
     * What a torrent's swarm looks like from here, for the led of its playlist.
     *
     * Three states and not two, and the third one is the whole reason this can
     * be shown at all: reactivation is lazy, so most torrents at start up have
     * never been handed to the session and there is nothing to count. Told with
     * two states they would all read as dead, and the sidebar would announce a
     * disaster at every launch.
     */
    enum SourceHealth {
         /** never asked: the torrent is not in the session, and will be at the first play */
         UnknownSource,
         /** asked, and nobody is there right now */
         SilentSource,
         /** peers, however many the tooltip says */
         LiveSource
    };
}

#include <QtCore/QMetaType>

Q_DECLARE_METATYPE( GrooveOff::TrackState )
Q_DECLARE_METATYPE( GrooveOff::PlayerState )

#endif // GROOVEOFFNAMESPACE_H
