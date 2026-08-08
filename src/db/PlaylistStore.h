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


#ifndef PLAYLISTSTORE_H
#define PLAYLISTSTORE_H

#include "../grooveoff/TrackInfo.h"

#include <QList>
#include <QString>

class PlaylistStore;

namespace The {
    PlaylistStore* playlistStore();
}

/**
  * @class PlaylistStore
  * @short Ordered lists of tracks, of both kinds, over the shared catalogue.
  *
  * A playlist born from a torrent and one built by hand out of ten different
  * torrents are the same thing structurally: an ordered list of track ids. Only
  * where they come from differs, which is what kind says. One table means one
  * implementation of reordering, renaming and playback.
  *
  * There is no separate queue: what plays is the open playlist, and which one
  * that is lives in app_state together with the track and the position.
  */
class PlaylistStore
{
public:
    friend PlaylistStore* The::playlistStore();

    ~PlaylistStore();

    enum Kind {
        TorrentPlaylist = 0,
        CustomPlaylist  = 1
    };

    struct PlaylistInfo
    {
        qint64  id = 0;
        QString name;
        Kind    kind = CustomPlaylist;
        QString infoHash;      /**< empty for a custom playlist */
        int     trackCount = 0;

        /**
         * The one that is always there: it cannot be removed or renamed, and
         * the interface shows a name of its own rather than the stored one, so
         * that it follows the language the program is running in.
         */
        bool pinned = false;

        bool isValid() const { return id > 0; }
    };

    // --- playlists ----------------------------------------------------------

    /** the playlist that belongs to a torrent; called by Catalog::addTorrent */
    qint64 createTorrentPlaylist( const QString &infoHash, const QString &name );

    qint64 createCustomPlaylist( const QString &name );

    bool rename( qint64 playlistId, const QString &name );

    /** drops a custom playlist; the tracks stay in the catalogue and in its torrent */
    bool removeCustomPlaylist( qint64 playlistId );

    /** takes every track out of a playlist made by hand and leaves the playlist */
    bool clearPlaylist( qint64 playlistId );

    /** the playlist that is always there, created by the migration */
    qint64 pinnedPlaylistId() const;

    /** every playlist there is; nothing is hidden any more */
    QList<PlaylistInfo> visiblePlaylists() const;

    PlaylistInfo playlist( qint64 playlistId ) const;
    qint64 torrentPlaylistId( const QString &infoHash ) const;

    // --- contents -----------------------------------------------------------

    QList<TrackInfoPtr> tracks( qint64 playlistId ) const;

    bool appendTracks( qint64 playlistId, const QList<TrackInfoPtr> &tracks );

    /** replaces the whole content, which is what the file selector does */
    bool setTracks( qint64 playlistId, const QList<TrackInfoPtr> &tracks );

    /** whether this playlist already holds this track, which it can only once */
    bool contains( qint64 playlistId, qint64 trackId ) const;

    bool removeTrackAt( qint64 playlistId, int position );
    bool moveTrack( qint64 playlistId, int from, int to );

    /**
     * Takes these tracks out of every list of both kinds. What removing a track
     * from a torrent's playlist means: that playlist is the list of what was
     * chosen, so a track nobody chose has no business sitting in a list made
     * out of it either.
     *
     * The rows in track stay, and that is the whole of the state afterwards —
     * the file is known and not chosen, which is where every file starts. There
     * is no third state and nothing is remembered about the refusal.
     *
     * One transaction, and every list touched is renumbered from zero, which
     * the arithmetic of a drag depends on.
     */
    bool removeTracksEverywhere( const QList<qint64> &trackIds );

    /** takes these tracks out of this list only, and leaves them everywhere else */
    bool removeTracks( qint64 playlistId, const QList<qint64> &trackIds );

    /**
     * Drops the playlist a torrent came with, keeping the torrent and its file
     * list. What a selector closed with nothing chosen means: there is nothing
     * to show, and the file list is worth keeping so that the same magnet
     * pasted again opens the selector instead of asking the swarm all over.
     *
     * The torrent is then known, has tracks and has no playlist — which is why
     * Catalog::magnetsWithoutMetadata() asks for no tracks as well as no
     * playlist, or the next start up would go looking for it as if the metadata
     * had never arrived and put the declined torrent back in the sidebar.
     */
    bool removeTorrentPlaylist( const QString &infoHash );

    // --- what was playing ---------------------------------------------------

    qint64 currentPlaylistId() const;
    qint64 currentTrackId() const;
    qint64 currentPositionMs() const;

    bool setPlaybackState( qint64 playlistId, qint64 trackId, qint64 positionMs );

private:
    PlaylistStore();
    Q_DISABLE_COPY( PlaylistStore )

    /** whether a row with this id is still in that table */
    bool exists( const QString &table, qint64 id ) const;

    bool isCustom( qint64 playlistId ) const;
    /** the contents of a playlist, in order, with any repeat dropped */
    bool writeTrackIds( qint64 playlistId, const QList<qint64> &requested );
    QList<qint64> trackIds( qint64 playlistId ) const;
    bool touch( qint64 playlistId );
};

#endif // PLAYLISTSTORE_H
