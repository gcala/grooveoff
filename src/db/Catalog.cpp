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


#include "Catalog.h"
#include "Database.h"
#include "PlaylistStore.h"

#include <QDateTime>
#include <QDebug>
#include <QHash>
#include <QSqlError>
#include <QSqlQuery>

namespace The {
    static Catalog* s_Catalog_instance = nullptr;

    Catalog* catalog()
    {
        if( !s_Catalog_instance )
            s_Catalog_instance = new Catalog();

        return s_Catalog_instance;
    }
}

namespace {

/** the columns of track, in the order trackFromQuery() expects them */
const char c_trackColumns[] = "id, info_hash, file_index, file_path, file_size,"
                              " is_audio, excluded, title, artist, album,"
                              " track_num, year, duration";

qint64 now()
{
    return QDateTime::currentSecsSinceEpoch();
}

bool reportFailure( const QSqlQuery &query )
{
    qWarning() << "GrooveOff ::" << "catalog query failed:" << query.lastError().text()
               << "in" << query.lastQuery();
    return false;
}

} // namespace

Catalog::Catalog()
{
}

Catalog::~Catalog()
{
}

TrackInfoPtr Catalog::trackFromQuery( const QSqlQuery &query )
{
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

    return track;
}

// --- torrents ---------------------------------------------------------------

bool Catalog::addTorrent( const QString &infoHash, const QString &magnetUri, const QString &name )
{
    Database *database = The::database();

    if( !database->isOpen() )
        return false;

    if( isKnown( infoHash ) ) {
        // Already in the catalogue. Bringing it back into sight is all that is
        // needed: its tracks, exclusions and playlist are still there.
        return isHidden( infoHash ) ? unhideTorrent( infoHash ) : true;
    }

    if( !database->transaction() )
        return false;

    QSqlQuery query( database->connection() );
    query.prepare( QStringLiteral( "INSERT INTO torrent ( info_hash, magnet, name, added_at, last_access )"
                                   " VALUES ( :hash, :magnet, :name, :now, :now )" ) );
    query.bindValue( QStringLiteral( ":hash" ), infoHash );
    query.bindValue( QStringLiteral( ":magnet" ), magnetUri );
    query.bindValue( QStringLiteral( ":name" ), name );
    query.bindValue( QStringLiteral( ":now" ), now() );

    if( !query.exec() ) {
        reportFailure( query );
        database->rollback();
        return false;
    }

    // Every torrent owns exactly one playlist: the unique index on
    // playlist(info_hash) is what keeps that true.
    if( The::playlistStore()->createTorrentPlaylist( infoHash, name ) <= 0 ) {
        database->rollback();
        return false;
    }

    return database->commit();
}

bool Catalog::isKnown( const QString &infoHash ) const
{
    QSqlQuery query( The::database()->connection() );
    query.prepare( QStringLiteral( "SELECT 1 FROM torrent WHERE info_hash = :hash" ) );
    query.bindValue( QStringLiteral( ":hash" ), infoHash );

    return query.exec() && query.next();
}

bool Catalog::isHidden( const QString &infoHash ) const
{
    QSqlQuery query( The::database()->connection() );
    query.prepare( QStringLiteral( "SELECT hidden FROM torrent WHERE info_hash = :hash" ) );
    query.bindValue( QStringLiteral( ":hash" ), infoHash );

    if( !query.exec() || !query.next() )
        return false;

    return query.value( 0 ).toBool();
}

QString Catalog::magnetUri( const QString &infoHash ) const
{
    QSqlQuery query( The::database()->connection() );
    query.prepare( QStringLiteral( "SELECT magnet FROM torrent WHERE info_hash = :hash" ) );
    query.bindValue( QStringLiteral( ":hash" ), infoHash );

    if( !query.exec() || !query.next() )
        return QString();

    return query.value( 0 ).toString();
}

QString Catalog::name( const QString &infoHash ) const
{
    QSqlQuery query( The::database()->connection() );
    query.prepare( QStringLiteral( "SELECT name FROM torrent WHERE info_hash = :hash" ) );
    query.bindValue( QStringLiteral( ":hash" ), infoHash );

    if( !query.exec() || !query.next() )
        return QString();

    return query.value( 0 ).toString();
}

bool Catalog::setName( const QString &infoHash, const QString &name )
{
    QSqlQuery query( The::database()->connection() );
    query.prepare( QStringLiteral( "UPDATE torrent SET name = :name WHERE info_hash = :hash" ) );
    query.bindValue( QStringLiteral( ":name" ), name );
    query.bindValue( QStringLiteral( ":hash" ), infoHash );

    return query.exec() ? true : reportFailure( query );
}

bool Catalog::touch( const QString &infoHash )
{
    QSqlQuery query( The::database()->connection() );
    query.prepare( QStringLiteral( "UPDATE torrent SET last_access = :now WHERE info_hash = :hash" ) );
    query.bindValue( QStringLiteral( ":now" ), now() );
    query.bindValue( QStringLiteral( ":hash" ), infoHash );

    return query.exec() ? true : reportFailure( query );
}

bool Catalog::setBytesDone( const QString &infoHash, qint64 bytes )
{
    QSqlQuery query( The::database()->connection() );
    query.prepare( QStringLiteral( "UPDATE torrent SET bytes_done = :bytes WHERE info_hash = :hash" ) );
    query.bindValue( QStringLiteral( ":bytes" ), bytes );
    query.bindValue( QStringLiteral( ":hash" ), infoHash );

    return query.exec() ? true : reportFailure( query );
}

qint64 Catalog::bytesDone( const QString &infoHash ) const
{
    QSqlQuery query( The::database()->connection() );
    query.prepare( QStringLiteral( "SELECT bytes_done FROM torrent WHERE info_hash = :hash" ) );
    query.bindValue( QStringLiteral( ":hash" ), infoHash );

    if( !query.exec() || !query.next() )
        return 0;

    return query.value( 0 ).toLongLong();
}

bool Catalog::setHasResume( const QString &infoHash, bool hasResume )
{
    QSqlQuery query( The::database()->connection() );
    query.prepare( QStringLiteral( "UPDATE torrent SET has_resume = :has WHERE info_hash = :hash" ) );
    query.bindValue( QStringLiteral( ":has" ), hasResume ? 1 : 0 );
    query.bindValue( QStringLiteral( ":hash" ), infoHash );

    return query.exec() ? true : reportFailure( query );
}

bool Catalog::hasResume( const QString &infoHash ) const
{
    QSqlQuery query( The::database()->connection() );
    query.prepare( QStringLiteral( "SELECT has_resume FROM torrent WHERE info_hash = :hash" ) );
    query.bindValue( QStringLiteral( ":hash" ), infoHash );

    if( !query.exec() || !query.next() )
        return false;

    return query.value( 0 ).toBool();
}

bool Catalog::unhideTorrent( const QString &infoHash )
{
    QSqlQuery query( The::database()->connection() );
    query.prepare( QStringLiteral( "UPDATE torrent SET hidden = 0 WHERE info_hash = :hash" ) );
    query.bindValue( QStringLiteral( ":hash" ), infoHash );

    return query.exec() ? true : reportFailure( query );
}

QStringList Catalog::knownTorrents() const
{
    QStringList hashes;

    QSqlQuery query( The::database()->connection() );
    if( !query.exec( QStringLiteral( "SELECT info_hash FROM torrent ORDER BY last_access DESC" ) ) ) {
        reportFailure( query );
        return hashes;
    }

    while( query.next() )
        hashes << query.value( 0 ).toString();

    return hashes;
}

// --- tracks -----------------------------------------------------------------

bool Catalog::addTracks( const QList<TrackInfoPtr> &tracks )
{
    if( tracks.isEmpty() )
        return true;

    Database *database = The::database();

    if( !database->transaction() )
        return false;

    QSqlQuery insert( database->connection() );
    insert.prepare( QStringLiteral( "INSERT OR IGNORE INTO track"
                                    " ( info_hash, file_index, file_path, file_size, is_audio,"
                                    "   title, artist, album, track_num, year, duration )"
                                    " VALUES ( :hash, :index, :path, :size, :audio,"
                                    "   :title, :artist, :album, :num, :year, :duration )" ) );

    for( const TrackInfoPtr &track : tracks ) {
        insert.bindValue( QStringLiteral( ":hash" ), track->infoHash() );
        insert.bindValue( QStringLiteral( ":index" ), track->fileIndex() );
        insert.bindValue( QStringLiteral( ":path" ), track->filePath() );
        insert.bindValue( QStringLiteral( ":size" ), track->fileSize() );
        insert.bindValue( QStringLiteral( ":audio" ), track->isAudio() ? 1 : 0 );
        insert.bindValue( QStringLiteral( ":title" ), track->title() );
        insert.bindValue( QStringLiteral( ":artist" ), track->artist() );
        insert.bindValue( QStringLiteral( ":album" ), track->album() );
        insert.bindValue( QStringLiteral( ":num" ), track->trackNum() );
        insert.bindValue( QStringLiteral( ":year" ), track->year() );
        insert.bindValue( QStringLiteral( ":duration" ), track->duration() );

        if( !insert.exec() ) {
            reportFailure( insert );
            database->rollback();
            return false;
        }
    }

    if( !database->commit() )
        return false;

    // Hand back the ids, including those of files that were already known and
    // whose insert was ignored.
    const QString &infoHash = tracks.first()->infoHash();

    QSqlQuery ids( database->connection() );
    ids.prepare( QStringLiteral( "SELECT file_index, id, excluded FROM track WHERE info_hash = :hash" ) );
    ids.bindValue( QStringLiteral( ":hash" ), infoHash );

    if( !ids.exec() )
        return reportFailure( ids );

    QHash<int, QPair<qint64, bool> > stored;
    while( ids.next() )
        stored.insert( ids.value( 0 ).toInt(),
                       qMakePair( ids.value( 1 ).toLongLong(), ids.value( 2 ).toBool() ) );

    for( const TrackInfoPtr &track : tracks ) {
        const auto it = stored.constFind( track->fileIndex() );
        if( it != stored.constEnd() ) {
            track->setId( it->first );
            track->setExcluded( it->second );
        }
    }

    return true;
}

QList<TrackInfoPtr> Catalog::tracks( const QString &infoHash, bool audioOnly, bool includeExcluded ) const
{
    QList<TrackInfoPtr> tracks;

    QString statement = QStringLiteral( "SELECT %1 FROM track WHERE info_hash = :hash" )
                            .arg( QLatin1String( c_trackColumns ) );

    if( audioOnly )
        statement += QLatin1String( " AND is_audio = 1" );
    if( !includeExcluded )
        statement += QLatin1String( " AND excluded = 0" );

    statement += QLatin1String( " ORDER BY file_index" );

    QSqlQuery query( The::database()->connection() );
    query.prepare( statement );
    query.bindValue( QStringLiteral( ":hash" ), infoHash );

    if( !query.exec() ) {
        reportFailure( query );
        return tracks;
    }

    const QString &magnet = magnetUri( infoHash );

    while( query.next() ) {
        TrackInfoPtr track = trackFromQuery( query );
        track->setMagnetUri( magnet );
        tracks << track;
    }

    return tracks;
}

TrackInfoPtr Catalog::track( qint64 trackId ) const
{
    QSqlQuery query( The::database()->connection() );
    query.prepare( QStringLiteral( "SELECT %1 FROM track WHERE id = :id" )
                       .arg( QLatin1String( c_trackColumns ) ) );
    query.bindValue( QStringLiteral( ":id" ), trackId );

    if( !query.exec() || !query.next() )
        return TrackInfoPtr();

    TrackInfoPtr track = trackFromQuery( query );
    track->setMagnetUri( magnetUri( track->infoHash() ) );

    return track;
}

TrackInfoPtr Catalog::track( const QString &infoHash, int fileIndex ) const
{
    QSqlQuery query( The::database()->connection() );
    query.prepare( QStringLiteral( "SELECT %1 FROM track WHERE info_hash = :hash AND file_index = :index" )
                       .arg( QLatin1String( c_trackColumns ) ) );
    query.bindValue( QStringLiteral( ":hash" ), infoHash );
    query.bindValue( QStringLiteral( ":index" ), fileIndex );

    if( !query.exec() || !query.next() )
        return TrackInfoPtr();

    TrackInfoPtr track = trackFromQuery( query );
    track->setMagnetUri( magnetUri( infoHash ) );

    return track;
}

bool Catalog::setExcluded( qint64 trackId, bool excluded )
{
    QSqlQuery query( The::database()->connection() );
    query.prepare( QStringLiteral( "UPDATE track SET excluded = :excluded WHERE id = :id" ) );
    query.bindValue( QStringLiteral( ":excluded" ), excluded ? 1 : 0 );
    query.bindValue( QStringLiteral( ":id" ), trackId );

    return query.exec() ? true : reportFailure( query );
}

bool Catalog::updateTags( const TrackInfoPtr &track )
{
    if( !track || track->id() <= 0 )
        return false;

    QSqlQuery query( The::database()->connection() );
    query.prepare( QStringLiteral( "UPDATE track SET title = :title, artist = :artist, album = :album,"
                                   " track_num = :num, year = :year, duration = :duration"
                                   " WHERE id = :id" ) );
    query.bindValue( QStringLiteral( ":title" ), track->title() );
    query.bindValue( QStringLiteral( ":artist" ), track->artist() );
    query.bindValue( QStringLiteral( ":album" ), track->album() );
    query.bindValue( QStringLiteral( ":num" ), track->trackNum() );
    query.bindValue( QStringLiteral( ":year" ), track->year() );
    query.bindValue( QStringLiteral( ":duration" ), track->duration() );
    query.bindValue( QStringLiteral( ":id" ), track->id() );

    return query.exec() ? true : reportFailure( query );
}

// --- removal ----------------------------------------------------------------

QList<Catalog::PlaylistReference> Catalog::customPlaylistReferences( const QString &infoHash ) const
{
    QList<PlaylistReference> references;

    QSqlQuery query( The::database()->connection() );
    query.prepare( QStringLiteral( "SELECT p.id, p.name, COUNT(*) FROM playlist_track pt"
                                   " JOIN track    t ON t.id = pt.track_id"
                                   " JOIN playlist p ON p.id = pt.playlist_id"
                                   " WHERE t.info_hash = :hash AND p.kind = 1"
                                   " GROUP BY p.id, p.name ORDER BY p.name" ) );
    query.bindValue( QStringLiteral( ":hash" ), infoHash );

    if( !query.exec() ) {
        reportFailure( query );
        return references;
    }

    while( query.next() ) {
        PlaylistReference reference;
        reference.playlistId = query.value( 0 ).toLongLong();
        reference.name       = query.value( 1 ).toString();
        reference.trackCount = query.value( 2 ).toInt();
        references << reference;
    }

    return references;
}

bool Catalog::removeTorrent( const QString &infoHash )
{
    QSqlQuery query( The::database()->connection() );
    query.prepare( QStringLiteral( "DELETE FROM torrent WHERE info_hash = :hash" ) );
    query.bindValue( QStringLiteral( ":hash" ), infoHash );

    // track, playlist and playlist_track follow through ON DELETE CASCADE.
    return query.exec() ? true : reportFailure( query );
}

bool Catalog::hideTorrent( const QString &infoHash )
{
    QSqlQuery query( The::database()->connection() );
    query.prepare( QStringLiteral( "UPDATE torrent SET hidden = 1 WHERE info_hash = :hash" ) );
    query.bindValue( QStringLiteral( ":hash" ), infoHash );

    return query.exec() ? true : reportFailure( query );
}

bool Catalog::collectGarbage( const QString &infoHash )
{
    if( infoHash.isEmpty() || !isHidden( infoHash ) )
        return true;

    if( !customPlaylistReferences( infoHash ).isEmpty() )
        return true;

    return removeTorrent( infoHash );
}

QStringList Catalog::orphanedHiddenTorrents() const
{
    QStringList hashes;

    QSqlQuery query( The::database()->connection() );
    if( !query.exec( QStringLiteral( "SELECT t.info_hash FROM torrent t"
                                     " WHERE t.hidden = 1 AND NOT EXISTS ("
                                     "   SELECT 1 FROM playlist_track pt"
                                     "     JOIN track    tr ON tr.id = pt.track_id"
                                     "     JOIN playlist p  ON p.id  = pt.playlist_id"
                                     "    WHERE tr.info_hash = t.info_hash AND p.kind = 1 )" ) ) ) {
        reportFailure( query );
        return hashes;
    }

    while( query.next() )
        hashes << query.value( 0 ).toString();

    return hashes;
}

// --- cache ------------------------------------------------------------------

bool Catalog::clearData( const QString &infoHash )
{
    QSqlQuery query( The::database()->connection() );
    query.prepare( QStringLiteral( "UPDATE torrent SET bytes_done = 0, has_resume = 0"
                                   " WHERE info_hash = :hash" ) );
    query.bindValue( QStringLiteral( ":hash" ), infoHash );

    // Deliberately not a DELETE: the cache holds bytes, the catalogue holds
    // knowledge, and tracks in custom playlists must survive an eviction.
    return query.exec() ? true : reportFailure( query );
}

QList<Catalog::CacheEntry> Catalog::evictionCandidates( const QString &keepInfoHash ) const
{
    QList<CacheEntry> entries;

    QSqlQuery query( The::database()->connection() );
    // COALESCE because a null QString binds as NULL, and "info_hash <> NULL" is
    // NULL rather than true: without it nothing is ever a candidate while no
    // track is playing, which is exactly when eviction matters most.
    query.prepare( QStringLiteral( "SELECT info_hash, bytes_done FROM torrent"
                                   " WHERE info_hash <> COALESCE( :keep, '' ) AND bytes_done > 0"
                                   " ORDER BY last_access ASC" ) );
    query.bindValue( QStringLiteral( ":keep" ), keepInfoHash );

    if( !query.exec() ) {
        reportFailure( query );
        return entries;
    }

    while( query.next() ) {
        CacheEntry entry;
        entry.infoHash  = query.value( 0 ).toString();
        entry.bytesDone = query.value( 1 ).toLongLong();
        entries << entry;
    }

    return entries;
}

qint64 Catalog::totalBytesDone() const
{
    QSqlQuery query( The::database()->connection() );

    if( !query.exec( QStringLiteral( "SELECT COALESCE( SUM( bytes_done ), 0 ) FROM torrent" ) ) || !query.next() )
        return 0;

    return query.value( 0 ).toLongLong();
}
