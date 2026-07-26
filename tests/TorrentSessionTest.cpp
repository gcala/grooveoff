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
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QtTest>

#include <libtorrent/create_torrent.hpp>
#include <libtorrent/error_code.hpp>

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

private:
    /** builds a two file torrent out of a directory made for the occasion */
    QString buildTorrentFile();

    QTemporaryDir m_root;
    QString       m_cacheRoot;
    QString       m_seedRoot;
    QString       m_torrentFile;
    QString       m_infoHash;
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

QTEST_MAIN( TorrentSessionTest )

#include "TorrentSessionTest.moc"
