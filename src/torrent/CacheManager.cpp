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


#include "CacheManager.h"
#include "TorrentSession.h"

#include "../db/Catalog.h"
#include "../grooveoff/Settings.h"

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QSettings>
#include <QTimer>

namespace {
    /** how often the cache is measured while the application runs */
    constexpr int checkIntervalMs = 10000;

    /**
     * How close to the ceiling the cache may get before speculative bytes stop
     * being fetched. Held back a little early on purpose: reaching the ceiling
     * and only then reacting means the prefetch has already caused the eviction
     * it was supposed to avoid.
     */
    constexpr double prefetchHeadroom = 0.9;
}

namespace The {
    static CacheManager* s_CacheManager_instance = nullptr;

    CacheManager* cacheManager()
    {
        if( !s_CacheManager_instance )
            s_CacheManager_instance = new CacheManager();

        return s_CacheManager_instance;
    }
}

CacheManager::CacheManager()
    : QObject()
    , m_timer( new QTimer( this ) )
{
    m_timer->setInterval( checkIntervalMs );
    connect( m_timer, &QTimer::timeout, this, &CacheManager::enforceLimit );
    m_timer->start();
}

CacheManager::~CacheManager() = default;

qint64 CacheManager::limit() const
{
    QSettings settings;
    const qint64 megabytes = settings.value( Settings::cacheSize, Settings::cacheSizeDefault ).toLongLong();

    return megabytes > 0 ? megabytes * 1024 * 1024 : 0;
}

qint64 CacheManager::usage() const
{
    return The::catalog()->totalBytesDone();
}

bool CacheManager::hasRoomForPrefetch() const
{
    const qint64 ceiling = limit();
    if( ceiling <= 0 )
        return true;

    return usage() < static_cast<qint64>( ceiling * prefetchHeadroom );
}

void CacheManager::setPlayingTorrent( const QString &infoHash )
{
    m_playing = infoHash;
}

void CacheManager::setPrefetchedTorrent( const QString &infoHash )
{
    m_prefetched = infoHash;
}

void CacheManager::enforceLimit()
{
    const qint64 ceiling = limit();
    if( ceiling <= 0 )
        return;

    qint64 used = usage();
    if( used <= ceiling )
        return;

    // Oldest first, and the one being listened to is already out of the list.
    const QList<Catalog::CacheEntry> candidates = The::catalog()->evictionCandidates( m_playing );

    for( const Catalog::CacheEntry &entry : candidates ) {
        if( used <= ceiling )
            break;

        if( entry.infoHash == m_prefetched )
            continue;

        if( evict( entry.infoHash ) )
            used -= entry.bytesDone;
    }

    // A single torrent larger than the whole ceiling ends up here, and there is
    // nothing to do about it: what is playing is never evicted, so the cache
    // stays over its limit until the listening moves elsewhere. Saying so is
    // better than looping over a list that cannot help.
    if( used > ceiling )
        qDebug() << "GrooveOff ::" << "the cache is over its limit and only what is playing is left";
}

void CacheManager::emptyCache()
{
    // Everything but the torrent being listened to, which would stop the music
    // to free bytes the very next second would ask for again. Emptying with
    // nothing playing — which is what happens on the way out — spares nothing.
    const QList<Catalog::CacheEntry> entries = The::catalog()->evictionCandidates( m_playing );

    for( const Catalog::CacheEntry &entry : entries )
        evict( entry.infoHash );
}

bool CacheManager::evict( const QString &infoHash )
{
    if( infoHash.isEmpty() )
        return false;

    TorrentSession *session = The::torrentSession();

    // Two ways for the bytes to go, and which one applies depends on whether
    // the session holds the torrent. libtorrent deletes the files of a torrent
    // it knows, on its own thread; one that was never reopened after start up
    // is a directory nobody but us would ever remove.
    const bool awake = session->isActive( infoHash );

    session->remove( infoHash, true );

    if( !awake ) {
        QDir directory( QDir( session->torrentsPath() ).absoluteFilePath( infoHash ) );
        if( directory.exists() && !directory.removeRecursively() )
            qDebug() << "GrooveOff ::" << "could not delete the cache directory of" << infoHash;

        // Belt and braces: the session skips this when it is not running, and a
        // resume blob whose data are gone makes libtorrent recheck files that
        // do not exist any more.
        QFile::remove( QDir( session->resumePath() ).absoluteFilePath( infoHash + QLatin1String( ".resume" ) ) );
    }

    // clearData, never removeTorrent. The bytes are the cache, the rows are the
    // catalogue, and a track the user put in a playlist has to survive the disk
    // filling up — otherwise a cache running out deletes somebody's music.
    if( !The::catalog()->clearData( infoHash ) )
        return false;

    if( infoHash == m_prefetched )
        m_prefetched.clear();

    emit evicted( infoHash );

    return true;
}
