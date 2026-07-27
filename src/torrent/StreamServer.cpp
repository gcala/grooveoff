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


#include "StreamServer.h"
#include "TorrentSession.h"
#include "TorrentTrack.h"

#include "../grooveoff/Settings.h"

#include <QDebug>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QHash>
#include <QRandomGenerator>
#include <QSettings>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTimer>

namespace The {
    static StreamServer* s_StreamServer_instance = nullptr;

    StreamServer* streamServer()
    {
        if( !s_StreamServer_instance )
            s_StreamServer_instance = new StreamServer();

        return s_StreamServer_instance;
    }
}

namespace {

/** how long a request may wait for a piece before it is told to give up */
const int readTimeoutMs = 30000;

/**
 * How much is allowed to pile up in the socket before writing stops. Without
 * this the whole track would end up in memory, which is the one thing the
 * cache exists to avoid.
 */
const qint64 socketHighWater = 4 * 1024 * 1024;

const qint64 chunkSize = 256 * 1024;

/** a request head longer than this is not a request head */
const int maxRequestBytes = 8 * 1024;

const int sweepIntervalMs = 1000;

/**
 * How long a track outlives its last reader. ffmpeg closes the connection and
 * opens another one at every seek, and letting the track die in between would
 * reset its priority and its deadlines several times a second.
 */
const int trackIdleMs = 10000;

QByteArray contentTypeFor( const QString &filePath )
{
    const QString suffix = QFileInfo( filePath ).suffix().toLower();

    if( suffix == QLatin1String( "mp3" ) )                                     return "audio/mpeg";
    if( suffix == QLatin1String( "flac" ) )                                    return "audio/flac";
    if( suffix == QLatin1String( "ogg" ) || suffix == QLatin1String( "oga" )
        || suffix == QLatin1String( "opus" ) )                                 return "audio/ogg";
    if( suffix == QLatin1String( "m4a" ) || suffix == QLatin1String( "mp4" ) ) return "audio/mp4";
    if( suffix == QLatin1String( "aac" ) )                                     return "audio/aac";
    if( suffix == QLatin1String( "wav" ) )                                     return "audio/wav";
    if( suffix == QLatin1String( "wma" ) )                                     return "audio/x-ms-wma";
    if( suffix == QLatin1String( "aiff" ) || suffix == QLatin1String( "aif" ) ) return "audio/aiff";
    if( suffix == QLatin1String( "ape" ) )                                     return "audio/x-ape";
    if( suffix == QLatin1String( "wv" ) )                                      return "audio/x-wavpack";
    if( suffix == QLatin1String( "mpc" ) )                                     return "audio/x-musepack";

    return "application/octet-stream";
}

/** one request being served, from its first byte to its last */
struct PendingRead
{
    QTcpSocket *socket = nullptr;

    QByteArray head;
    bool       responded = false;
    bool       headOnly  = false;

    QString       trackKey;
    TorrentTrack *track = nullptr;

    QFile  file;
    qint64 nextOffset = 0;
    qint64 endOffset  = -1;

    /**
     * Only a read stopped on a missing piece is on the clock. One stopped
     * because the player is paused and has stopped reading is not: timing that
     * one out would kill the stream of somebody who did nothing wrong.
     */
    bool          waitingForPiece = false;
    QElapsedTimer waiting;
};

} // namespace

class StreamServer::Private
{
public:
    explicit Private( StreamServer *server )
        : q( server )
    {}

    /** a track and the reads that hold it alive */
    struct TrackEntry
    {
        TorrentTrack *track   = nullptr;
        int           readers = 0;
        QElapsedTimer idle;
    };

    void accept();
    void readRequest( PendingRead *read );
    void serve( PendingRead *read );
    void pump( PendingRead *read );
    void finish( PendingRead *read );
    void fail( PendingRead *read, int code, const QByteArray &reason, const QByteArray &extraHeader = QByteArray() );
    void close( PendingRead *read );

    TorrentTrack* trackFor( const QString &infoHash, int fileIndex,
                            TorrentTrack::Mode mode, QString *key );
    void releaseTrack( const QString &key );

    /** emits trackPlayable and lets the preparation go once the opening is in */
    void checkPreparation();

    void sweep();

    StreamServer *q;

    QTcpServer *server = nullptr;
    QByteArray  token;

    QHash<QTcpSocket*, PendingRead*> reads;
    QHash<QString, TrackEntry>       tracks;

    /** the one track being brought up for the player, if any */
    QString preparedKey;
    QString preparedHash;
    int     preparedIndex = -1;

    /** and the one being taken in the background, at most one at a time */
    QString prefetchedKey;

    QTimer *sweepTimer = nullptr;
};

void StreamServer::Private::accept()
{
    while( server->hasPendingConnections() ) {
        QTcpSocket *socket = server->nextPendingConnection();

        PendingRead *read = new PendingRead;
        read->socket = socket;
        read->waiting.start();
        reads.insert( socket, read );

        QObject::connect( socket, &QTcpSocket::readyRead, q, [this, socket] {
            if( PendingRead *read = reads.value( socket ) )
                readRequest( read );
        } );

        QObject::connect( socket, &QTcpSocket::bytesWritten, q, [this, socket] {
            if( PendingRead *read = reads.value( socket ) )
                pump( read );
        } );

        QObject::connect( socket, &QTcpSocket::disconnected, q, [this, socket] {
            if( PendingRead *read = reads.value( socket ) )
                close( read );
        } );
    }
}

void StreamServer::Private::readRequest( PendingRead *read )
{
    if( read->responded ) {
        // Whatever a client says after its request, it is not our business.
        read->socket->readAll();
        return;
    }

    read->head += read->socket->readAll();

    if( read->head.size() > maxRequestBytes ) {
        fail( read, 431, "Request Header Fields Too Large" );
        return;
    }

    if( read->head.contains( "\r\n\r\n" ) )
        serve( read );
}

void StreamServer::Private::serve( PendingRead *read )
{
    const QByteArray headers = read->head.left( read->head.indexOf( "\r\n\r\n" ) );
    const QList<QByteArray> lines = headers.split( '\n' );

    const QList<QByteArray> requestLine = lines.value( 0 ).trimmed().split( ' ' );
    if( requestLine.size() < 2 ) {
        fail( read, 400, "Bad Request" );
        return;
    }

    const QByteArray method = requestLine.at( 0 );
    if( method != "GET" && method != "HEAD" ) {
        fail( read, 405, "Method Not Allowed", "Allow: GET, HEAD\r\n" );
        return;
    }
    read->headOnly = method == "HEAD";

    QHash<QByteArray, QByteArray> fields;
    for( int i = 1; i < lines.size(); ++i ) {
        const QByteArray line = lines.at( i ).trimmed();
        const int colon = line.indexOf( ':' );
        if( colon > 0 )
            fields.insert( line.left( colon ).toLower(), line.mid( colon + 1 ).trimmed() );
    }

    // Against dns rebinding: a page in a browser can be made to resolve its own
    // domain to 127.0.0.1, and would then be talking to this server with its
    // own name in the Host header. One comparison closes the whole class.
    const QByteArray expectedHost = "127.0.0.1:" + QByteArray::number( server->serverPort() );
    if( fields.value( "host" ) != expectedHost ) {
        fail( read, 404, "Not Found" );
        return;
    }

    const QByteArray target = requestLine.at( 1 );
    const int queryStart = target.indexOf( '?' );
    const QByteArray path  = queryStart < 0 ? target : target.left( queryStart );
    const QByteArray query = queryStart < 0 ? QByteArray() : target.mid( queryStart + 1 );

    // Without a token any process on this machine could read the cache through
    // the server. It is not a secret worth much, but it is worth what it costs.
    if( query != "t=" + token ) {
        fail( read, 404, "Not Found" );
        return;
    }

    const QList<QByteArray> parts = path.split( '/' );
    if( parts.size() != 3 ) {
        fail( read, 404, "Not Found" );
        return;
    }

    const QString infoHash = QString::fromLatin1( parts.at( 1 ) );

    bool ok = false;
    const int fileIndex = parts.at( 2 ).toInt( &ok );
    if( !ok || fileIndex < 0 ) {
        fail( read, 404, "Not Found" );
        return;
    }

    // A torrent that is not in the session cannot be read from. Waking it up is
    // the caller's business, not the server's: it is the lazy reactivation, and
    // it belongs where the track was asked for.
    read->track = trackFor( infoHash, fileIndex, TorrentTrack::Foreground, &read->trackKey );
    if( !read->track ) {
        fail( read, 404, "Not Found" );
        return;
    }

    const qint64 total = read->track->size();
    qint64 first = 0;
    qint64 last  = total - 1;
    bool   partial = false;

    const QByteArray range = fields.value( "range" );
    if( range.startsWith( "bytes=" ) ) {
        // Only the first range of a set is served. Nothing in the players we
        // care about asks for more than one, and answering half of a multipart
        // request would be worse than answering the whole file.
        const QByteArray spec = range.mid( 6 ).split( ',' ).value( 0 ).trimmed();
        const int dash = spec.indexOf( '-' );

        if( dash < 0 ) {
            fail( read, 416, "Range Not Satisfiable", "Content-Range: bytes */" + QByteArray::number( total ) + "\r\n" );
            return;
        }

        const QByteArray fromText = spec.left( dash );
        const QByteArray toText   = spec.mid( dash + 1 );

        if( fromText.isEmpty() ) {
            const qint64 suffix = toText.toLongLong();
            if( suffix <= 0 ) {
                fail( read, 416, "Range Not Satisfiable", "Content-Range: bytes */" + QByteArray::number( total ) + "\r\n" );
                return;
            }
            first = qMax( qint64( 0 ), total - suffix );
        } else {
            first = fromText.toLongLong();
            if( !toText.isEmpty() )
                last = qMin( toText.toLongLong(), total - 1 );
        }

        if( first < 0 || first > last || first >= total ) {
            fail( read, 416, "Range Not Satisfiable", "Content-Range: bytes */" + QByteArray::number( total ) + "\r\n" );
            return;
        }

        partial = true;
    }

    read->nextOffset = first;
    read->endOffset  = last;

    QByteArray response = partial ? "HTTP/1.1 206 Partial Content\r\n" : "HTTP/1.1 200 OK\r\n";
    response += "Content-Type: " + contentTypeFor( read->track->localPath() ) + "\r\n";
    response += "Accept-Ranges: bytes\r\n";
    response += "Content-Length: " + QByteArray::number( last - first + 1 ) + "\r\n";
    if( partial ) {
        response += "Content-Range: bytes " + QByteArray::number( first )
                    + "-" + QByteArray::number( last )
                    + "/" + QByteArray::number( total ) + "\r\n";
    }
    response += "Cache-Control: no-store\r\n";
    response += "Connection: close\r\n\r\n";

    read->socket->write( response );
    read->responded = true;

    if( read->headOnly ) {
        finish( read );
        return;
    }

    pump( read );
}

void StreamServer::Private::pump( PendingRead *read )
{
    if( !read->responded || read->headOnly || !read->track )
        return;

    while( read->nextOffset <= read->endOffset ) {
        // The socket is the brake. Filling it further would only move the whole
        // track from the disk into memory.
        if( read->socket->bytesToWrite() >= socketHighWater ) {
            read->waitingForPiece = false;
            return;
        }

        if( !read->track->isReadableAt( read->nextOffset ) ) {
            read->track->wantAt( read->nextOffset );

            if( !read->waitingForPiece ) {
                read->waitingForPiece = true;
                read->waiting.restart();
            }
            return;
        }

        // Only after have_piece said yes: the piece has been written and its
        // hash checked, so what is under it on disk is the real thing.
        if( !read->file.isOpen() ) {
            read->file.setFileName( read->track->localPath() );
            if( !read->file.open( QIODevice::ReadOnly ) ) {
                qDebug() << "GrooveOff ::" << "cannot open" << read->file.fileName();
                close( read );
                return;
            }
        }

        const qint64 stop = qMin( read->track->pieceEnd( read->nextOffset ), read->endOffset + 1 );
        const qint64 wanted = qMin( stop - read->nextOffset, chunkSize );
        if( wanted <= 0 ) {
            close( read );
            return;
        }

        if( !read->file.seek( read->nextOffset ) ) {
            close( read );
            return;
        }

        const QByteArray chunk = read->file.read( wanted );
        if( chunk.isEmpty() ) {
            close( read );
            return;
        }

        read->socket->write( chunk );
        read->nextOffset += chunk.size();
        read->waitingForPiece = false;
        read->waiting.restart();
    }

    finish( read );
}

void StreamServer::Private::finish( PendingRead *read )
{
    // disconnectFromHost flushes what is still queued before closing, so the
    // last chunk is not lost by saying goodbye too early.
    read->socket->disconnectFromHost();
}

void StreamServer::Private::fail( PendingRead *read, int code, const QByteArray &reason, const QByteArray &extraHeader )
{
    QByteArray response = "HTTP/1.1 " + QByteArray::number( code ) + " " + reason + "\r\n";
    response += extraHeader;
    response += "Content-Length: 0\r\n";
    response += "Connection: close\r\n\r\n";

    read->socket->write( response );
    read->responded = true;

    finish( read );
}

void StreamServer::Private::close( PendingRead *read )
{
    QTcpSocket *socket = read->socket;

    reads.remove( socket );

    if( !read->trackKey.isEmpty() )
        releaseTrack( read->trackKey );

    delete read;

    socket->disconnect();
    socket->abort();
    socket->deleteLater();
}

TorrentTrack* StreamServer::Private::trackFor( const QString &infoHash, int fileIndex,
                                               TorrentTrack::Mode mode, QString *key )
{
    if( !The::torrentSession()->hasMetadata( infoHash ) )
        return nullptr;

    const QString trackKey = infoHash + QLatin1Char( '/' ) + QString::number( fileIndex );

    auto entry = tracks.find( trackKey );

    // The track that was being taken in the background is now the one being
    // listened to: it has to be built again, or it would keep the modest
    // priority and the distant deadlines it was given as a spare.
    if( entry != tracks.end()
        && entry.value().track->mode() != mode
        && entry.value().readers == 0 ) {
        delete entry.value().track;
        tracks.erase( entry );
        entry = tracks.end();

        if( prefetchedKey == trackKey )
            prefetchedKey.clear();
    }

    if( entry == tracks.end() ) {
        TorrentTrack *track = new TorrentTrack( infoHash, fileIndex, mode, q );
        if( !track->isValid() ) {
            delete track;
            return nullptr;
        }

        TrackEntry created;
        created.track = track;
        created.idle.start();
        entry = tracks.insert( trackKey, created );
    }

    ++entry.value().readers;
    *key = trackKey;

    return entry.value().track;
}

void StreamServer::Private::releaseTrack( const QString &key )
{
    auto entry = tracks.find( key );
    if( entry == tracks.end() )
        return;

    if( entry.value().readers > 0 )
        --entry.value().readers;

    if( entry.value().readers == 0 )
        entry.value().idle.restart();
}

void StreamServer::Private::checkPreparation()
{
    if( preparedKey.isEmpty() )
        return;

    if( !q->isPlayable( preparedHash, preparedIndex ) )
        return;

    const QString infoHash  = preparedHash;
    const int     fileIndex = preparedIndex;
    const QString key       = preparedKey;

    preparedKey.clear();
    preparedHash.clear();
    preparedIndex = -1;

    // The reference is dropped here rather than at the request: the track has
    // ten seconds of grace, which is more than the player needs to ask for it.
    releaseTrack( key );

    emit q->trackPlayable( infoHash, fileIndex );
}

void StreamServer::Private::sweep()
{
    const QList<QTcpSocket*> sockets = reads.keys();
    for( QTcpSocket *socket : sockets ) {
        PendingRead *read = reads.value( socket );
        if( !read )
            continue;

        // A connection that never finished saying what it wants is not going
        // to. Left alone it would sit here for as long as the application runs.
        if( !read->responded && !read->head.contains( "\r\n\r\n" ) ) {
            if( read->waiting.elapsed() > readTimeoutMs )
                fail( read, 408, "Request Timeout" );
            continue;
        }

        if( !read->waitingForPiece || read->waiting.elapsed() < readTimeoutMs )
            continue;

        if( read->responded ) {
            // The status line is long gone, so there is nothing left to say:
            // closing is how the player finds out.
            close( read );
        } else {
            fail( read, 503, "Service Unavailable" );
        }
    }

    const QList<QString> keys = tracks.keys();
    for( const QString &key : keys ) {
        const TrackEntry entry = tracks.value( key );
        if( entry.readers == 0 && entry.idle.elapsed() > trackIdleMs ) {
            tracks.remove( key );
            delete entry.track;
        }
    }
}

StreamServer::StreamServer()
    : QObject( nullptr )
    , d( new Private( this ) )
{
}

StreamServer::~StreamServer()
{
    stop();
}

bool StreamServer::start()
{
    if( d->server )
        return true;

    QSettings settings;
    const quint16 wanted = static_cast<quint16>( settings.value( Settings::streamPort, Settings::streamPortDefault ).toUInt() );

    d->server = new QTcpServer( this );

    // The loopback and nothing else. Listening on Any would put the cache of
    // whoever runs this on their local network.
    if( !d->server->listen( QHostAddress::LocalHost, wanted ) ) {
        if( wanted == 0 ) {
            qDebug() << "GrooveOff ::" << "cannot listen on the loopback:" << d->server->errorString();
            delete d->server;
            d->server = nullptr;
            return false;
        }

        qDebug() << "GrooveOff ::" << "port" << wanted << "is taken, asking the system for one";
        if( !d->server->listen( QHostAddress::LocalHost, 0 ) ) {
            qDebug() << "GrooveOff ::" << "cannot listen on the loopback:" << d->server->errorString();
            delete d->server;
            d->server = nullptr;
            return false;
        }
    }

    // A fresh token at every start: a url that leaked from a previous run is
    // worth nothing in this one.
    QRandomGenerator *random = QRandomGenerator::system();
    d->token = ( QByteArray::number( random->generate64(), 16 )
                 + QByteArray::number( random->generate64(), 16 ) ).rightJustified( 32, '0' );

    connect( d->server, &QTcpServer::newConnection, this, [this] { d->accept(); } );

    connect( The::torrentSession(), &TorrentSession::pieceReady, this,
             [this]( const QString &infoHash, int ) {
        // Which read was waiting for which piece is not worth tracking: a
        // torrent has a handful of readers at most, and pumping one that has
        // nothing to send costs a comparison.
        const QList<QTcpSocket*> sockets = d->reads.keys();
        for( QTcpSocket *socket : sockets ) {
            PendingRead *read = d->reads.value( socket );
            if( read && read->track && read->track->infoHash() == infoHash )
                d->pump( read );
        }

        d->checkPreparation();
    } );

    if( !d->sweepTimer ) {
        d->sweepTimer = new QTimer( this );
        d->sweepTimer->setInterval( sweepIntervalMs );
        connect( d->sweepTimer, &QTimer::timeout, this, [this] { d->sweep(); } );
    }
    d->sweepTimer->start();

    return true;
}

void StreamServer::stop()
{
    if( !d->server )
        return;

    if( d->sweepTimer )
        d->sweepTimer->stop();

    const QList<QTcpSocket*> sockets = d->reads.keys();
    for( QTcpSocket *socket : sockets ) {
        if( PendingRead *read = d->reads.value( socket ) )
            d->close( read );
    }

    for( const Private::TrackEntry &entry : std::as_const( d->tracks ) )
        delete entry.track;
    d->tracks.clear();

    d->server->close();
    delete d->server;
    d->server = nullptr;
    d->token.clear();
}

bool StreamServer::isRunning() const
{
    return d->server && d->server->isListening();
}

quint16 StreamServer::port() const
{
    return d->server ? d->server->serverPort() : 0;
}

QUrl StreamServer::urlFor( const QString &infoHash, int fileIndex ) const
{
    if( !isRunning() || infoHash.isEmpty() )
        return QUrl();

    QUrl url;
    url.setScheme( QLatin1String( "http" ) );
    url.setHost( QLatin1String( "127.0.0.1" ) );
    url.setPort( d->server->serverPort() );
    url.setPath( QLatin1Char( '/' ) + infoHash + QLatin1Char( '/' ) + QString::number( fileIndex ) );
    url.setQuery( QLatin1String( "t=" ) + QString::fromLatin1( d->token ) );

    return url;
}

QUrl StreamServer::urlFor( const TrackInfoPtr &track ) const
{
    if( !track )
        return QUrl();

    return urlFor( track->infoHash(), track->fileIndex() );
}

bool StreamServer::isPlayable( const QString &infoHash, int fileIndex ) const
{
    TorrentSession *session = The::torrentSession();

    if( !session->hasMetadata( infoHash ) )
        return false;

    const qint64 size = session->fileSize( infoHash, fileIndex );
    if( size <= 0 )
        return false;

    // The head and nothing more. The tail is asked for at the same time, by
    // TorrentTrack, because ffmpeg probes it — but waiting for it here would
    // hold a forty megabyte flac hostage to a piece at the far end of it,
    // while a piece it has to wait for is a wait and not an error.
    const TorrentSession::PieceLocation head = session->locate( infoHash, fileIndex, 0 );

    return session->havePiece( infoHash, head.piece );
}

void StreamServer::prepare( const QString &infoHash, int fileIndex )
{
    if( !isRunning() )
        return;

    cancelPreparation();

    QString key;
    if( !d->trackFor( infoHash, fileIndex, TorrentTrack::Foreground, &key ) )
        return;

    d->preparedKey   = key;
    d->preparedHash  = infoHash;
    d->preparedIndex = fileIndex;

    // Whatever is already there may be enough: a track played before is
    // playable at once, and waiting for a piece that will never come again
    // would leave it silent for ever.
    d->checkPreparation();
}

void StreamServer::cancelPreparation()
{
    if( d->preparedKey.isEmpty() )
        return;

    d->releaseTrack( d->preparedKey );
    d->preparedKey.clear();
    d->preparedHash.clear();
    d->preparedIndex = -1;
}

void StreamServer::prefetch( const QString &infoHash, int fileIndex )
{
    if( !isRunning() )
        return;

    const QString wanted = infoHash + QLatin1Char( '/' ) + QString::number( fileIndex );
    if( d->prefetchedKey == wanted )
        return;

    cancelPrefetch();

    QString key;
    if( !d->trackFor( infoHash, fileIndex, TorrentTrack::Background, &key ) )
        return;

    d->prefetchedKey = key;
}

void StreamServer::cancelPrefetch()
{
    if( d->prefetchedKey.isEmpty() )
        return;

    d->releaseTrack( d->prefetchedKey );
    d->prefetchedKey.clear();
}
