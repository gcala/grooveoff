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
     * The context in which DownloadItem can be found
     */
    enum Context {
         Download, // when DownloadItem is used in DownloadList
         Session,  // when DownloadItem is used as Session item
         Track     // when DownloadItem is used as track item in SessionManager
    };

    /**
     * The state of song download
     */
    enum DownloadState {
        /**
         * The initial state.
         */
         QueuedState,
         /**
          * Download in progress.
          */
         DownloadingState,
         /**
          * Download finished successfully.
          */
         FinishedState,
         /**
          * Download was aborted or error occurred.
          */
         AbortedState,
         /**
          * Something goes wrong during download
          */
         ErrorState
    };

    /**
     * The network operation
     */
    enum NetworkJob {
         /**
          * Retrieval of site token
          */
         TokenJob,
         /**
          * Search for a given text
          */
         SearchJob,
         /**
          * Get key
          */
         KeyJob,
         /**
          * Get song
          */
         SongJob
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
}

#include <QtCore/QMetaType>

Q_DECLARE_METATYPE( GrooveOff::Context )
Q_DECLARE_METATYPE( GrooveOff::DownloadState )
Q_DECLARE_METATYPE( GrooveOff::NetworkJob )

QT_END_HEADER

#endif // GROOVEOFFNAMESPACE_H
