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


#ifndef CACHEMANAGER_H
#define CACHEMANAGER_H

#include <QObject>
#include <QString>

class QTimer;

class CacheManager;

namespace The {
    CacheManager* cacheManager();
}

/**
  * @class CacheManager
  * @short How much of the disk the cache is allowed, and what goes when it is
  *        over.
  *
  * There is one distinction here that everything else depends on, and getting
  * it wrong is the worst thing this project can do to somebody's data:
  * **evicting is not removing**. Eviction throws away bytes — the sparse files
  * and the resume blob — and leaves every row of the catalogue where it is, so
  * a track sitting in a custom playlist goes back to Dormant and is fetched
  * again at the next play. Removal is what the user asked for and takes the
  * rows with it. This class only ever evicts; the word remove does not appear
  * in it except to say so.
  *
  * The order is least recently used first, read straight out of the catalogue,
  * and the torrent being listened to is never a candidate however old it is.
  */
class CacheManager : public QObject
{
    Q_OBJECT

public:
    friend CacheManager* The::cacheManager();

    ~CacheManager() override;

    /** the ceiling in bytes, from the settings; 0 means there is none */
    qint64 limit() const;

    /**
     * What the cache holds, summed over the catalogue. It is the bytes
     * libtorrent reports as done, not the size the files claim: they are
     * sparse, and asking the filesystem would count holes as content.
     */
    qint64 usage() const;

    /**
     * Whether there is room for bytes nobody has asked to hear yet. Prefetched
     * bytes are speculative, so a cache close to its ceiling suspends the
     * prefetch instead of evicting something to make room for it — throwing
     * away a track that was listened to in favour of one that may never be is
     * a bad trade in every direction.
     */
    bool hasRoomForPrefetch() const;

    /** the torrent being listened to, which no eviction may touch */
    void setPlayingTorrent( const QString &infoHash );

    /** the torrent of the next track, spared as long as the prefetch runs */
    void setPrefetchedTorrent( const QString &infoHash );

public Q_SLOTS:
    /**
     * Evicts oldest first until the cache fits under its ceiling. Cheap when it
     * already does, which is the usual case, so calling it often costs nothing.
     */
    void enforceLimit();

    /** throws away everything the torrent being listened to does not need */
    void emptyCache();

    /** one torrent: its files and its resume blob go, its rows stay */
    bool evict( const QString &infoHash );

    /**
     * Some files of one torrent: their bytes go, every row of the catalogue
     * stays. The per-file counterpart of an eviction, and it lives here for the
     * same reason that one does — the bytes and the number saying how many
     * there are have to go together, and a caller made to remember the second
     * half would one day forget it.
     *
     * Never a DELETE, exactly like evict(): what was asked for is a track out
     * of a playlist, and the row saying the file exists is what lets the
     * selector offer it again.
     *
     * No evicted() either. That signal means a whole torrent's bytes went and
     * its rows should go back to Dormant; here the tracks that are left are as
     * playable as they were a moment ago.
     */
    bool discardFiles( const QString &infoHash, const QList<int> &fileIndexes );

Q_SIGNALS:
    /**
     * The bytes of this torrent are gone. Rows showing its tracks go back to
     * Dormant; nothing is in error and nothing is missing, so nobody is
     * interrupted about it.
     */
    void evicted( const QString &infoHash );

private:
    CacheManager();
    Q_DISABLE_COPY( CacheManager )

    QString m_playing;
    QString m_prefetched;

    /**
     * The cache grows while a track streams, so the check cannot only happen
     * when something is added. A timer keeps it away from the alert handling,
     * where a signal ordering would decide whether the catalogue had been
     * updated yet.
     */
    QTimer *m_timer;
};

#endif // CACHEMANAGER_H
