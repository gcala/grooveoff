/*
    GrooveOff - Music streaming from torrents
    Copyright (C) 2026  Giuseppe Calà <jiveaxe@gmail.com>

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

#include "DownloadQueue.h"

#include "CacheManager.h"
#include "../grooveoff/Settings.h"

#include <QSettings>
#include <QTimer>

namespace The {
    static DownloadQueue* s_DownloadQueue_instance = nullptr;

    DownloadQueue* downloadQueue()
    {
        if( !s_DownloadQueue_instance )
            s_DownloadQueue_instance = new DownloadQueue();

        return s_DownloadQueue_instance;
    }
}

namespace {

/** how often the room left in the cache is looked at */
const int roomIntervalMs = 5000;

} // namespace

DownloadQueue::DownloadQueue()
    : QObject( nullptr )
    , m_roomTimer( new QTimer( this ) )
{
    reloadSettings();

    // A priority handed to a torrent the session is not holding goes nowhere,
    // and nothing repeats it — so a torrent that has just been let in is asked
    // again what its files are worth.
    //
    // Measured, this changes nothing today: waking a torrent brings its
    // metadata, the window rebuilds the sidebar when they arrive, and that
    // hands the selection here again. But it does so for its own reasons, and
    // a queue whose priorities are right because somebody else redraws a list
    // is a queue that is right by accident. This is the class keeping its own
    // promise.
    //
    // Both signals and not one: a torrent reopened from its resume file
    // arrives with its file list in hand and never sends the second, while a
    // magnet sends the first with nothing behind it yet.
    connect( The::torrentSession(), &TorrentSession::torrentAdded, this,
             [this]( const QString & ) { refresh(); } );

    connect( The::torrentSession(), &TorrentSession::metadataReady, this,
             [this]( const QString &, const QString &, const QList<TrackInfoPtr> & ) { refresh(); } );

    // Asked once here rather than assumed, because what this remembers is used
    // to spot the moment the answer changes: starting out believing there is
    // room when there is none means the first change — the one that matters,
    // from no room to room — looks like no change at all.
    m_hadRoom = The::cacheManager()->hasRoomForPrefetch();

    connect( m_roomTimer, &QTimer::timeout, this, &DownloadQueue::checkCacheRoom );
    m_roomTimer->start( roomIntervalMs );
}

DownloadQueue::~DownloadQueue() = default;

void DownloadQueue::setSelection( const QString &infoHash, const QList<int> &fileIndexes )
{
    if( infoHash.isEmpty() )
        return;

    m_selections.insert( infoHash, fileIndexes );
    refresh();
}

void DownloadQueue::forget( const QString &infoHash )
{
    m_selections.remove( infoHash );

    if( m_playingHash == infoHash )
        clearPlaying();
}

void DownloadQueue::forgetAll()
{
    m_selections.clear();
}

void DownloadQueue::setPlaying( const QString &infoHash, int fileIndex )
{
    if( m_playingHash == infoHash && m_playingFile == fileIndex )
        return;

    m_playingHash = infoHash;
    m_playingFile = fileIndex;

    refresh();
}

void DownloadQueue::clearPlaying()
{
    if( m_playingHash.isEmpty() && m_playingFile < 0 )
        return;

    m_playingHash.clear();
    m_playingFile = -1;

    refresh();
}

bool DownloadQueue::isEnabled() const
{
    return m_enabled;
}

void DownloadQueue::reloadSettings()
{
    QSettings settings;

    const bool wasEnabled = m_enabled;
    m_enabled = settings.value( Settings::downloadAhead, Settings::downloadAheadDefault ).toBool();

    // Switching it on is one of the two moments at which a sleeping torrent
    // gains something to do. Nothing else notices it: refresh() hands the new
    // priorities to the torrents the session is holding, and the ones it is not
    // holding are precisely the ones that would have to be woken to act on them.
    if( m_enabled && !wasEnabled )
        emit fetchingResumed();
}

int DownloadQueue::positionOf( const QString &infoHash, int fileIndex ) const
{
    const auto selection = m_selections.constFind( infoHash );
    if( selection == m_selections.constEnd() )
        return -1;

    return selection.value().indexOf( fileIndex );
}

TorrentSession::FilePriority DownloadQueue::priorityFor( const QString &infoHash, int fileIndex ) const
{
    // The one being listened to, always and whatever else is true. It comes
    // before the question of whether the file is in a playlist at all: a track
    // can be played out of a custom playlist made of several torrents, and the
    // queue of its own torrent knows nothing about that.
    if( infoHash == m_playingHash && fileIndex == m_playingFile )
        return TorrentSession::TopFile;

    const int position = positionOf( infoHash, fileIndex );
    if( position < 0 )
        return TorrentSession::SkipFile;

    // Switched off, this is the application it used to be: a track is fetched
    // when somebody asks to hear it, and not before.
    if( !m_enabled )
        return TorrentSession::SkipFile;

    // And the brake. Stopping is the whole of it — the alternative would be
    // throwing away a torrent somebody listened to yesterday to make room for
    // one nobody has asked for yet, which is the worst trade in the program.
    if( !The::cacheManager()->hasRoomForPrefetch() )
        return TorrentSession::SkipFile;

    // Everything from the cursor forward comes first, in order; the holes left
    // behind by a jump forward are fetched after them rather than instead of
    // them, because what follows what is playing is what will be wanted next.
    const int cursor = m_playingHash == infoHash ? positionOf( infoHash, m_playingFile ) : -1;

    return position > cursor ? TorrentSession::NormalFile : TorrentSession::QueuedFile;
}

void DownloadQueue::refresh()
{
    TorrentSession *session = The::torrentSession();

    for( auto selection = m_selections.constBegin(); selection != m_selections.constEnd(); ++selection ) {
        const QString &infoHash = selection.key();

        for( int fileIndex : selection.value() )
            session->setFilePriority( infoHash, fileIndex, priorityFor( infoHash, fileIndex ) );
    }

    // A track played out of a custom playlist belongs to a torrent that may
    // have no queue of its own here, and it still has to be fetched.
    if( !m_playingHash.isEmpty() && !m_selections.contains( m_playingHash ) )
        session->setFilePriority( m_playingHash, m_playingFile, TorrentSession::TopFile );

    // Nothing is said here about which torrent is served first when several
    // are fetching at once, and it was tried: libtorrent's per-torrent
    // bandwidth priority is deprecated and compiled out of this build. It
    // turns out not to be wanted. What protects the track being listened to
    // is not a priority but a clock — TorrentTrack puts short deadlines on the
    // pieces ahead of the reader, and a deadline is answered before work that
    // has none, whichever torrent it belongs to. The queue deliberately sets
    // no deadlines at all: it is meant to use the line that is left over.
}

void DownloadQueue::checkCacheRoom()
{
    const bool hasRoom = The::cacheManager()->hasRoomForPrefetch();
    if( hasRoom == m_hadRoom )
        return;

    m_hadRoom = hasRoom;
    refresh();

    // The other one. Room comes back on its own — an eviction, a track thrown
    // out of a playlist, a ceiling raised in the settings — so there is no
    // gesture to hang this on and nobody else is watching for it.
    if( hasRoom )
        emit fetchingResumed();
}
