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

/*
    A hand driven harness for the torrent session, not a ctest test: it needs a
    swarm and it needs somebody to look at the output. It exists because the
    session has to be provable before there is any interface to prove it with.

        torrentcli <magnet uri | file.torrent | info hash> [--cache DIR]
                   [--seconds N] [--offline]

    What it prints is what the plan asks to be checked: the file list of a
    torrent, and the fact that having the list costs no content at all.

    Given a bare info hash it reopens the torrent from its resume file, and
    with --offline it does so with dht and port mapping off and nothing but the
    loopback to listen on. A file list coming out of that pair of options came
    off the disk and from nowhere else, which is the test for save_info_dict
    that no amount of staring at the code can replace.
*/

#include "torrent/TorrentSession.h"

#include <QCommandLineParser>
#include <QCoreApplication>
#include <QDir>
#include <QDirIterator>
#include <QRegularExpression>
#include <QTextStream>
#include <QTimer>

#include <csignal>

namespace {

QTextStream out( stdout );

volatile std::sig_atomic_t interrupted = 0;

void onInterrupt( int )
{
    interrupted = 1;
}

qint64 bytesUnder( const QString &path )
{
    qint64 total = 0;

    QDirIterator files( path, QDir::Files, QDirIterator::Subdirectories );
    while( files.hasNext() ) {
        files.next();
        total += files.fileInfo().size();
    }

    return total;
}

} // namespace

int main( int argc, char **argv )
{
    QCoreApplication application( argc, argv );
    QCoreApplication::setOrganizationName( QLatin1String( "gcala" ) );
    QCoreApplication::setApplicationName( QLatin1String( "grooveoff" ) );

    QCommandLineParser parser;
    parser.setApplicationDescription( QLatin1String( "Adds a torrent and prints its file list." ) );
    parser.addHelpOption();
    parser.addPositionalArgument( QLatin1String( "source" ), QLatin1String( "A magnet uri or a .torrent file." ) );

    QCommandLineOption cacheOption( QLatin1String( "cache" ),
                                    QLatin1String( "Cache directory to work in." ),
                                    QLatin1String( "dir" ),
                                    QDir::temp().absoluteFilePath( QLatin1String( "grooveoff-torrentcli" ) ) );
    QCommandLineOption secondsOption( QLatin1String( "seconds" ),
                                      QLatin1String( "How long to stay online." ),
                                      QLatin1String( "n" ),
                                      QLatin1String( "60" ) );
    QCommandLineOption offlineOption( QLatin1String( "offline" ),
                                      QLatin1String( "No dht, no port mapping, loopback only." ) );
    parser.addOption( cacheOption );
    parser.addOption( secondsOption );
    parser.addOption( offlineOption );
    parser.process( application );

    if( parser.positionalArguments().isEmpty() ) {
        parser.showHelp( 1 );
        return 1;
    }

    const QString source    = parser.positionalArguments().first();
    const QString cacheRoot = parser.value( cacheOption );
    const int     seconds   = parser.value( secondsOption ).toInt();

    TorrentSession *session = The::torrentSession();

    if( !session->start( cacheRoot, parser.isSet( offlineOption ) ? TorrentSession::Offline
                                                                  : TorrentSession::Connected ) ) {
        out << "cannot start the session under " << cacheRoot << Qt::endl;
        return 1;
    }

    const TorrentSession::UploadPolicy policy = session->uploadPolicy();
    out << "cache      " << cacheRoot << Qt::endl;
    out << "no upload  unchoke slots " << policy.unchokeSlots
        << ", upload rate limit " << policy.uploadRateLimit << " B/s" << Qt::endl;

    QObject::connect( session, &TorrentSession::torrentAdded, []( const QString &infoHash ) {
        out << "added      " << infoHash << Qt::endl;
    } );

    QObject::connect( session, &TorrentSession::torrentError,
                      []( const QString &infoHash, const QString &message ) {
        out << "error      " << infoHash << ": " << message << Qt::endl;
    } );

    QObject::connect( session, &TorrentSession::metadataReady,
                      []( const QString &infoHash, const QString &name, const QList<TrackInfoPtr> &tracks ) {
        out << "metadata   " << name << " (" << infoHash << "), " << tracks.count() << " files" << Qt::endl;
        for( const TrackInfoPtr &track : tracks ) {
            out << QString::asprintf( "  %4d  %10lld  %s  ",
                                      track->fileIndex(),
                                      static_cast<long long>( track->fileSize() ),
                                      track->isAudio() ? "audio" : "     " )
                << track->filePath() << Qt::endl;
        }
    } );

    QObject::connect( session, &TorrentSession::statsUpdated,
                      []( const QString &, int peers, int seeds, qint64 rate, qint64 done ) {
        out << QString::asprintf( "\rpeers %3d  seeds %3d  %8lld B/s  %10lld B done",
                                  peers, seeds,
                                  static_cast<long long>( rate ),
                                  static_cast<long long>( done ) );
        out.flush();
    } );

    const bool looksLikeAnInfoHash = QRegularExpression( QLatin1String( "^[0-9a-fA-F]{40}$" ) )
                                     .match( source ).hasMatch();

    QString error;
    QString infoHash;

    if( looksLikeAnInfoHash ) {
        infoHash = source;
        if( !session->reopen( infoHash, QString(), &error ) )
            infoHash.clear();
    } else if( source.startsWith( QLatin1String( "magnet:" ) ) ) {
        infoHash = session->addMagnet( source, &error );
    } else {
        infoHash = session->addTorrentFile( source, &error );
    }

    if( infoHash.isEmpty() ) {
        out << "refused    " << error << Qt::endl;
        return 1;
    }

    out << ( looksLikeAnInfoHash ? "reopening  " : "adding     " ) << infoHash << Qt::endl;

    std::signal( SIGINT, onInterrupt );

    QTimer interruptTimer;
    QObject::connect( &interruptTimer, &QTimer::timeout, [] {
        if( interrupted )
            QCoreApplication::quit();
    } );
    interruptTimer.start( 200 );

    QTimer::singleShot( seconds * 1000, &application, &QCoreApplication::quit );

    const int result = application.exec();

    out << Qt::endl;
    session->shutdown();

    // The number that matters: adding a torrent buys the metadata, and the
    // metadata do not live in the cache directory.
    out << "content    " << bytesUnder( QDir( session->torrentsPath() ).absoluteFilePath( infoHash ) )
        << " bytes on disk" << Qt::endl;
    out << "uploaded   " << session->totalUpload() << " bytes" << Qt::endl;
    out << "resume     " << ( session->hasResume( infoHash ) ? "saved" : "missing" ) << Qt::endl;

    return result;
}
