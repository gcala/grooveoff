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


#ifndef CATALOG_H
#define CATALOG_H

#include "../grooveoff/TrackInfo.h"

#include <QList>
#include <QString>

class Catalog;

namespace The {
    Catalog* catalog();
}

/**
  * @class Catalog
  * @short What torrents and tracks are known, and what becomes of them.
  *
  * Two removals live here and must never be confused: evicting from the cache
  * throws away bytes and keeps the catalogue, removing from the catalogue is
  * what the user asked for and takes the rows with it. Mixing them up would
  * delete tracks that custom playlists still point at.
  */
class Catalog
{
public:
    friend Catalog* The::catalog();

    ~Catalog();

    /** a custom playlist holding tracks of a given torrent */
    struct PlaylistReference
    {
        qint64  playlistId = 0;
        QString name;
        int     trackCount = 0;
    };

    /** a torrent holding data in the cache */
    struct CacheEntry
    {
        QString infoHash;
        qint64  bytesDone = 0;
    };

    // --- torrents -----------------------------------------------------------

    /**
     * Records a torrent and creates its own playlist. Adding a torrent that is
     * already known leaves its rows alone and only brings it back into sight if
     * it was hidden, so that re-adding a magnet restores the previous selection
     * and exclusions instead of starting over.
     */
    bool addTorrent( const QString &infoHash, const QString &magnetUri, const QString &name );

    /** a magnet whose track list has never arrived: all that is known of it */
    struct PendingMagnet
    {
        QString infoHash;
        QString magnetUri;
        QString name;      /**< from the magnet's own dn field */
    };

    /**
     * Records a magnet and **no playlist**, which is the whole difference from
     * addTorrent(): there is nothing to make a playlist of yet. It is what
     * makes the waiting outlive the program — somebody asked to go on looking,
     * and that is a decision worth keeping rather than a torrent worth showing.
     *
     * A torrent with no playlist is exactly one whose metadata never came, and
     * addTorrent() repairs that the moment they do.
     */
    bool rememberMagnet( const QString &infoHash, const QString &magnetUri, const QString &name );

    /** those magnets, to be looked for again at the next start */
    QList<PendingMagnet> magnetsWithoutMetadata() const;

    bool isKnown( const QString &infoHash ) const;
    bool isHidden( const QString &infoHash ) const;
    QString magnetUri( const QString &infoHash ) const;
    QString name( const QString &infoHash ) const;

    bool setName( const QString &infoHash, const QString &name );

    /** marks the torrent as used now, which is what the eviction order reads */
    bool touch( const QString &infoHash );

    bool setBytesDone( const QString &infoHash, qint64 bytes );

    /**
     * What this torrent has in the cache, as the last statistics said. Read at
     * start up to tell a torrent that still has music to fetch from one that
     * has it all already: the first is worth waking, the second can go on
     * sleeping until somebody presses play on it.
     */
    qint64 bytesDone( const QString &infoHash ) const;
    bool setHasResume( const QString &infoHash, bool hasResume );
    bool hasResume( const QString &infoHash ) const;

    /** brings a hidden torrent back into sight, with its selection intact */
    bool unhideTorrent( const QString &infoHash );

    /** every torrent that should be reopened at start up, hidden ones included */
    QStringList knownTorrents() const;

    // --- tracks -------------------------------------------------------------

    /**
     * Stores every file of a torrent, audio or not, and fills the id of each
     * TrackInfo. Files already known are left untouched, so their exclusions
     * survive a second look at the same torrent.
     */
    bool addTracks( const QList<TrackInfoPtr> &tracks );

    QList<TrackInfoPtr> tracks( const QString &infoHash,
                                bool audioOnly = true,
                                bool includeExcluded = false ) const;

    TrackInfoPtr track( qint64 trackId ) const;
    TrackInfoPtr track( const QString &infoHash, int fileIndex ) const;

    bool setExcluded( qint64 trackId, bool excluded );

    /** writes back the tags read from the file once its first pieces arrived */
    bool updateTags( const TrackInfoPtr &track );

    // --- removal ------------------------------------------------------------

    /**
     * Which custom playlists still hold tracks of this torrent. Empty means the
     * torrent can be removed after a plain yes or no; anything else means the
     * user has to be told what is at stake.
     */
    QList<PlaylistReference> customPlaylistReferences( const QString &infoHash ) const;

    /** removes torrent, tracks and playlists; the caller deletes files and resume */
    bool removeTorrent( const QString &infoHash );

    /** keeps the rows but takes the torrent out of sight */
    bool hideTorrent( const QString &infoHash );

    /**
     * Removes a hidden torrent once nothing refers to it any more. Meant to be
     * called inside the same transaction as the removal that triggered it,
     * otherwise a crash in between leaves an invisible torrent holding cache
     * for ever.
     */
    bool collectGarbage( const QString &infoHash );

    /** every hidden torrent no custom playlist refers to; a hygiene check */
    QStringList orphanedHiddenTorrents() const;

    // --- cache --------------------------------------------------------------

    /** forgets the bytes without touching the catalogue */
    bool clearData( const QString &infoHash );

    /** oldest used first, skipping @p keepInfoHash and torrents holding nothing */
    QList<CacheEntry> evictionCandidates( const QString &keepInfoHash ) const;

    qint64 totalBytesDone() const;

private:
    Catalog();
    Q_DISABLE_COPY( Catalog )

    static TrackInfoPtr trackFromQuery( const class QSqlQuery &query );
};

#endif // CATALOG_H
