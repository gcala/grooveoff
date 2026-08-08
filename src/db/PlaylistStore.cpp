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


#include "PlaylistStore.h"
#include "Catalog.h"
#include "Database.h"

#include <QDateTime>
#include <QDebug>
#include <QSet>
#include <QSqlError>
#include <QSqlQuery>

#include <utility>

namespace The {
    static PlaylistStore* s_PlaylistStore_instance = nullptr;

    PlaylistStore* playlistStore()
    {
        if( !s_PlaylistStore_instance )
            s_PlaylistStore_instance = new PlaylistStore();

        return s_PlaylistStore_instance;
    }
}

namespace {

qint64 now()
{
    return QDateTime::currentSecsSinceEpoch();
}

bool reportFailure( const QSqlQuery &query )
{
    qWarning() << "GrooveOff ::" << "playlist query failed:" << query.lastError().text()
               << "in" << query.lastQuery();
    return false;
}

} // namespace

PlaylistStore::PlaylistStore()
{
}

PlaylistStore::~PlaylistStore()
{
}

// --- playlists --------------------------------------------------------------

qint64 PlaylistStore::createTorrentPlaylist( const QString &infoHash, const QString &name )
{
    const qint64 existing = torrentPlaylistId( infoHash );
    if( existing > 0 )
        return existing;

    QSqlQuery query( The::database()->connection() );
    query.prepare( QStringLiteral( "INSERT INTO playlist ( name, kind, info_hash, created_at, updated_at )"
                                   " VALUES ( :name, :kind, :hash, :now, :now )" ) );
    query.bindValue( QStringLiteral( ":name" ), name.isEmpty() ? infoHash : name );
    query.bindValue( QStringLiteral( ":kind" ), int( TorrentPlaylist ) );
    query.bindValue( QStringLiteral( ":hash" ), infoHash );
    query.bindValue( QStringLiteral( ":now" ), now() );

    if( !query.exec() ) {
        reportFailure( query );
        return 0;
    }

    return query.lastInsertId().toLongLong();
}

qint64 PlaylistStore::createCustomPlaylist( const QString &name )
{
    QSqlQuery query( The::database()->connection() );
    query.prepare( QStringLiteral( "INSERT INTO playlist ( name, kind, info_hash, created_at, updated_at )"
                                   " VALUES ( :name, :kind, NULL, :now, :now )" ) );
    query.bindValue( QStringLiteral( ":name" ), name );
    query.bindValue( QStringLiteral( ":kind" ), int( CustomPlaylist ) );
    query.bindValue( QStringLiteral( ":now" ), now() );

    if( !query.exec() ) {
        reportFailure( query );
        return 0;
    }

    return query.lastInsertId().toLongLong();
}

bool PlaylistStore::rename( qint64 playlistId, const QString &name )
{
    // The one that is always there says its own name in the language the
    // interface is running in, so there is nothing here to rename: a name
    // stored in a row could only ever be in one language.
    if( playlist( playlistId ).pinned ) {
        qWarning() << "GrooveOff ::" << "refusing to rename playlist" << playlistId
                   << "because its name belongs to the interface";
        return false;
    }

    QSqlQuery query( The::database()->connection() );
    query.prepare( QStringLiteral( "UPDATE playlist SET name = :name, updated_at = :now WHERE id = :id" ) );
    query.bindValue( QStringLiteral( ":name" ), name );
    query.bindValue( QStringLiteral( ":now" ), now() );
    query.bindValue( QStringLiteral( ":id" ), playlistId );

    return query.exec() ? true : reportFailure( query );
}

bool PlaylistStore::removeCustomPlaylist( qint64 playlistId )
{
    if( playlist( playlistId ).pinned ) {
        qWarning() << "GrooveOff ::" << "refusing to remove playlist" << playlistId
                   << "because it is the one that is always there";
        return false;
    }

    if( !isCustom( playlistId ) ) {
        qWarning() << "GrooveOff ::" << "refusing to remove playlist" << playlistId
                   << "because it belongs to a torrent";
        return false;
    }

    Database *database = The::database();

    if( !database->transaction() )
        return false;

    QSqlQuery query( database->connection() );
    query.prepare( QStringLiteral( "DELETE FROM playlist WHERE id = :id" ) );
    query.bindValue( QStringLiteral( ":id" ), playlistId );

    if( !query.exec() ) {
        reportFailure( query );
        database->rollback();
        return false;
    }

    return database->commit();
}

/*!
  \brief clearPlaylist: empties a list without removing it

  Nothing follows from it beyond the rows going: a track that leaves a list made
  by hand is still in its torrent's playlist, which is where it was copied from.
  It used to carry a garbage collection, back when a torrent could be referenced
  by lists that were not its own.
*/
bool PlaylistStore::clearPlaylist( qint64 playlistId )
{
    if( !isCustom( playlistId ) ) {
        qWarning() << "GrooveOff ::" << "refusing to empty playlist" << playlistId
                   << "because it belongs to a torrent";
        return false;
    }

    Database *database = The::database();

    if( !database->transaction() )
        return false;

    QSqlQuery query( database->connection() );
    query.prepare( QStringLiteral( "DELETE FROM playlist_track WHERE playlist_id = :id" ) );
    query.bindValue( QStringLiteral( ":id" ), playlistId );

    if( !query.exec() ) {
        reportFailure( query );
        database->rollback();
        return false;
    }

    return database->commit();
}

qint64 PlaylistStore::pinnedPlaylistId() const
{
    QSqlQuery query( The::database()->connection() );

    if( !query.exec( QStringLiteral( "SELECT id FROM playlist WHERE pinned = 1" ) ) || !query.next() )
        return 0;

    return query.value( 0 ).toLongLong();
}

QList<PlaylistStore::PlaylistInfo> PlaylistStore::visiblePlaylists() const
{
    QList<PlaylistInfo> playlists;

    QSqlQuery query( The::database()->connection() );
    if( !query.exec( QStringLiteral(
            "SELECT p.id, p.name, p.kind, COALESCE( p.info_hash, '' ),"
            "       ( SELECT COUNT(*) FROM playlist_track pt WHERE pt.playlist_id = p.id ),"
            "       p.pinned"
            "  FROM playlist p"
            " ORDER BY p.pinned DESC, p.kind, p.name COLLATE NOCASE" ) ) ) {
        reportFailure( query );
        return playlists;
    }

    while( query.next() ) {
        PlaylistInfo info;
        info.id         = query.value( 0 ).toLongLong();
        info.name       = query.value( 1 ).toString();
        info.kind       = Kind( query.value( 2 ).toInt() );
        info.infoHash   = query.value( 3 ).toString();
        info.trackCount = query.value( 4 ).toInt();
        info.pinned     = query.value( 5 ).toInt() != 0;
        playlists << info;
    }

    return playlists;
}

PlaylistStore::PlaylistInfo PlaylistStore::playlist( qint64 playlistId ) const
{
    PlaylistInfo info;

    QSqlQuery query( The::database()->connection() );
    query.prepare( QStringLiteral(
        "SELECT p.id, p.name, p.kind, COALESCE( p.info_hash, '' ),"
        "       ( SELECT COUNT(*) FROM playlist_track pt WHERE pt.playlist_id = p.id ),"
        "       p.pinned"
        "  FROM playlist p WHERE p.id = :id" ) );
    query.bindValue( QStringLiteral( ":id" ), playlistId );

    if( !query.exec() || !query.next() )
        return info;

    info.id         = query.value( 0 ).toLongLong();
    info.name       = query.value( 1 ).toString();
    info.kind       = Kind( query.value( 2 ).toInt() );
    info.infoHash   = query.value( 3 ).toString();
    info.trackCount = query.value( 4 ).toInt();
    info.pinned     = query.value( 5 ).toInt() != 0;

    return info;
}

qint64 PlaylistStore::torrentPlaylistId( const QString &infoHash ) const
{
    QSqlQuery query( The::database()->connection() );
    query.prepare( QStringLiteral( "SELECT id FROM playlist WHERE info_hash = :hash" ) );
    query.bindValue( QStringLiteral( ":hash" ), infoHash );

    if( !query.exec() || !query.next() )
        return 0;

    return query.value( 0 ).toLongLong();
}

// --- contents ---------------------------------------------------------------

QList<TrackInfoPtr> PlaylistStore::tracks( qint64 playlistId ) const
{
    QList<TrackInfoPtr> tracks;

    QSqlQuery query( The::database()->connection() );
    query.prepare( QStringLiteral(
        "SELECT t.id, t.info_hash, t.file_index, t.file_path, t.file_size,"
        "       t.is_audio, t.title, t.artist, t.album,"
        "       t.track_num, t.year, t.duration, tor.magnet"
        "  FROM playlist_track pt"
        "  JOIN track   t   ON t.id = pt.track_id"
        "  JOIN torrent tor ON tor.info_hash = t.info_hash"
        " WHERE pt.playlist_id = :id ORDER BY pt.position" ) );
    query.bindValue( QStringLiteral( ":id" ), playlistId );

    if( !query.exec() ) {
        reportFailure( query );
        return tracks;
    }

    while( query.next() ) {
        TrackInfoPtr track( new TrackInfo() );
        track->setId( query.value( 0 ).toLongLong() );
        track->setInfoHash( query.value( 1 ).toString() );
        track->setFileIndex( query.value( 2 ).toInt() );
        track->setFilePath( query.value( 3 ).toString() );
        track->setFileSize( query.value( 4 ).toLongLong() );
        track->setIsAudio( query.value( 5 ).toBool() );
        track->setTitle( query.value( 6 ).toString() );
        track->setArtist( query.value( 7 ).toString() );
        track->setAlbum( query.value( 8 ).toString() );
        track->setTrackNum( query.value( 9 ).toUInt() );
        track->setYear( query.value( 10 ).toUInt() );
        track->setDuration( query.value( 11 ).toLongLong() );
        track->setMagnetUri( query.value( 12 ).toString() );
        tracks << track;
    }

    return tracks;
}

QList<qint64> PlaylistStore::trackIds( qint64 playlistId ) const
{
    QList<qint64> ids;

    QSqlQuery query( The::database()->connection() );
    query.prepare( QStringLiteral( "SELECT track_id FROM playlist_track"
                                   " WHERE playlist_id = :id ORDER BY position" ) );
    query.bindValue( QStringLiteral( ":id" ), playlistId );

    if( !query.exec() ) {
        reportFailure( query );
        return ids;
    }

    while( query.next() )
        ids << query.value( 0 ).toLongLong();

    return ids;
}

/*!
  \brief writeTrackIds: the contents of a playlist, in order and without repeats

  Every path that changes what a playlist holds comes through here — appending,
  replacing, reordering — which is what makes this the place to keep a track
  from appearing twice. A repeat is dropped and the first one stays where it
  was: the schema refuses it as well, and a caller that sent one would
  otherwise lose the whole write to a constraint rather than the one row it got
  wrong.

  Positions stay contiguous from zero afterwards, which the arithmetic of a
  drag depends on.
*/
bool PlaylistStore::writeTrackIds( qint64 playlistId, const QList<qint64> &requested )
{
    QList<qint64> trackIds;
    for( qint64 trackId : requested ) {
        if( !trackIds.contains( trackId ) )
            trackIds << trackId;
    }

    QSqlQuery clear( The::database()->connection() );
    clear.prepare( QStringLiteral( "DELETE FROM playlist_track WHERE playlist_id = :id" ) );
    clear.bindValue( QStringLiteral( ":id" ), playlistId );

    if( !clear.exec() )
        return reportFailure( clear );

    QSqlQuery insert( The::database()->connection() );
    insert.prepare( QStringLiteral( "INSERT INTO playlist_track ( playlist_id, position, track_id )"
                                    " VALUES ( :id, :position, :track )" ) );

    for( int position = 0; position < trackIds.count(); ++position ) {
        insert.bindValue( QStringLiteral( ":id" ), playlistId );
        insert.bindValue( QStringLiteral( ":position" ), position );
        insert.bindValue( QStringLiteral( ":track" ), trackIds.at( position ) );

        if( !insert.exec() )
            return reportFailure( insert );
    }

    return touch( playlistId );
}

bool PlaylistStore::appendTracks( qint64 playlistId, const QList<TrackInfoPtr> &tracks )
{
    if( tracks.isEmpty() )
        return true;

    QList<qint64> ids = trackIds( playlistId );

    for( const TrackInfoPtr &track : tracks ) {
        if( track && track->id() > 0 )
            ids << track->id();
    }

    Database *database = The::database();

    if( !database->transaction() )
        return false;

    if( !writeTrackIds( playlistId, ids ) ) {
        database->rollback();
        return false;
    }

    return database->commit();
}

bool PlaylistStore::setTracks( qint64 playlistId, const QList<TrackInfoPtr> &tracks )
{
    QList<qint64> ids;
    for( const TrackInfoPtr &track : tracks ) {
        if( track && track->id() > 0 )
            ids << track->id();
    }

    Database *database = The::database();

    if( !database->transaction() )
        return false;

    if( !writeTrackIds( playlistId, ids ) ) {
        database->rollback();
        return false;
    }

    return database->commit();
}

bool PlaylistStore::contains( qint64 playlistId, qint64 trackId ) const
{
    QSqlQuery query( The::database()->connection() );
    query.prepare( QStringLiteral( "SELECT 1 FROM playlist_track"
                                   " WHERE playlist_id = :playlist AND track_id = :track" ) );
    query.bindValue( QStringLiteral( ":playlist" ), playlistId );
    query.bindValue( QStringLiteral( ":track" ), trackId );

    return query.exec() && query.next();
}

bool PlaylistStore::removeTrackAt( qint64 playlistId, int position )
{
    QList<qint64> ids = trackIds( playlistId );

    if( position < 0 || position >= ids.count() )
        return false;

    const qint64 removedTrackId = ids.takeAt( position );

    Database *database = The::database();

    QStringList affected;
    const TrackInfoPtr &removed = The::catalog()->track( removedTrackId );
    if( removed )
        affected << removed->infoHash();

    if( !database->transaction() )
        return false;

    if( !writeTrackIds( playlistId, ids ) ) {
        database->rollback();
        return false;
    }

    return database->commit();
}

/*!
  \brief removeTracksEverywhere: the removal that means "never chosen"

  Every list that held one of these tracks is rewritten, not only the one on
  screen: the torrent's playlist is the list of what was chosen from it, so a
  track taken out of it has to leave the lists made by hand as well, or a list
  would go on pointing at music whose bytes are about to be thrown away.

  The rewriting is per list rather than one DELETE, because the positions have
  to come back contiguous from zero — the arithmetic of a drag reads them as
  indexes, and a hole makes a track move by two.
*/
bool PlaylistStore::removeTracksEverywhere( const QList<qint64> &trackIds )
{
    if( trackIds.isEmpty() )
        return true;

    QSet<qint64> removed;
    for( qint64 trackId : trackIds ) {
        if( trackId > 0 )
            removed.insert( trackId );
    }

    if( removed.isEmpty() )
        return true;

    // Which lists are touched has to be read before anything is written, and
    // the ids are checked to be numbers on the way in, so the list built into
    // the statement is ours rather than anybody's.
    QStringList placeholders;
    for( qint64 trackId : std::as_const( removed ) )
        placeholders << QString::number( trackId );

    Database *database = The::database();

    QList<qint64> playlists;
    QSqlQuery affected( database->connection() );
    if( !affected.exec( QStringLiteral( "SELECT DISTINCT playlist_id FROM playlist_track"
                                        " WHERE track_id IN ( %1 )" )
                            .arg( placeholders.join( QLatin1Char( ',' ) ) ) ) ) {
        return reportFailure( affected );
    }

    while( affected.next() )
        playlists << affected.value( 0 ).toLongLong();

    if( playlists.isEmpty() )
        return true;

    if( !database->transaction() )
        return false;

    for( qint64 playlistId : std::as_const( playlists ) ) {
        QList<qint64> kept;
        for( qint64 trackId : this->trackIds( playlistId ) ) {
            if( !removed.contains( trackId ) )
                kept << trackId;
        }

        if( !writeTrackIds( playlistId, kept ) ) {
            database->rollback();
            return false;
        }
    }

    return database->commit();
}

bool PlaylistStore::removeTracks( qint64 playlistId, const QList<qint64> &trackIds )
{
    if( trackIds.isEmpty() )
        return true;

    QSet<qint64> removed( trackIds.constBegin(), trackIds.constEnd() );

    QList<qint64> kept;
    for( qint64 trackId : this->trackIds( playlistId ) ) {
        if( !removed.contains( trackId ) )
            kept << trackId;
    }

    Database *database = The::database();

    if( !database->transaction() )
        return false;

    if( !writeTrackIds( playlistId, kept ) ) {
        database->rollback();
        return false;
    }

    return database->commit();
}

bool PlaylistStore::removeTorrentPlaylist( const QString &infoHash )
{
    if( infoHash.isEmpty() )
        return false;

    QSqlQuery query( The::database()->connection() );
    query.prepare( QStringLiteral( "DELETE FROM playlist WHERE info_hash = :hash" ) );
    query.bindValue( QStringLiteral( ":hash" ), infoHash );

    // playlist_track follows through ON DELETE CASCADE; torrent and track are
    // on the other side of the reference and are deliberately left alone.
    return query.exec() ? true : reportFailure( query );
}

bool PlaylistStore::moveTrack( qint64 playlistId, int from, int to )
{
    QList<qint64> ids = trackIds( playlistId );

    if( from < 0 || from >= ids.count() || to < 0 || to >= ids.count() || from == to )
        return false;

    ids.move( from, to );

    Database *database = The::database();

    if( !database->transaction() )
        return false;

    if( !writeTrackIds( playlistId, ids ) ) {
        database->rollback();
        return false;
    }

    return database->commit();
}

// --- what was playing -------------------------------------------------------

qint64 PlaylistStore::currentPlaylistId() const
{
    QSqlQuery query( The::database()->connection() );

    if( !query.exec( QStringLiteral( "SELECT playlist_id FROM app_state WHERE id = 1" ) ) || !query.next() )
        return 0;

    return query.value( 0 ).toLongLong();
}

qint64 PlaylistStore::currentTrackId() const
{
    QSqlQuery query( The::database()->connection() );

    if( !query.exec( QStringLiteral( "SELECT track_id FROM app_state WHERE id = 1" ) ) || !query.next() )
        return 0;

    return query.value( 0 ).toLongLong();
}

qint64 PlaylistStore::currentPositionMs() const
{
    QSqlQuery query( The::database()->connection() );

    if( !query.exec( QStringLiteral( "SELECT position_ms FROM app_state WHERE id = 1" ) ) || !query.next() )
        return 0;

    return query.value( 0 ).toLongLong();
}

bool PlaylistStore::setPlaybackState( qint64 playlistId, qint64 trackId, qint64 positionMs )
{
    QSqlQuery query( The::database()->connection() );
    // An upsert and not an UPDATE: the row is a singleton the writer has no
    // business assuming somebody else created. An UPDATE that matches nothing
    // reports success, so getting this wrong loses what was playing without a
    // word anywhere.
    query.prepare( QStringLiteral( "INSERT INTO app_state ( id, playlist_id, track_id, position_ms )"
                                   " VALUES ( 1, :playlist, :track, :position )"
                                   " ON CONFLICT( id ) DO UPDATE SET playlist_id = :playlist,"
                                   " track_id = :track, position_ms = :position" ) );
    // A playlist or a track that is not there any more is written as nothing,
    // and that is the writer guaranteeing what it writes into rather than
    // trusting whoever called it. It is not a hypothetical: removing a torrent
    // while one of its tracks is playing leaves the player holding rows that
    // have just been deleted, and naming one of them makes the whole upsert
    // fail on the foreign key — taking the playlist and the position down with
    // a track that was doomed anyway. Silence in the log and a playback state
    // that stops being written, for a reason nothing on screen could show.
    query.bindValue( QStringLiteral( ":playlist" ),
                     exists( QStringLiteral( "playlist" ), playlistId ) ? QVariant( playlistId ) : QVariant() );
    query.bindValue( QStringLiteral( ":track" ),
                     exists( QStringLiteral( "track" ), trackId ) ? QVariant( trackId ) : QVariant() );
    query.bindValue( QStringLiteral( ":position" ), positionMs );

    return query.exec() ? true : reportFailure( query );
}

// --- helpers ----------------------------------------------------------------

/*!
  \brief exists: whether a row of that table is still there

  @p table is a literal from this file and never anything a caller supplied,
  which is why it can be pasted into the statement: there is no way for the
  outside to reach it.
*/
bool PlaylistStore::exists( const QString &table, qint64 id ) const
{
    if( id <= 0 )
        return false;

    QSqlQuery query( The::database()->connection() );
    query.prepare( QStringLiteral( "SELECT 1 FROM %1 WHERE id = :id" ).arg( table ) );
    query.bindValue( QStringLiteral( ":id" ), id );

    return query.exec() && query.next();
}

bool PlaylistStore::isCustom( qint64 playlistId ) const
{
    QSqlQuery query( The::database()->connection() );
    query.prepare( QStringLiteral( "SELECT kind FROM playlist WHERE id = :id" ) );
    query.bindValue( QStringLiteral( ":id" ), playlistId );

    if( !query.exec() || !query.next() )
        return false;

    return query.value( 0 ).toInt() == int( CustomPlaylist );
}

bool PlaylistStore::touch( qint64 playlistId )
{
    QSqlQuery query( The::database()->connection() );
    query.prepare( QStringLiteral( "UPDATE playlist SET updated_at = :now WHERE id = :id" ) );
    query.bindValue( QStringLiteral( ":now" ), now() );
    query.bindValue( QStringLiteral( ":id" ), playlistId );

    return query.exec() ? true : reportFailure( query );
}
