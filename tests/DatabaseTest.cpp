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


#include "db/Catalog.h"
#include "db/Database.h"
#include "db/PlaylistStore.h"
#include "grooveoff/TrackInfo.h"

#include <QCoreApplication>
#include <QFile>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QTest>

/**
  * @class DatabaseTest
  * @short Exercises the catalogue, the playlists and the boundary between them.
  *
  * This is one scenario told in order, not a set of independent cases: the
  * functions share state on purpose, because what is worth checking here is how
  * a torrent behaves over its whole life, from being added to being collected
  * once nothing refers to it any more.
  *
  * QStandardPaths test mode keeps the database out of the real data directory.
  */
class DatabaseTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void addsTorrentsAndTheirPlaylists();
    void storesEveryFileOfATorrent();
    void exclusionIsStickyAndDoesNotEmptyPlaylists();
    void customPlaylistSpansTorrents();
    void hidingKeepsTracksReachable();
    void reAddingRestoresTheSelection();
    void garbageCollectionRemovesTheLastUnreferenced();
    void cacheEvictionKeepsTheCatalogue();
    void remembersWhatWasPlaying();
    void customPlaylistRemoval();
    void survivesALostPlaybackRow();

private:
    Catalog*       m_catalog  = nullptr;
    PlaylistStore* m_playlists = nullptr;

    QString m_hashA;
    QString m_hashB;
    QList<TrackInfoPtr> m_tracksA;
    QList<TrackInfoPtr> m_tracksB;
    qint64  m_customId = 0;

    static TrackInfoPtr makeTrack( const QString &infoHash, int fileIndex, const QString &filePath );
};

TrackInfoPtr DatabaseTest::makeTrack( const QString &infoHash, int fileIndex, const QString &filePath )
{
    TrackInfoPtr track( new TrackInfo() );
    track->setInfoHash( infoHash );
    track->setFileIndex( fileIndex );
    track->setFilePath( filePath );
    track->setFileSize( 5 * 1024 * 1024 );
    track->setIsAudio( TrackInfo::isAudioFile( filePath ) );

    return track;
}

void DatabaseTest::initTestCase()
{
    QStandardPaths::setTestModeEnabled( true );
    QCoreApplication::setOrganizationName( QStringLiteral( "gcala" ) );
    QCoreApplication::setApplicationName( QStringLiteral( "grooveoff-test" ) );

    // Start from nothing: the singleton reads the path in its constructor, so
    // the file has to go before anything touches it.
    const QString &path = QStandardPaths::writableLocation( QStandardPaths::AppDataLocation )
                          + QLatin1String( "/grooveoff.db" );
    QFile::remove( path );
    QFile::remove( path + QLatin1String( "-wal" ) );
    QFile::remove( path + QLatin1String( "-shm" ) );

    m_catalog   = The::catalog();
    m_playlists = The::playlistStore();

    QVERIFY2( The::database()->isOpen(), qPrintable( The::database()->errorString() ) );
    QCOMPARE( Database::schemaVersion(), 1 );

    // The playlist that is always there arrives with the schema and not with
    // the interface: a database is the only place that can guarantee it exists
    // before anything has drawn a sidebar.
    const qint64 pinned = The::playlistStore()->pinnedPlaylistId();
    QVERIFY2( pinned > 0, "the migration did not leave a pinned playlist behind" );
    QVERIFY( The::playlistStore()->playlist( pinned ).pinned );

    m_hashA = QStringLiteral( "aaaa000000000000000000000000000000000001" );
    m_hashB = QStringLiteral( "bbbb000000000000000000000000000000000002" );
}

void DatabaseTest::addsTorrentsAndTheirPlaylists()
{
    QVERIFY( m_catalog->addTorrent( m_hashA, QLatin1String( "magnet:?xt=urn:btih:" ) + m_hashA,
                                    QStringLiteral( "Album A" ) ) );
    QVERIFY( m_catalog->addTorrent( m_hashB, QLatin1String( "magnet:?xt=urn:btih:" ) + m_hashB,
                                    QStringLiteral( "Album B" ) ) );

    QVERIFY( m_catalog->isKnown( m_hashA ) );
    QVERIFY( !m_catalog->isHidden( m_hashA ) );
    QVERIFY( m_playlists->torrentPlaylistId( m_hashA ) > 0 );

    // Adding a known torrent again must not disturb anything.
    QVERIFY( m_catalog->addTorrent( m_hashA, QLatin1String( "magnet:?xt=urn:btih:" ) + m_hashA,
                                    QStringLiteral( "Album A" ) ) );

    // The two torrents, and the playlist that came with the schema.
    QCOMPARE( m_playlists->visiblePlaylists().count(), 3 );
}

void DatabaseTest::storesEveryFileOfATorrent()
{
    m_tracksA << makeTrack( m_hashA, 0, QStringLiteral( "Album A/01 - one.mp3" ) )
              << makeTrack( m_hashA, 1, QStringLiteral( "Album A/02 - two.flac" ) )
              << makeTrack( m_hashA, 2, QStringLiteral( "Album A/cover.jpg" ) );

    QVERIFY( m_catalog->addTracks( m_tracksA ) );
    QVERIFY( m_tracksA.at( 0 )->id() > 0 );

    // The cover is kept, only flagged: an unforeseen extension must stay
    // reachable instead of silently disappearing.
    QVERIFY( !m_tracksA.at( 2 )->isAudio() );
    QCOMPARE( m_catalog->tracks( m_hashA ).count(), 2 );
    QCOMPARE( m_catalog->tracks( m_hashA, false ).count(), 3 );

    m_tracksB << makeTrack( m_hashB, 0, QStringLiteral( "Album B/01 - uno.ogg" ) )
              << makeTrack( m_hashB, 1, QStringLiteral( "Album B/02 - due.opus" ) );
    QVERIFY( m_catalog->addTracks( m_tracksB ) );

    // Looking at the same torrent twice must not duplicate its files.
    QVERIFY( m_catalog->addTracks( m_tracksA ) );
    QCOMPARE( m_catalog->tracks( m_hashA, false ).count(), 3 );
}

void DatabaseTest::exclusionIsStickyAndDoesNotEmptyPlaylists()
{
    const qint64 playlistA = m_playlists->torrentPlaylistId( m_hashA );
    QVERIFY( m_playlists->setTracks( playlistA, m_catalog->tracks( m_hashA ) ) );
    QCOMPARE( m_playlists->tracks( playlistA ).count(), 2 );

    QVERIFY( m_catalog->setExcluded( m_tracksA.at( 1 )->id(), true ) );

    // Excluded means "stop offering it", not "erase it".
    QCOMPARE( m_catalog->tracks( m_hashA ).count(), 1 );
    QCOMPARE( m_catalog->tracks( m_hashA, true, true ).count(), 2 );

    // And a deliberate choice already made in a playlist is not undone by it.
    QCOMPARE( m_playlists->tracks( playlistA ).count(), 2 );
}

void DatabaseTest::customPlaylistSpansTorrents()
{
    m_customId = m_playlists->createCustomPlaylist( QStringLiteral( "Preferite" ) );
    QVERIFY( m_customId > 0 );

    QList<TrackInfoPtr> mixed;
    mixed << m_tracksA.at( 0 ) << m_tracksB.at( 0 ) << m_tracksB.at( 1 );
    QVERIFY( m_playlists->appendTracks( m_customId, mixed ) );

    QCOMPARE( m_playlists->tracks( m_customId ).count(), 3 );
    QCOMPARE( m_playlists->tracks( m_customId ).at( 0 )->infoHash(), m_hashA );
    QCOMPARE( m_playlists->tracks( m_customId ).at( 1 )->infoHash(), m_hashB );

    QVERIFY( m_playlists->moveTrack( m_customId, 0, 2 ) );
    QCOMPARE( m_playlists->tracks( m_customId ).at( 2 )->infoHash(), m_hashA );
}

void DatabaseTest::hidingKeepsTracksReachable()
{
    const QList<Catalog::PlaylistReference> &references = m_catalog->customPlaylistReferences( m_hashA );
    QCOMPARE( references.count(), 1 );
    QCOMPARE( references.first().trackCount, 1 );
    QCOMPARE( references.first().name, QStringLiteral( "Preferite" ) );

    QVERIFY( m_catalog->hideTorrent( m_hashA ) );
    QVERIFY( m_catalog->isHidden( m_hashA ) );

    // Out of sight: torrent B, the custom playlist and the pinned one remain.
    QCOMPARE( m_playlists->visiblePlaylists().count(), 3 );

    // Still alive, because a custom playlist points at one of its tracks.
    QVERIFY( m_catalog->collectGarbage( m_hashA ) );
    QVERIFY( m_catalog->isKnown( m_hashA ) );
    QCOMPARE( m_playlists->tracks( m_customId ).count(), 3 );
}

void DatabaseTest::reAddingRestoresTheSelection()
{
    QVERIFY( m_catalog->addTorrent( m_hashA, QLatin1String( "magnet:?xt=urn:btih:" ) + m_hashA,
                                    QStringLiteral( "Album A" ) ) );
    QVERIFY( !m_catalog->isHidden( m_hashA ) );

    // Files and exclusions survived being hidden.
    QCOMPARE( m_catalog->tracks( m_hashA, true, true ).count(), 2 );
    QCOMPARE( m_catalog->tracks( m_hashA ).count(), 1 );
}

void DatabaseTest::garbageCollectionRemovesTheLastUnreferenced()
{
    QVERIFY( m_catalog->hideTorrent( m_hashA ) );

    QList<TrackInfoPtr> onlyB;
    onlyB << m_tracksB.at( 0 ) << m_tracksB.at( 1 );
    QVERIFY( m_playlists->setTracks( m_customId, onlyB ) );

    // The last reference is gone, so the hidden torrent goes with it.
    QVERIFY( !m_catalog->isKnown( m_hashA ) );
    QVERIFY( m_catalog->orphanedHiddenTorrents().isEmpty() );
}

void DatabaseTest::cacheEvictionKeepsTheCatalogue()
{
    QVERIFY( m_catalog->setBytesDone( m_hashB, 4 * 1024 * 1024 ) );
    QCOMPARE( m_catalog->totalBytesDone(), qint64( 4 * 1024 * 1024 ) );

    // A null QString binds as NULL, and "info_hash <> NULL" is NULL rather than
    // true: without COALESCE nothing is ever a candidate while nothing plays.
    QCOMPARE( m_catalog->evictionCandidates( QString() ).count(), 1 );
    QVERIFY( m_catalog->evictionCandidates( m_hashB ).isEmpty() );

    QVERIFY( m_catalog->clearData( m_hashB ) );

    // The cache holds bytes, the catalogue holds knowledge: evicting must not
    // take away tracks a custom playlist still points at.
    QVERIFY( m_catalog->isKnown( m_hashB ) );
    QCOMPARE( m_playlists->tracks( m_customId ).count(), 2 );
    QCOMPARE( m_catalog->totalBytesDone(), qint64( 0 ) );
}

void DatabaseTest::remembersWhatWasPlaying()
{
    QVERIFY( m_playlists->setPlaybackState( m_customId, m_tracksB.at( 0 )->id(), 87340 ) );

    QCOMPARE( m_playlists->currentPlaylistId(), m_customId );
    QCOMPARE( m_playlists->currentTrackId(), m_tracksB.at( 0 )->id() );
    QCOMPARE( m_playlists->currentPositionMs(), qint64( 87340 ) );
}

void DatabaseTest::customPlaylistRemoval()
{
    QVERIFY( m_playlists->removeCustomPlaylist( m_customId ) );

    // Torrent B and the pinned playlist: the list made by hand is gone and the
    // one that cannot be removed is not.
    QCOMPARE( m_playlists->visiblePlaylists().count(), 2 );

    // A torrent playlist belongs to its torrent and is removed with it.
    QVERIFY( !m_playlists->removeCustomPlaylist( m_playlists->torrentPlaylistId( m_hashB ) ) );
}

/*!
  \brief survivesALostPlaybackRow: the singleton row is the writer's business

  Reaching for QSqlQuery is against the rule that no QSql type leaves src/db,
  and it is the reason this case had no test until it cost somebody their track
  list: there is no way through the public interface to take the row away, and
  what happens when it is gone is exactly what needed checking. An UPDATE that
  matches nothing reports success, so the application opened with an empty list
  and nothing anywhere said why.
*/
void DatabaseTest::survivesALostPlaybackRow()
{
    QSqlQuery query( The::database()->connection() );
    QVERIFY( query.exec( QStringLiteral( "DELETE FROM app_state" ) ) );

    QCOMPARE( m_playlists->currentPlaylistId(), qint64( 0 ) );

    const qint64 playlistId = m_playlists->createCustomPlaylist( QStringLiteral( "Again" ) );
    QVERIFY( playlistId > 0 );

    // The writer puts the row back rather than assuming somebody else did.
    QVERIFY( m_playlists->setPlaybackState( playlistId, 0, 0 ) );

    QCOMPARE( m_playlists->currentPlaylistId(), playlistId );
    QCOMPARE( m_playlists->currentTrackId(), qint64( 0 ) );
    QCOMPARE( m_playlists->currentPositionMs(), qint64( 0 ) );
}

QTEST_MAIN( DatabaseTest )

#include "DatabaseTest.moc"
