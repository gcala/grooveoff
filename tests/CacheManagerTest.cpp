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
#include "grooveoff/Settings.h"
#include "torrent/CacheManager.h"
#include "torrent/TorrentSession.h"

#include <QDir>
#include <QFile>
#include <QSettings>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QtTest>

#include <libtorrent/create_torrent.hpp>
#include <libtorrent/error_code.hpp>
#include <libtorrent/load_torrent.hpp>
#include <libtorrent/torrent_info.hpp>

#include <vector>

namespace lt = libtorrent;

/**
  * @class CacheManagerTest
  * @short A cache filling up, and a catalogue that comes through it intact.
  *
  * Told in order like the other two, and the order is the point: the torrent
  * that gets evicted first is the one whose track sits in a custom playlist,
  * so that every later function is checking a survivor rather than a fresh row.
  * If a `DELETE` ever creeps in where a `bytes_done = 0` belongs, this is where
  * it shows up — and it is the one mistake in the whole project that destroys
  * something the user made by hand.
  *
  * Three torrents, on purpose two different kinds: one is really in the session
  * and libtorrent deletes its files, one was never reopened and its directory
  * is ours to remove. Those are the two branches of an eviction and they are
  * easy to get right one at a time and wrong together.
  */
class CacheManagerTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();

    void countsWhatTheCatalogueHolds();
    void evictsTheLeastRecentlyUsedFirst();
    void evictingIsNotRemoving();
    void deletesTheFilesAndTheResume();
    void neverEvictsWhatIsPlaying();
    void suspendsThePrefetchNearTheCeiling();
    void emptiesTheCache();
    void discardingOneFileKeepsTheCatalogue();

private:
    /** a torrent really added to the session, files and all */
    void buildAwakeTorrent();

    /** a torrent the session has never seen, with a directory of its own */
    void buildDormantTorrent();

    static TrackInfoPtr makeTrack( const QString &infoHash, int fileIndex, const QString &filePath );

    /** the ceiling, in megabytes, as the settings hold it */
    void setLimit( int megabytes );

    QTemporaryDir m_root;
    QString       m_cacheRoot;

    QString m_awake;      /**< in the session: libtorrent owns its files */
    QString m_dormant;    /**< catalogue only: the directory is ours to delete */
    QString m_playing;    /**< what is being listened to, and never a candidate */

    qint64 m_customId = 0;
};

TrackInfoPtr CacheManagerTest::makeTrack( const QString &infoHash, int fileIndex, const QString &filePath )
{
    TrackInfoPtr track( new TrackInfo() );
    track->setInfoHash( infoHash );
    track->setFileIndex( fileIndex );
    track->setFilePath( filePath );
    track->setFileSize( 5 * 1024 * 1024 );
    track->setIsAudio( TrackInfo::isAudioFile( filePath ) );

    return track;
}

void CacheManagerTest::setLimit( int megabytes )
{
    QSettings settings;
    settings.setValue( Settings::cacheSize, megabytes );
    settings.sync();
}

void CacheManagerTest::initTestCase()
{
    QVERIFY( m_root.isValid() );

    QStandardPaths::setTestModeEnabled( true );
    QCoreApplication::setOrganizationName( QStringLiteral( "gcala" ) );
    QCoreApplication::setApplicationName( QStringLiteral( "grooveoff-cachetest" ) );

    // Start from nothing: the singleton reads the path in its constructor, so
    // the file has to go before anything touches it.
    const QString &path = QStandardPaths::writableLocation( QStandardPaths::AppDataLocation )
                          + QLatin1String( "/grooveoff.db" );
    QFile::remove( path );
    QFile::remove( path + QLatin1String( "-wal" ) );
    QFile::remove( path + QLatin1String( "-shm" ) );

    QVERIFY2( The::database()->isOpen(), qPrintable( The::database()->errorString() ) );

    m_cacheRoot = m_root.filePath( QLatin1String( "cache" ) );

    // Offline, as everywhere else here: a test run must never announce anything
    // to anybody.
    QVERIFY( The::torrentSession()->start( m_cacheRoot, TorrentSession::Offline ) );

    buildAwakeTorrent();
    buildDormantTorrent();

    m_playing = QStringLiteral( "cccc000000000000000000000000000000000003" );
    QVERIFY( The::catalog()->addTorrent( m_playing,
                                         QLatin1String( "magnet:?xt=urn:btih:" ) + m_playing,
                                         QStringLiteral( "Playing" ) ) );
    QVERIFY( The::catalog()->addTracks( { makeTrack( m_playing, 0, QStringLiteral( "Playing/01.mp3" ) ) } ) );
}

void CacheManagerTest::cleanupTestCase()
{
    The::torrentSession()->shutdown();
}

void CacheManagerTest::buildAwakeTorrent()
{
    const QString staging = m_root.filePath( QLatin1String( "staging" ) );
    const QString album   = staging + QLatin1String( "/Awake Album" );
    QVERIFY( QDir().mkpath( album ) );

    QFile source( album + QLatin1String( "/01 - prova.mp3" ) );
    QVERIFY( source.open( QIODevice::WriteOnly ) );
    source.write( QByteArray( 64 * 1024, 'a' ) );
    source.close();

    std::vector<lt::create_file_entry> files = lt::list_files( album.toStdString() );
    lt::create_torrent torrent( std::move( files ), 16 * 1024 );

    lt::error_code error;
    lt::set_piece_hashes( torrent, staging.toStdString(), error );
    QVERIFY( !error );

    const std::vector<char> buffer = torrent.generate_buf();

    const QString torrentFile = m_root.filePath( QLatin1String( "awake.torrent" ) );
    QFile file( torrentFile );
    QVERIFY( file.open( QIODevice::WriteOnly ) );
    file.write( buffer.data(), qint64( buffer.size() ) );
    file.close();

    const lt::add_torrent_params parsed = lt::load_torrent_file( torrentFile.toStdString() );
    const lt::sha1_hash hash = parsed.ti->info_hashes().get_best();
    m_awake = QString::fromLatin1( QByteArray( hash.data(), int( hash.size() ) ).toHex() );

    // The content goes where the session will look for it, so there is
    // something real for the eviction to delete.
    const QString target = m_cacheRoot + QLatin1String( "/torrents/" ) + m_awake + QLatin1String( "/Awake Album" );
    QVERIFY( QDir().mkpath( target ) );
    QVERIFY( QFile::copy( album + QLatin1String( "/01 - prova.mp3" ),
                          target + QLatin1String( "/01 - prova.mp3" ) ) );

    QSignalSpy metadata( The::torrentSession(), &TorrentSession::metadataReady );
    QString addError;
    QCOMPARE( The::torrentSession()->addTorrentFile( torrentFile, &addError ), m_awake );
    QVERIFY2( metadata.wait( 5000 ), qPrintable( addError ) );
    QVERIFY( The::torrentSession()->isActive( m_awake ) );

    QVERIFY( The::catalog()->addTorrent( m_awake,
                                         The::torrentSession()->magnetUri( m_awake ),
                                         QStringLiteral( "Awake Album" ) ) );

    const QList<TrackInfoPtr> tracks = { makeTrack( m_awake, 0, QStringLiteral( "Awake Album/01 - prova.mp3" ) ) };
    QVERIFY( The::catalog()->addTracks( tracks ) );

    // And here is what the whole test is about: a track of this torrent, put in
    // a playlist by hand. Evicting the torrent must not take it away.
    m_customId = The::playlistStore()->createCustomPlaylist( QStringLiteral( "Favourites" ) );
    QVERIFY( m_customId > 0 );
    QVERIFY( The::playlistStore()->appendTracks( m_customId, tracks ) );
}

void CacheManagerTest::buildDormantTorrent()
{
    m_dormant = QStringLiteral( "bbbb000000000000000000000000000000000002" );

    QVERIFY( The::catalog()->addTorrent( m_dormant,
                                         QLatin1String( "magnet:?xt=urn:btih:" ) + m_dormant,
                                         QStringLiteral( "Dormant Album" ) ) );
    QVERIFY( The::catalog()->addTracks( { makeTrack( m_dormant, 0, QStringLiteral( "Dormant Album/01.mp3" ) ) } ) );

    // A directory and a resume blob left behind by an earlier run, which is
    // exactly what a torrent that was never reopened looks like on disk.
    const QString directory = QDir( The::torrentSession()->torrentsPath() ).absoluteFilePath( m_dormant );
    QVERIFY( QDir().mkpath( directory ) );

    QFile data( directory + QLatin1String( "/01.mp3" ) );
    QVERIFY( data.open( QIODevice::WriteOnly ) );
    data.write( QByteArray( 32 * 1024, 'b' ) );
    data.close();

    QFile resume( QDir( The::torrentSession()->resumePath() )
                  .absoluteFilePath( m_dormant + QLatin1String( ".resume" ) ) );
    QVERIFY( resume.open( QIODevice::WriteOnly ) );
    resume.write( QByteArray( 128, 'r' ) );
    resume.close();

    QVERIFY( The::catalog()->setHasResume( m_dormant, true ) );
}

void CacheManagerTest::countsWhatTheCatalogueHolds()
{
    // The bytes libtorrent reports as done, not the size the files claim: they
    // are sparse and the filesystem would count the holes as content.
    QVERIFY( The::catalog()->setBytesDone( m_awake, 10 * 1024 * 1024 ) );
    QVERIFY( The::catalog()->setBytesDone( m_dormant, 10 * 1024 * 1024 ) );
    QVERIFY( The::catalog()->setBytesDone( m_playing, 10 * 1024 * 1024 ) );

    QCOMPARE( The::cacheManager()->usage(), qint64( 30 ) * 1024 * 1024 );

    setLimit( 100 );
    QCOMPARE( The::cacheManager()->limit(), qint64( 100 ) * 1024 * 1024 );

    // Nothing to do: a cache under its ceiling never touches anything.
    The::cacheManager()->enforceLimit();
    QCOMPARE( The::cacheManager()->usage(), qint64( 30 ) * 1024 * 1024 );

    // Zero means no ceiling at all, which is not the same as a ceiling of zero.
    setLimit( 0 );
    QCOMPARE( The::cacheManager()->limit(), qint64( 0 ) );
    The::cacheManager()->enforceLimit();
    QCOMPARE( The::cacheManager()->usage(), qint64( 30 ) * 1024 * 1024 );
}

void CacheManagerTest::evictsTheLeastRecentlyUsedFirst()
{
    // last_access is in seconds, so the three touches have to fall in different
    // ones for the order to mean anything. It is the only wait in the test.
    QVERIFY( The::catalog()->touch( m_awake ) );
    QTest::qWait( 1100 );
    QVERIFY( The::catalog()->touch( m_dormant ) );
    QTest::qWait( 1100 );
    QVERIFY( The::catalog()->touch( m_playing ) );

    The::cacheManager()->setPlayingTorrent( m_playing );

    QSignalSpy evicted( The::cacheManager(), &CacheManager::evicted );

    // 25 MB of ceiling against 30 held: one torrent has to go, and it is the
    // one that was played longest ago.
    setLimit( 25 );
    The::cacheManager()->enforceLimit();

    QCOMPARE( evicted.count(), 1 );
    QCOMPARE( evicted.first().at( 0 ).toString(), m_awake );

    QCOMPARE( The::cacheManager()->usage(), qint64( 20 ) * 1024 * 1024 );
}

void CacheManagerTest::evictingIsNotRemoving()
{
    Catalog *catalog = The::catalog();

    // The bytes are gone.
    QVERIFY( !catalog->hasResume( m_awake ) );

    // Everything else is not. The torrent is still known, its track is still in
    // the catalogue, and — the one that matters — it is still in the playlist
    // the user built by hand.
    QVERIFY( catalog->isKnown( m_awake ) );

    const QList<TrackInfoPtr> tracks = catalog->tracks( m_awake );
    QCOMPARE( tracks.count(), 1 );
    QCOMPARE( tracks.first()->filePath(), QLatin1String( "Awake Album/01 - prova.mp3" ) );

    const QList<TrackInfoPtr> inPlaylist = The::playlistStore()->tracks( m_customId );
    QCOMPARE( inPlaylist.count(), 1 );
    QCOMPARE( inPlaylist.first()->infoHash(), m_awake );
}

void CacheManagerTest::deletesTheFilesAndTheResume()
{
    // libtorrent deletes the files of a torrent it holds on its own thread, so
    // this is the one thing here that has to be waited for rather than read.
    const QString directory = QDir( The::torrentSession()->torrentsPath() ).absoluteFilePath( m_awake );
    QTRY_VERIFY_WITH_TIMEOUT( !QDir( directory ).exists(), 10000 );

    const QString resumeFile = QDir( The::torrentSession()->resumePath() )
                               .absoluteFilePath( m_awake + QLatin1String( ".resume" ) );
    QVERIFY( !QFile::exists( resumeFile ) );

    // And it is out of the session, so it announces to nobody until somebody
    // asks for it again.
    QVERIFY( !The::torrentSession()->isActive( m_awake ) );
}

void CacheManagerTest::neverEvictsWhatIsPlaying()
{
    // A ceiling below what the playing torrent alone holds: everything else
    // goes and it stays, because stopping the music to free disk is not a
    // trade anybody would accept.
    setLimit( 1 );
    The::cacheManager()->enforceLimit();

    QCOMPARE( The::cacheManager()->usage(), qint64( 10 ) * 1024 * 1024 );

    // The dormant one went the other way: nobody deleted its files but us.
    QVERIFY( !QDir( QDir( The::torrentSession()->torrentsPath() ).absoluteFilePath( m_dormant ) ).exists() );
    QVERIFY( !QFile::exists( QDir( The::torrentSession()->resumePath() )
                             .absoluteFilePath( m_dormant + QLatin1String( ".resume" ) ) ) );
    QVERIFY( The::catalog()->isKnown( m_dormant ) );
}

void CacheManagerTest::suspendsThePrefetchNearTheCeiling()
{
    // Prefetched bytes are a guess. A cache with room pays for guesses, a cache
    // close to its ceiling does not — and it holds back a little early, because
    // reacting once the ceiling is reached means the prefetch has already
    // caused the eviction it was meant to avoid.
    setLimit( 100 );
    QVERIFY( The::cacheManager()->hasRoomForPrefetch() );

    setLimit( 11 );
    QVERIFY( !The::cacheManager()->hasRoomForPrefetch() );

    // No ceiling, no reason to hold anything back.
    setLimit( 0 );
    QVERIFY( The::cacheManager()->hasRoomForPrefetch() );
}

void CacheManagerTest::emptiesTheCache()
{
    // Nothing is playing on the way out, so nothing is spared.
    The::cacheManager()->setPlayingTorrent( QString() );
    The::cacheManager()->emptyCache();

    QCOMPARE( The::cacheManager()->usage(), qint64( 0 ) );

    // Not one row fewer than before, in either table.
    QCOMPARE( The::catalog()->knownTorrents().count(), 3 );
    QCOMPARE( The::playlistStore()->tracks( m_customId ).count(), 1 );
}

/*!
  \brief discardingOneFileKeepsTheCatalogue

  Throwing away the bytes of one track, which is what removing it from a
  playlist now does. Two things are asked, and the second is the one with teeth.

  That the catalogue is untouched — the torrent still known, the file still a
  row, the list made by hand still holding it — is the rule this whole class is
  built around, and it is the same rule as evict().

  That the byte count **stays down** is the other one, and it is what tells this
  implementation apart from the obvious wrong one. bytes_done is written from
  libtorrent's own figure once a second: an implementation that unlinks the file
  behind libtorrent's back leaves it believing it still has those pieces, the
  next status update puts the old number straight back, and the cache spends
  the following months evicting other people's music to make room that already
  exists. Two seconds of watching is enough to catch it.
*/
void CacheManagerTest::discardingOneFileKeepsTheCatalogue()
{
    // A torrent of its own, because everything above has been evicted by now.
    const QString staging = m_root.filePath( QLatin1String( "discard" ) );
    const QString album   = staging + QLatin1String( "/Discard Album" );
    QVERIFY( QDir().mkpath( album ) );

    QFile one( album + QLatin1String( "/01 - one.mp3" ) );
    QVERIFY( one.open( QIODevice::WriteOnly ) );
    one.write( QByteArray( 64 * 1024, 'a' ) );
    one.close();

    QFile two( album + QLatin1String( "/02 - two.mp3" ) );
    QVERIFY( two.open( QIODevice::WriteOnly ) );
    two.write( QByteArray( 64 * 1024, 'b' ) );
    two.close();

    std::vector<lt::create_file_entry> files = lt::list_files( album.toStdString() );
    lt::create_torrent torrent( std::move( files ), 16 * 1024 );

    lt::error_code error;
    lt::set_piece_hashes( torrent, staging.toStdString(), error );
    QVERIFY( !error );

    const std::vector<char> buffer = torrent.generate_buf();
    const QString torrentFile = m_root.filePath( QLatin1String( "discard.torrent" ) );
    QFile file( torrentFile );
    QVERIFY( file.open( QIODevice::WriteOnly ) );
    file.write( buffer.data(), qint64( buffer.size() ) );
    file.close();

    const lt::add_torrent_params parsed = lt::load_torrent_file( torrentFile.toStdString() );
    const lt::sha1_hash hash = parsed.ti->info_hashes().get_best();
    const QString infoHash = QString::fromLatin1( QByteArray( hash.data(), int( hash.size() ) ).toHex() );

    const QString target = m_cacheRoot + QLatin1String( "/torrents/" ) + infoHash
                           + QLatin1String( "/Discard Album" );
    QVERIFY( QDir().mkpath( target ) );
    QVERIFY( QFile::copy( album + QLatin1String( "/01 - one.mp3" ),
                          target + QLatin1String( "/01 - one.mp3" ) ) );
    QVERIFY( QFile::copy( album + QLatin1String( "/02 - two.mp3" ),
                          target + QLatin1String( "/02 - two.mp3" ) ) );

    QSignalSpy metadata( The::torrentSession(), &TorrentSession::metadataReady );
    QString addError;
    QCOMPARE( The::torrentSession()->addTorrentFile( torrentFile, &addError ), infoHash );
    QVERIFY2( metadata.wait( 5000 ), qPrintable( addError ) );

    // The indexes are looked up: a hybrid layout puts pad files between the
    // real ones, so the second track is not file 1.
    int goneIndex = -1;
    int keptIndex = -1;
    const QList<TrackInfoPtr> published = metadata.first().at( 2 ).value<QList<TrackInfoPtr>>();
    for( const TrackInfoPtr &track : published ) {
        if( track->filePath().endsWith( QLatin1String( "01 - one.mp3" ) ) )
            goneIndex = track->fileIndex();
        else if( track->filePath().endsWith( QLatin1String( "02 - two.mp3" ) ) )
            keptIndex = track->fileIndex();
    }
    QVERIFY( goneIndex >= 0 );
    QVERIFY( keptIndex >= 0 );

    QTRY_VERIFY_WITH_TIMEOUT( The::torrentSession()->havePiece( infoHash, 0 ), 15000 );

    // The wiring MainWindow does, reproduced here because it is the mechanism
    // under test: libtorrent reports its own figure once a second and it lands
    // in the column. Without this the count could only ever be what the test
    // wrote, and the assertion that matters would prove nothing.
    const QMetaObject::Connection wiring =
        connect( The::torrentSession(), &TorrentSession::statsUpdated,
                 []( const QString &hash, int, int, qint64, qint64 bytes ) {
                     The::catalog()->setBytesDone( hash, bytes );
                 } );

    QVERIFY( The::catalog()->addTorrent( infoHash, The::torrentSession()->magnetUri( infoHash ),
                                         QStringLiteral( "Discard Album" ) ) );

    const QList<TrackInfoPtr> tracks = {
        makeTrack( infoHash, goneIndex, QStringLiteral( "Discard Album/01 - one.mp3" ) ),
        makeTrack( infoHash, keptIndex, QStringLiteral( "Discard Album/02 - two.mp3" ) )
    };
    QVERIFY( The::catalog()->addTracks( tracks ) );
    QVERIFY( The::playlistStore()->appendTracks( m_customId, tracks ) );

    // Whatever libtorrent has settled on, before anything is thrown away.
    QTRY_VERIFY_WITH_TIMEOUT( The::catalog()->bytesDone( infoHash ) > 0, 15000 );
    const qint64 before = The::catalog()->bytesDone( infoHash );

    QVERIFY( The::cacheManager()->discardFiles( infoHash, { goneIndex } ) );

    const qint64 after = The::catalog()->bytesDone( infoHash );
    QVERIFY2( after < before, "the byte count did not come down" );

    // The catalogue is untouched: the row, the file and the list made by hand.
    QVERIFY( The::catalog()->isKnown( infoHash ) );
    QCOMPARE( The::catalog()->tracks( infoHash ).count(), 2 );
    QCOMPARE( The::playlistStore()->tracks( m_customId ).count(), 3 );

    // And with the torrent back in the session, the figure stays down.
    QTest::qWait( 2500 );
    const qint64 settled = The::catalog()->bytesDone( infoHash );

    disconnect( wiring );

    QVERIFY2( settled <= after,
              "libtorrent put the old byte count back: the piece map was not corrected" );
}

QTEST_MAIN( CacheManagerTest )

#include "CacheManagerTest.moc"
