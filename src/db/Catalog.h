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

    /** a custom playlist holding tracks of a given torrent, or of given tracks */
    struct PlaylistReference
    {
        qint64  playlistId = 0;
        QString name;
        int     trackCount = 0;

        // The one that is always there has no name of its own to report: the
        // interface says it in the language it is running in. Without this the
        // question naming the lists at stake prints the stored English.
        bool    pinned = false;
    };

    /** a torrent holding data in the cache */
    struct CacheEntry
    {
        QString infoHash;
        qint64  bytesDone = 0;
    };

    // --- torrents -----------------------------------------------------------

    /**
     * Records a torrent and creates its own playlist. Adding one that is
     * already known leaves its rows alone, so pasting the same magnet again
     * finds the file list already there instead of asking the swarm for it —
     * and gives back the playlist if it has none, which is how a magnet left
     * waiting and a torrent whose selector was closed both come back.
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

    /** every torrent that could be reopened at start up */
    QStringList knownTorrents() const;

    // --- tracks -------------------------------------------------------------

    /**
     * Stores every file of a torrent, audio or not, and fills the id of each
     * TrackInfo. Files already known are left untouched, so a second look at
     * the same torrent does not renumber anything a playlist points at.
     */
    bool addTracks( const QList<TrackInfoPtr> &tracks );

    QList<TrackInfoPtr> tracks( const QString &infoHash, bool audioOnly = true ) const;

    TrackInfoPtr track( qint64 trackId ) const;
    TrackInfoPtr track( const QString &infoHash, int fileIndex ) const;

    /** writes back the tags read from the file once its first pieces arrived */
    bool updateTags( const TrackInfoPtr &track );

    // --- removal ------------------------------------------------------------

    /**
     * Which custom playlists still hold tracks of this torrent. Empty means the
     * torrent can be removed after a plain yes or no; anything else means the
     * user has to be told what is at stake.
     */
    QList<PlaylistReference> customPlaylistReferences( const QString &infoHash ) const;

    /**
     * Which lists made by hand hold any of these tracks, and how many of them
     * each one holds. The question every removal of a track asks before it
     * destroys anything, and it is about tracks where the one above is about a
     * whole torrent — that one still answers for «Remove playlist…».
     *
     * The list that is always there answers like any other, because it is one:
     * a track dropped into it was put there by hand and is exactly as much at
     * stake as one in a list somebody named.
     */
    QList<PlaylistReference> customPlaylistsHolding( const QList<qint64> &trackIds ) const;

    /**
     * The tracks of this torrent that a list made by hand still holds. What
     * «remove only what nothing else uses» keeps, and what the torrent's own
     * playlist is left holding afterwards.
     */
    QList<qint64> tracksInCustomPlaylists( const QString &infoHash ) const;

    /** removes torrent, tracks and playlists; the caller deletes files and resume */
    bool removeTorrent( const QString &infoHash );


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
