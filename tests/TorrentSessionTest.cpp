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


#include "torrent/TorrentSession.h"

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QSet>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QtTest>

#include <libtorrent/add_torrent_params.hpp>
#include <libtorrent/create_torrent.hpp>
#include <libtorrent/error_code.hpp>
#include <libtorrent/load_torrent.hpp>
#include <libtorrent/read_resume_data.hpp>
#include <libtorrent/torrent_info.hpp>

#include <vector>

namespace lt = libtorrent;

/**
  * @class TorrentSessionTest
  * @short One torrent followed from the .torrent file to a restart with no network.
  *
  * The functions run in order and share their state, as in DatabaseTest: it is
  * one story, not a set of cases. The session runs in Offline mode throughout,
  * so a test run never announces anything to anybody, and the torrent is built
  * here rather than fetched, so the whole scenario holds without a swarm.
  */
class TorrentSessionTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();

    void startsAndPreparesItsCache();
    void neverUploads();
    void refusesAMalformedMagnet();
    void listsEveryFileOfATorrent();
    void downloadsNothingBeforeATrackIsAsked();
    void savesResumeDataOnShutdown();
    void reopensWithoutTheTorrentFile();
    void keepsThePrioritiesItWasGivenAtTheAdd();
    void dropsOneFileAndKeepsTheOther();
    void libtorrentAgreesTheBytesAreGone();

private:
    /** builds a two file torrent out of a directory made for the occasion */
    QString buildTorrentFile();

    /** the same, with its content already where the session will look for it */
    void buildFullTorrent();

    QTemporaryDir m_root;
    QString       m_cacheRoot;
    QString       m_seedRoot;
    QString       m_torrentFile;
    QString       m_infoHash;

    /** the second torrent, the one that actually holds its bytes */
    QString       m_fullHash;
    QString       m_keptFile;
    QByteArray    m_keptContent;
    int           m_goneIndex = -1;
    int           m_keptIndex = -1;
};

void TorrentSessionTest::initTestCase()
{
    QVERIFY( m_root.isValid() );

    QCoreApplication::setOrganizationName( QLatin1String( "gcala" ) );
    QCoreApplication::setApplicationName( QLatin1String( "grooveoff-torrenttest" ) );
    QStandardPaths::setTestModeEnabled( true );

    m_cacheRoot = m_root.filePath( QLatin1String( "cache" ) );
    m_seedRoot  = m_root.filePath( QLatin1String( "seed" ) );

    m_torrentFile = buildTorrentFile();
    QVERIFY( !m_torrentFile.isEmpty() );
}

void TorrentSessionTest::cleanupTestCase()
{
    The::torrentSession()->shutdown();
}

QString TorrentSessionTest::buildTorrentFile()
{
    // Two files, one audio and one not: the catalogue keeps both and tells them
    // apart, so the file list has to come back with both as well.
    const QString album = m_seedRoot + QLatin1String( "/Test Album" );
    if( !QDir().mkpath( album ) )
        return QString();

    QFile track( album + QLatin1String( "/01 - prova.mp3" ) );
    if( !track.open( QIODevice::WriteOnly ) )
        return QString();
    track.write( QByteArray( 40000, 'a' ) );
    track.close();

    QFile cover( album + QLatin1String( "/cover.jpg" ) );
    if( !cover.open( QIODevice::WriteOnly ) )
        return QString();
    cover.write( QByteArray( 1000, 'b' ) );
    cover.close();

    std::vector<lt::create_file_entry> files = lt::list_files( album.toStdString() );
    lt::create_torrent torrent( std::move( files ), 16 * 1024 );

    lt::error_code error;
    lt::set_piece_hashes( torrent, m_seedRoot.toStdString(), error );
    if( error )
        return QString();

    const std::vector<char> buffer = torrent.generate_buf();

    const QString fileName = m_root.filePath( QLatin1String( "prova.torrent" ) );
    QFile file( fileName );
    if( !file.open( QIODevice::WriteOnly ) )
        return QString();
    file.write( buffer.data(), qint64( buffer.size() ) );
    file.close();

    return fileName;
}

void TorrentSessionTest::startsAndPreparesItsCache()
{
    QVERIFY( The::torrentSession()->start( m_cacheRoot, TorrentSession::Offline ) );
    QVERIFY( The::torrentSession()->isRunning() );

    QVERIFY( QDir( The::torrentSession()->torrentsPath() ).exists() );
    QVERIFY( QDir( The::torrentSession()->resumePath() ).exists() );
}

void TorrentSessionTest::neverUploads()
{
    // The regression this guards is the documented libtorrent trap: a rate
    // limit of zero means unlimited, so what has to be zero is the unchoke
    // slots and what has to be small, but not zero, is the rate limit.
    const TorrentSession::UploadPolicy policy = The::torrentSession()->uploadPolicy();
    QCOMPARE( policy.unchokeSlots, 0 );
    QCOMPARE( policy.uploadRateLimit, 1024 );
}

void TorrentSessionTest::refusesAMalformedMagnet()
{
    QString error;
    const QString infoHash = The::torrentSession()->addMagnet( QLatin1String( "not a magnet at all" ), &error );

    QVERIFY( infoHash.isEmpty() );
    QVERIFY( !error.isEmpty() );
    QVERIFY( The::torrentSession()->activeTorrents().isEmpty() );
}

void TorrentSessionTest::listsEveryFileOfATorrent()
{
    QSignalSpy metadata( The::torrentSession(), &TorrentSession::metadataReady );

    QString error;
    m_infoHash = The::torrentSession()->addTorrentFile( m_torrentFile, &error );

    QVERIFY2( !m_infoHash.isEmpty(), qPrintable( error ) );
    QCOMPARE( m_infoHash.size(), 40 );

    QVERIFY( metadata.wait( 5000 ) );
    QCOMPARE( metadata.count(), 1 );

    const QList<QVariant> arguments = metadata.first();
    QCOMPARE( arguments.at( 0 ).toString(), m_infoHash );
    QCOMPARE( arguments.at( 1 ).toString(), QLatin1String( "Test Album" ) );

    const QList<TrackInfoPtr> tracks = arguments.at( 2 ).value<QList<TrackInfoPtr>>();
    QCOMPARE( tracks.count(), 2 );

    QCOMPARE( tracks.at( 0 )->filePath(), QLatin1String( "Test Album/01 - prova.mp3" ) );
    QCOMPARE( tracks.at( 0 )->fileSize(), 40000 );
    QCOMPARE( tracks.at( 0 )->fileIndex(), 0 );
    QVERIFY( tracks.at( 0 )->isAudio() );

    QCOMPARE( tracks.at( 1 )->filePath(), QLatin1String( "Test Album/cover.jpg" ) );
    QVERIFY( !tracks.at( 1 )->isAudio() );

    // Every track carries the magnet of its torrent, or a playlist could not
    // reopen it once neither the .torrent nor the resume file is around.
    QVERIFY( tracks.at( 0 )->magnetUri().contains( m_infoHash ) );

    QVERIFY( The::torrentSession()->isActive( m_infoHash ) );
}

void TorrentSessionTest::downloadsNothingBeforeATrackIsAsked()
{
    // Adding a torrent buys the file list and nothing else. If this ever grows
    // a byte, the priorities are being set too late or not at all.
    QTest::qWait( 500 );

    const QString torrentDir = QDir( The::torrentSession()->torrentsPath() ).absoluteFilePath( m_infoHash );

    qint64 bytes = 0;
    QDirIterator files( torrentDir, QDir::Files, QDirIterator::Subdirectories );
    while( files.hasNext() ) {
        files.next();
        bytes += files.fileInfo().size();
    }

    QCOMPARE( bytes, 0 );
    QCOMPARE( The::torrentSession()->totalUpload(), 0 );
}

void TorrentSessionTest::savesResumeDataOnShutdown()
{
    The::torrentSession()->shutdown();
    QVERIFY( !The::torrentSession()->isRunning() );

    const QString resumeFile = QDir( The::torrentSession()->resumePath() )
                               .absoluteFilePath( m_infoHash + QLatin1String( ".resume" ) );

    QVERIFY2( QFile::exists( resumeFile ), "the shutdown did not wait for the resume data" );
    QVERIFY( QFileInfo( resumeFile ).size() > 0 );

    QVERIFY( QFile::exists( QDir( m_cacheRoot ).absoluteFilePath( QLatin1String( "session.dat" ) ) ) );
}

void TorrentSessionTest::reopensWithoutTheTorrentFile()
{
    // The point of save_info_dict: with the .torrent gone and the session
    // offline, the file list can only come from the resume blob on disk.
    QVERIFY( QFile::remove( m_torrentFile ) );

    QVERIFY( The::torrentSession()->start( m_cacheRoot, TorrentSession::Offline ) );
    QVERIFY( The::torrentSession()->hasResume( m_infoHash ) );

    QSignalSpy metadata( The::torrentSession(), &TorrentSession::metadataReady );

    QString error;
    QVERIFY2( The::torrentSession()->reopen( m_infoHash, QString(), &error ), qPrintable( error ) );

    QVERIFY( metadata.wait( 5000 ) );

    const QList<QVariant> arguments = metadata.first();
    QCOMPARE( arguments.at( 0 ).toString(), m_infoHash );

    const QList<TrackInfoPtr> tracks = arguments.at( 2 ).value<QList<TrackInfoPtr>>();
    QCOMPARE( tracks.count(), 2 );
    QCOMPARE( tracks.at( 0 )->filePath(), QLatin1String( "Test Album/01 - prova.mp3" ) );
}

/*!
  \brief keepsThePrioritiesItWasGivenAtTheAdd: the whole of the restart

  This is the chain that makes a half fetched album go on fetching itself after
  a restart, and it hangs on an order that is easy to get wrong and impossible
  to notice. DownloadQueue answers torrentAdded by handing out the priorities of
  the tracks somebody chose; publishing the file list sets every file of the
  torrent to dont_download, because adding a torrent must cost the metadata and
  nothing else. Announce the torrent before that and the answer is thrown away a
  line later — the fetching came back only because metadataReady happens to be
  emitted afterwards and the queue happens to listen to that too.

  So the invariant is asserted from outside, the way the queue sees it: nobody
  hears about a torrent before its files are at dont_download, and a priority
  set on hearing about it is a priority that stays.
*/
void TorrentSessionTest::keepsThePrioritiesItWasGivenAtTheAdd()
{
    TorrentSession *session = The::torrentSession();

    // A restart, the same one the previous function does: this torrent is in
    // the session and has to come back out of it to be added again.
    session->shutdown();
    QVERIFY( session->start( m_cacheRoot, TorrentSession::Offline ) );

    // Exactly what DownloadQueue does, and from the same signal.
    const QMetaObject::Connection handler =
        QObject::connect( session, &TorrentSession::torrentAdded, session,
                          [session]( const QString &infoHash ) {
        session->setFilePriority( infoHash, 0, TorrentSession::NormalFile );
    } );

    QSignalSpy metadata( session, &TorrentSession::metadataReady );

    QString error;
    QVERIFY2( session->reopen( m_infoHash, QString(), &error ), qPrintable( error ) );
    QVERIFY( metadata.wait( 5000 ) );

    QObject::disconnect( handler );

    // Waited for rather than read back: a priority is handed to libtorrent's own
    // thread, so the line after the call still sees the old value. Which is also
    // why this cannot be checked by reading once — and why, announced too early,
    // the wipe would win here and time this out.
    QTRY_COMPARE_WITH_TIMEOUT( session->filePriority( m_infoHash, 0 ),
                               TorrentSession::NormalFile, 5000 );

    // And the file nobody asked for is still where the add left it, or the
    // check above would pass on a torrent fetching the whole thing.
    QCOMPARE( session->filePriority( m_infoHash, 1 ), TorrentSession::SkipFile );
}

void TorrentSessionTest::buildFullTorrent()
{
    // Two audio files this time, and both whole on the disk before the torrent
    // is ever added: libtorrent checks them at the add and finds every piece,
    // so the whole thing works with no peer anywhere. Dropping one of them is
    // then a question about bytes that really exist.
    const QString album = m_seedRoot + QLatin1String( "/Full Album" );
    QVERIFY( QDir().mkpath( album ) );

    const QByteArray first( 40000, 'x' );
    m_keptContent = QByteArray( 40000, 'y' );

    QFile one( album + QLatin1String( "/01 - one.mp3" ) );
    QVERIFY( one.open( QIODevice::WriteOnly ) );
    one.write( first );
    one.close();

    QFile two( album + QLatin1String( "/02 - two.mp3" ) );
    QVERIFY( two.open( QIODevice::WriteOnly ) );
    two.write( m_keptContent );
    two.close();

    // v1 only, deliberately: without pad files the files share their boundary
    // pieces, which is the layout of nearly every music torrent in the world
    // and the one the piece arithmetic of a drop has to be right about.
    std::vector<lt::create_file_entry> files = lt::list_files( album.toStdString() );
    lt::create_torrent torrent( std::move( files ), 16 * 1024, lt::create_torrent::v1_only );

    lt::error_code error;
    lt::set_piece_hashes( torrent, m_seedRoot.toStdString(), error );
    QVERIFY( !error );

    const std::vector<char> buffer = torrent.generate_buf();

    const QString fileName = m_root.filePath( QLatin1String( "full.torrent" ) );
    QFile file( fileName );
    QVERIFY( file.open( QIODevice::WriteOnly ) );
    file.write( buffer.data(), qint64( buffer.size() ) );
    file.close();

    const lt::add_torrent_params parsed = lt::load_torrent_file( fileName.toStdString() );
    const lt::sha1_hash hash = parsed.ti->info_hashes().get_best();
    m_fullHash = QString::fromLatin1( QByteArray( hash.data(), int( hash.size() ) ).toHex() );

    const QString target = m_cacheRoot + QLatin1String( "/torrents/" ) + m_fullHash
                           + QLatin1String( "/Full Album" );
    QVERIFY( QDir().mkpath( target ) );
    QVERIFY( QFile::copy( album + QLatin1String( "/01 - one.mp3" ),
                          target + QLatin1String( "/01 - one.mp3" ) ) );
    QVERIFY( QFile::copy( album + QLatin1String( "/02 - two.mp3" ),
                          target + QLatin1String( "/02 - two.mp3" ) ) );

    m_keptFile = target + QLatin1String( "/02 - two.mp3" );

    QSignalSpy metadata( The::torrentSession(), &TorrentSession::metadataReady );
    QString addError;
    QCOMPARE( The::torrentSession()->addTorrentFile( fileName, &addError ), m_fullHash );
    QVERIFY2( metadata.wait( 5000 ), qPrintable( addError ) );

    // The indexes are looked up and not assumed: libtorrent lays a hybrid
    // torrent out with pad files between the real ones, so the second track is
    // not file 1. Guessing gave a test that failed against correct code.
    const QList<TrackInfoPtr> tracks = metadata.first().at( 2 ).value<QList<TrackInfoPtr>>();
    for( const TrackInfoPtr &track : tracks ) {
        if( track->filePath().endsWith( QLatin1String( "01 - one.mp3" ) ) )
            m_goneIndex = track->fileIndex();
        else if( track->filePath().endsWith( QLatin1String( "02 - two.mp3" ) ) )
            m_keptIndex = track->fileIndex();
    }

    QVERIFY( m_goneIndex >= 0 );
    QVERIFY( m_keptIndex >= 0 );

    // The check takes a moment, and until it is done there is nothing to drop.
    // Asked of the live handle and not of cachedFile(), which reads the resume
    // blob: a torrent that got its bytes from a check rather than from a
    // download has not written one yet, which is precisely the case dropFiles()
    // has to cope with.
    QTRY_VERIFY_WITH_TIMEOUT( The::torrentSession()->havePiece( m_fullHash, 0 ), 15000 );
    QTRY_VERIFY_WITH_TIMEOUT( The::torrentSession()->havePiece( m_fullHash, 4 ), 15000 );
}

/*!
  \brief dropsOneFileAndKeepsTheOther

  The per-file removal, asked of the layer that has to do it. libtorrent has no
  such operation — a torrent is the unit of everything destructive it offers —
  so what is being checked is that the assembly built out of what it does offer
  arrives at the right place: one file gone from the disk, the other intact byte
  for byte, and the piece map agreeing with both.

  The second half is the one that matters and the one an obvious implementation
  gets wrong: a file left claimed in the map is a file that will be believed
  present next time and played as silence.
*/
void TorrentSessionTest::dropsOneFileAndKeepsTheOther()
{
    buildFullTorrent();

    const QString goneFile = The::torrentSession()->filePath( m_fullHash, m_goneIndex );
    QVERIFY( QFile::exists( goneFile ) );

    const qint64 remaining = The::torrentSession()->dropFiles( m_fullHash, { m_goneIndex } );

    // Something is left, and it is less than everything: the exact figure
    // depends on whether the layout pads its files apart, and asserting it
    // would be asserting libtorrent's business rather than ours.
    QVERIFY( remaining > 0 );
    QVERIFY( remaining < 80000 );

    QTRY_VERIFY_WITH_TIMEOUT( !QFile::exists( goneFile ), 5000 );

    // The neighbour is untouched, and compared rather than counted: a sparse
    // file is the full size whether it is whole or all holes.
    QFile kept( m_keptFile );
    QVERIFY( kept.open( QIODevice::ReadOnly ) );
    QCOMPARE( kept.readAll(), m_keptContent );

    QVERIFY( !The::torrentSession()->cachedFile( m_fullHash, m_goneIndex ).complete );

    // The neighbour's own bytes are untouched — compared above rather than
    // counted — but it is no longer *whole* as far as the piece map goes, and
    // that is the declared cost of a drop rather than a defect: the two share a
    // piece, the piece had to go, and whoever wants that file next fetches it
    // again. Claiming it instead is what left the dropped file headless.
    QVERIFY( QFile::exists( m_keptFile ) );
}

/*!
  \brief libtorrentAgreesTheBytesAreGone

  The test that stops anybody simplifying the drop back to "skip the file and
  unlink it". That version passes everything above: the file is gone, the
  neighbour is fine, and cachedFile() — which stats the disk — says the right
  thing. What it gets wrong is libtorrent's own map, and the symptom arrives
  weeks later as a track that will not play out of a swarm that is working.
*/
void TorrentSessionTest::libtorrentAgreesTheBytesAreGone()
{
    // The drop put the torrent back in the session, and that it did is half the
    // assertion: an add that libtorrent refuses as a duplicate leaves no handle,
    // and every question asked of a torrent it is not holding answers "no" —
    // which is how a first version of this test passed while the map was whole.
    QTRY_VERIFY_WITH_TIMEOUT( The::torrentSession()->isActive( m_fullHash ), 5000 );

    // The map itself, read off the disk rather than off the session, because
    // the map on the disk is what the next start will believe. Every piece
    // wholly inside the file that went has to be clear; the ones it shared with
    // the file that stays are deliberately left alone.
    const QString resumeFile = The::torrentSession()->resumePath()
                               + QLatin1Char( '/' ) + m_fullHash + QLatin1String( ".resume" );

    QFile blob( resumeFile );
    QVERIFY( blob.open( QIODevice::ReadOnly ) );
    const QByteArray stored = blob.readAll();
    QVERIFY( !stored.isEmpty() );

    lt::error_code error;
    const lt::add_torrent_params params =
        lt::read_resume_data( lt::span<const char>( stored.constData(), stored.size() ), error );
    QVERIFY( !error );
    QVERIFY( params.ti );

    const lt::file_storage &layout = params.ti->layout();
    const lt::file_index_t gone { m_goneIndex };

    const int goneFirst = int( layout.map_file( gone, 0, 0 ).piece );
    const int goneLast  = int( layout.map_file( gone, layout.file_size( gone ) - 1, 0 ).piece );

    // **Every** piece it touched, the shared ones at each end included. Those
    // are the ones that matter: a piece straddling two files is stored across
    // both, so unlinking one takes away its half, and a piece left claimed is
    // one nobody will ask for again. The head of the file lives in that first
    // shared piece, which is where the tags and the cover are — keeping it is
    // how the track came back as holes with no cover and stayed that way.
    for( int piece = goneFirst; piece <= goneLast; ++piece ) {
        QVERIFY2( piece < params.have_pieces.size(), "the resume lost its piece map" );
        QVERIFY2( !params.have_pieces[lt::piece_index_t { piece }],
                  "a piece the file touched is still claimed: its head will never come back" );
    }

    QVERIFY2( goneLast > goneFirst, "the file fits in one piece, so this proves little" );

    // And it did not throw the whole torrent away to say so: the pieces of the
    // file that stays, the shared ones apart, are still claimed — which is what
    // tells a corrected map from a full recheck.
    const lt::file_index_t kept { m_keptIndex };
    const int keptFirst = int( layout.map_file( kept, 0, 0 ).piece );
    const int keptLast  = int( layout.map_file( kept, layout.file_size( kept ) - 1, 0 ).piece );

    int stillThere = 0;
    for( int piece = keptFirst; piece <= keptLast; ++piece ) {
        if( piece >= goneFirst && piece <= goneLast )
            continue;

        QVERIFY( params.have_pieces[lt::piece_index_t { piece }] );
        ++stillThere;
    }

    QVERIFY( stillThere > 0 );
}

QTEST_MAIN( TorrentSessionTest )

#include "TorrentSessionTest.moc"
