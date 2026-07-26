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


#include "torrent/StreamServer.h"
#include "torrent/TorrentSession.h"

#include <QDir>
#include <QFile>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTcpSocket>
#include <QTemporaryDir>
#include <QtTest>

#include <libtorrent/create_torrent.hpp>
#include <libtorrent/error_code.hpp>
#include <libtorrent/load_torrent.hpp>
#include <libtorrent/torrent_info.hpp>

#include <vector>

namespace lt = libtorrent;

/**
  * @class StreamServerTest
  * @short The http side of streaming, told with a torrent whose bytes are all there.
  *
  * The trick that makes this possible offline: the content is written straight
  * into the cache directory the session will use as its save path, so
  * libtorrent checks the files, finds every piece, and have_piece is true from
  * the start. What is left to prove is then exactly the server — ranges, the
  * status codes, and the two things that keep the surface closed.
  */
class StreamServerTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();

    void listensOnTheLoopbackOnly();
    void tellsWhenATrackCanBePlayed();
    void neverLetsThePrefetchOutrankWhatIsPlaying();
    void servesTheWholeFile();
    void servesARange();
    void servesASuffixRange();
    void answersHead();
    void refusesAnUnsatisfiableRange();
    void refusesABadToken();
    void refusesAForeignHost();
    void refusesAnUnknownTorrent();

private:
    /** one request, one response, driven by the event loop so the server can run */
    QByteArray http( const QByteArray &request );

    QByteArray requestFor( const QByteArray &method, const QByteArray &target, const QByteArray &host,
                           const QByteArray &extraHeaders = QByteArray() ) const;

    static QByteArray headOf( const QByteArray &response );
    static QByteArray bodyOf( const QByteArray &response );

    QTemporaryDir m_root;
    QString       m_cacheRoot;
    QString       m_infoHash;
    QByteArray    m_content;
    QUrl          m_url;
};

void StreamServerTest::initTestCase()
{
    QVERIFY( m_root.isValid() );

    QCoreApplication::setOrganizationName( QLatin1String( "gcala" ) );
    QCoreApplication::setApplicationName( QLatin1String( "grooveoff-streamtest" ) );
    QStandardPaths::setTestModeEnabled( true );

    m_cacheRoot = m_root.filePath( QLatin1String( "cache" ) );

    // A file of several pieces, filled with something a wrong byte would show
    // up in. 300 kB over 16 kB pieces is nineteen of them.
    m_content.resize( 300 * 1024 );
    for( int i = 0; i < m_content.size(); ++i )
        m_content[i] = char( ( i * 31 + i / 251 ) & 0xff );

    const QString staging = m_root.filePath( QLatin1String( "staging" ) );
    const QString album   = staging + QLatin1String( "/Test Album" );
    QVERIFY( QDir().mkpath( album ) );

    QFile source( album + QLatin1String( "/01 - prova.mp3" ) );
    QVERIFY( source.open( QIODevice::WriteOnly ) );
    QCOMPARE( source.write( m_content ), qint64( m_content.size() ) );
    source.close();

    std::vector<lt::create_file_entry> files = lt::list_files( album.toStdString() );
    lt::create_torrent torrent( std::move( files ), 16 * 1024 );

    lt::error_code error;
    lt::set_piece_hashes( torrent, staging.toStdString(), error );
    QVERIFY( !error );

    const std::vector<char> buffer = torrent.generate_buf();

    const QString torrentFile = m_root.filePath( QLatin1String( "prova.torrent" ) );
    QFile file( torrentFile );
    QVERIFY( file.open( QIODevice::WriteOnly ) );
    file.write( buffer.data(), qint64( buffer.size() ) );
    file.close();

    const lt::add_torrent_params parsed = lt::load_torrent_file( torrentFile.toStdString() );
    const lt::sha1_hash hash = parsed.ti->info_hashes().get_best();
    m_infoHash = QString::fromLatin1( QByteArray( hash.data(), int( hash.size() ) ).toHex() );

    // The content goes where the session will look for it, so that the files
    // check out at once and every piece is readable without a single peer.
    const QString target = m_cacheRoot + QLatin1String( "/torrents/" ) + m_infoHash + QLatin1String( "/Test Album" );
    QVERIFY( QDir().mkpath( target ) );
    QVERIFY( QFile::copy( album + QLatin1String( "/01 - prova.mp3" ),
                          target + QLatin1String( "/01 - prova.mp3" ) ) );

    QVERIFY( The::torrentSession()->start( m_cacheRoot, TorrentSession::Offline ) );

    QSignalSpy metadata( The::torrentSession(), &TorrentSession::metadataReady );
    QString addError;
    QCOMPARE( The::torrentSession()->addTorrentFile( torrentFile, &addError ), m_infoHash );
    QVERIFY2( metadata.wait( 5000 ), qPrintable( addError ) );

    // The check of the files takes a moment, and until it is done nothing is
    // readable: this is the only wait in the whole test.
    QTRY_VERIFY_WITH_TIMEOUT( The::torrentSession()->havePiece( m_infoHash, 0 ), 15000 );
    QTRY_VERIFY_WITH_TIMEOUT( The::torrentSession()->havePiece( m_infoHash, 18 ), 15000 );

    QVERIFY( The::streamServer()->start() );
    QVERIFY( The::streamServer()->isRunning() );

    m_url = The::streamServer()->urlFor( m_infoHash, 0 );
    QVERIFY( !m_url.isEmpty() );
}

void StreamServerTest::cleanupTestCase()
{
    The::streamServer()->stop();
    The::torrentSession()->shutdown();
}

QByteArray StreamServerTest::requestFor( const QByteArray &method, const QByteArray &target,
                                         const QByteArray &host, const QByteArray &extraHeaders ) const
{
    return method + " " + target + " HTTP/1.1\r\n"
           "Host: " + host + "\r\n"
           + extraHeaders +
           "\r\n";
}

QByteArray StreamServerTest::http( const QByteArray &request )
{
    QTcpSocket socket;
    QByteArray response;
    bool done = false;

    // waitForReadyRead would be the short way and the wrong one: it blocks on
    // this socket alone, and the server lives in the same thread, so it would
    // never get to answer.
    connect( &socket, &QTcpSocket::connected, &socket, [&socket, request] { socket.write( request ); } );
    connect( &socket, &QTcpSocket::readyRead, &socket, [&socket, &response] { response += socket.readAll(); } );
    connect( &socket, &QTcpSocket::disconnected, &socket, [&done] { done = true; } );

    socket.connectToHost( QHostAddress::LocalHost, The::streamServer()->port() );

    const int timeout = 10000;
    QElapsedTimer clock;
    clock.start();
    while( !done && clock.elapsed() < timeout )
        QTest::qWait( 10 );

    response += socket.readAll();
    return response;
}

QByteArray StreamServerTest::headOf( const QByteArray &response )
{
    const int end = response.indexOf( "\r\n\r\n" );
    return end < 0 ? response : response.left( end );
}

QByteArray StreamServerTest::bodyOf( const QByteArray &response )
{
    const int end = response.indexOf( "\r\n\r\n" );
    return end < 0 ? QByteArray() : response.mid( end + 4 );
}

void StreamServerTest::listensOnTheLoopbackOnly()
{
    QTcpSocket socket;
    socket.connectToHost( QHostAddress::LocalHost, The::streamServer()->port() );
    QVERIFY( socket.waitForConnected( 3000 ) );
    QCOMPARE( socket.peerAddress(), QHostAddress( QHostAddress::LocalHost ) );
    socket.abort();
}

void StreamServerTest::tellsWhenATrackCanBePlayed()
{
    // The gate in front of the player: handing ffmpeg a url before the head of
    // the file is there gets an i/o error rather than a wait, and the track
    // never starts at all.
    QVERIFY( The::streamServer()->isPlayable( m_infoHash, 0 ) );
    QVERIFY( !The::streamServer()->isPlayable( QString( 40, QLatin1Char( 'a' ) ), 0 ) );
    QVERIFY( !The::streamServer()->isPlayable( m_infoHash, 99 ) );

    // Nothing has asked for this track yet, so it is where every file of a
    // torrent rests: at dont_download. Adding a torrent costs the metadata and
    // nothing else.
    QCOMPARE( The::torrentSession()->filePriority( m_infoHash, 0 ), TorrentSession::SkipFile );

    // A track whose head is already in the cache is playable at once, and
    // saying so has to be immediate: waiting for a piece that has already
    // arrived would leave it silent for ever.
    QSignalSpy playable( The::streamServer(), &StreamServer::trackPlayable );
    The::streamServer()->prepare( m_infoHash, 0 );

    QCOMPARE( playable.count(), 1 );
    QCOMPARE( playable.first().at( 0 ).toString(), m_infoHash );
    QCOMPARE( playable.first().at( 1 ).toInt(), 0 );
}

void StreamServerTest::neverLetsThePrefetchOutrankWhatIsPlaying()
{
    TorrentSession *session = The::torrentSession();

    // Setting a priority is asynchronous — the call is handed to libtorrent's
    // own thread — so every one of these is waited for rather than read back
    // on the next line.
    The::streamServer()->cancelPrefetch();
    The::streamServer()->cancelPreparation();

    // Taken in the background it is wanted, but only with the bandwidth that
    // is left over.
    The::streamServer()->prefetch( m_infoHash, 0 );
    QTRY_COMPARE( session->filePriority( m_infoHash, 0 ), TorrentSession::NormalFile );

    // And when it becomes the track being listened to it is promoted, rather
    // than keeping the modest priority it was given as a spare. Getting this
    // one wrong gives a track that stutters while the network is fine.
    The::streamServer()->cancelPrefetch();
    The::streamServer()->prepare( m_infoHash, 0 );
    QTRY_COMPARE( session->filePriority( m_infoHash, 0 ), TorrentSession::TopFile );

    The::streamServer()->cancelPreparation();
}

void StreamServerTest::servesTheWholeFile()
{
    const QByteArray host = "127.0.0.1:" + QByteArray::number( The::streamServer()->port() );
    const QByteArray response = http( requestFor( "GET", m_url.toEncoded( QUrl::RemoveScheme | QUrl::RemoveAuthority ), host ) );

    const QByteArray head = headOf( response );
    QVERIFY2( head.startsWith( "HTTP/1.1 200 OK" ), head.constData() );
    QVERIFY( head.contains( "Accept-Ranges: bytes" ) );
    QVERIFY( head.contains( "Content-Type: audio/mpeg" ) );
    QVERIFY( head.contains( "Content-Length: " + QByteArray::number( m_content.size() ) ) );

    QCOMPARE( bodyOf( response ), m_content );
}

void StreamServerTest::servesARange()
{
    const QByteArray host = "127.0.0.1:" + QByteArray::number( The::streamServer()->port() );

    // Across a piece boundary on purpose: this is the seek that a player does,
    // and the piece grid must not show through.
    const QByteArray response = http( requestFor( "GET", m_url.toEncoded( QUrl::RemoveScheme | QUrl::RemoveAuthority ),
                                                  host, "Range: bytes=16000-49999\r\n" ) );

    const QByteArray head = headOf( response );
    QVERIFY2( head.startsWith( "HTTP/1.1 206 Partial Content" ), head.constData() );
    QVERIFY2( head.contains( "Content-Range: bytes 16000-49999/" + QByteArray::number( m_content.size() ) ),
              head.constData() );
    QVERIFY( head.contains( "Content-Length: 34000" ) );

    QCOMPARE( bodyOf( response ), m_content.mid( 16000, 34000 ) );
}

void StreamServerTest::servesASuffixRange()
{
    const QByteArray host = "127.0.0.1:" + QByteArray::number( The::streamServer()->port() );
    const QByteArray response = http( requestFor( "GET", m_url.toEncoded( QUrl::RemoveScheme | QUrl::RemoveAuthority ),
                                                  host, "Range: bytes=-2048\r\n" ) );

    QVERIFY( headOf( response ).startsWith( "HTTP/1.1 206 Partial Content" ) );
    QCOMPARE( bodyOf( response ), m_content.right( 2048 ) );
}

void StreamServerTest::answersHead()
{
    const QByteArray host = "127.0.0.1:" + QByteArray::number( The::streamServer()->port() );
    const QByteArray response = http( requestFor( "HEAD", m_url.toEncoded( QUrl::RemoveScheme | QUrl::RemoveAuthority ), host ) );

    QVERIFY( headOf( response ).startsWith( "HTTP/1.1 200 OK" ) );
    QVERIFY( headOf( response ).contains( "Content-Length: " + QByteArray::number( m_content.size() ) ) );
    QVERIFY( bodyOf( response ).isEmpty() );
}

void StreamServerTest::refusesAnUnsatisfiableRange()
{
    const QByteArray host = "127.0.0.1:" + QByteArray::number( The::streamServer()->port() );
    const QByteArray response = http( requestFor( "GET", m_url.toEncoded( QUrl::RemoveScheme | QUrl::RemoveAuthority ),
                                                  host, "Range: bytes=999999999-\r\n" ) );

    const QByteArray head = headOf( response );
    QVERIFY2( head.startsWith( "HTTP/1.1 416 Range Not Satisfiable" ), head.constData() );
    QVERIFY( head.contains( "Content-Range: bytes */" + QByteArray::number( m_content.size() ) ) );
}

void StreamServerTest::refusesABadToken()
{
    const QByteArray host = "127.0.0.1:" + QByteArray::number( The::streamServer()->port() );
    const QByteArray target = "/" + m_infoHash.toLatin1() + "/0?t=wrong";

    QVERIFY( headOf( http( requestFor( "GET", target, host ) ) ).startsWith( "HTTP/1.1 404" ) );

    // And with no token at all.
    QVERIFY( headOf( http( requestFor( "GET", "/" + m_infoHash.toLatin1() + "/0", host ) ) ).startsWith( "HTTP/1.1 404" ) );
}

void StreamServerTest::refusesAForeignHost()
{
    // What a page doing dns rebinding would send: its own name, resolved to
    // 127.0.0.1. The port matches, the name does not, and that is enough.
    const QByteArray host = "music.example.com:" + QByteArray::number( The::streamServer()->port() );
    const QByteArray response = http( requestFor( "GET", m_url.toEncoded( QUrl::RemoveScheme | QUrl::RemoveAuthority ), host ) );

    QVERIFY2( headOf( response ).startsWith( "HTTP/1.1 404" ), headOf( response ).constData() );
}

void StreamServerTest::refusesAnUnknownTorrent()
{
    const QByteArray host = "127.0.0.1:" + QByteArray::number( The::streamServer()->port() );
    const QString unknown = QString( 40, QLatin1Char( 'a' ) );
    const QUrl url = The::streamServer()->urlFor( unknown, 0 );

    QVERIFY( headOf( http( requestFor( "GET", url.toEncoded( QUrl::RemoveScheme | QUrl::RemoveAuthority ), host ) ) )
             .startsWith( "HTTP/1.1 404" ) );
}

QTEST_MAIN( StreamServerTest )

#include "StreamServerTest.moc"
