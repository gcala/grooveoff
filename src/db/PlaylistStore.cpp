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
#include <QSqlError>
#include <QSqlQuery>

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
    QSqlQuery query( The::database()->connection() );
    query.prepare( QStringLiteral( "UPDATE playlist SET name = :name, updated_at = :now WHERE id = :id" ) );
    query.bindValue( QStringLiteral( ":name" ), name );
    query.bindValue( QStringLiteral( ":now" ), now() );
    query.bindValue( QStringLiteral( ":id" ), playlistId );

    return query.exec() ? true : reportFailure( query );
}

bool PlaylistStore::removeCustomPlaylist( qint64 playlistId )
{
    if( !isCustom( playlistId ) ) {
        qWarning() << "GrooveOff ::" << "refusing to remove playlist" << playlistId
                   << "because it belongs to a torrent";
        return false;
    }

    Database *database = The::database();

    // Which torrents this playlist was keeping alive has to be read before the
    // rows go away.
    const QStringList &affected = referencedTorrents( playlistId );

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

    if( !collectGarbageFor( affected ) ) {
        database->rollback();
        return false;
    }

    return database->commit();
}

QList<PlaylistStore::PlaylistInfo> PlaylistStore::visiblePlaylists() const
{
    QList<PlaylistInfo> playlists;

    QSqlQuery query( The::database()->connection() );
    if( !query.exec( QStringLiteral(
            "SELECT p.id, p.name, p.kind, COALESCE( p.info_hash, '' ),"
            "       ( SELECT COUNT(*) FROM playlist_track pt WHERE pt.playlist_id = p.id )"
            "  FROM playlist p"
            "  LEFT JOIN torrent t ON t.info_hash = p.info_hash"
            " WHERE p.kind = 1 OR t.hidden = 0"
            " ORDER BY p.kind, p.name COLLATE NOCASE" ) ) ) {
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
        "       ( SELECT COUNT(*) FROM playlist_track pt WHERE pt.playlist_id = p.id )"
        "  FROM playlist p WHERE p.id = :id" ) );
    query.bindValue( QStringLiteral( ":id" ), playlistId );

    if( !query.exec() || !query.next() )
        return info;

    info.id         = query.value( 0 ).toLongLong();
    info.name       = query.value( 1 ).toString();
    info.kind       = Kind( query.value( 2 ).toInt() );
    info.infoHash   = query.value( 3 ).toString();
    info.trackCount = query.value( 4 ).toInt();

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
        "       t.is_audio, t.excluded, t.title, t.artist, t.album,"
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
        track->setExcluded( query.value( 6 ).toBool() );
        track->setTitle( query.value( 7 ).toString() );
        track->setArtist( query.value( 8 ).toString() );
        track->setAlbum( query.value( 9 ).toString() );
        track->setTrackNum( query.value( 10 ).toUInt() );
        track->setYear( query.value( 11 ).toUInt() );
        track->setDuration( query.value( 12 ).toLongLong() );
        track->setMagnetUri( query.value( 13 ).toString() );
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

bool PlaylistStore::writeTrackIds( qint64 playlistId, const QList<qint64> &trackIds )
{
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

    // What the playlist used to hold may have been the last thing keeping a
    // hidden torrent alive.
    const QStringList &affected = referencedTorrents( playlistId );

    if( !database->transaction() )
        return false;

    if( !writeTrackIds( playlistId, ids ) || !collectGarbageFor( affected ) ) {
        database->rollback();
        return false;
    }

    return database->commit();
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

    if( !writeTrackIds( playlistId, ids ) || !collectGarbageFor( affected ) ) {
        database->rollback();
        return false;
    }

    return database->commit();
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

QStringList PlaylistStore::referencedTorrents( qint64 playlistId ) const
{
    QStringList hashes;

    QSqlQuery query( The::database()->connection() );
    query.prepare( QStringLiteral( "SELECT DISTINCT t.info_hash FROM playlist_track pt"
                                   " JOIN track t ON t.id = pt.track_id"
                                   " WHERE pt.playlist_id = :id" ) );
    query.bindValue( QStringLiteral( ":id" ), playlistId );

    if( !query.exec() ) {
        reportFailure( query );
        return hashes;
    }

    while( query.next() )
        hashes << query.value( 0 ).toString();

    return hashes;
}

bool PlaylistStore::collectGarbageFor( const QStringList &infoHashes )
{
    Catalog *catalog = The::catalog();

    for( const QString &infoHash : infoHashes ) {
        if( !catalog->collectGarbage( infoHash ) )
            return false;
    }

    return true;
}

bool PlaylistStore::touch( qint64 playlistId )
{
    QSqlQuery query( The::database()->connection() );
    query.prepare( QStringLiteral( "UPDATE playlist SET updated_at = :now WHERE id = :id" ) );
    query.bindValue( QStringLiteral( ":now" ), now() );
    query.bindValue( QStringLiteral( ":id" ), playlistId );

    return query.exec() ? true : reportFailure( query );
}
