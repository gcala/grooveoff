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


#include "TorrentSession.h"

#include "../grooveoff/Settings.h"

#include <../config-version.h>

#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QHash>
#include <QSaveFile>
#include <QSet>
#include <QSettings>
#include <QTimer>
#include <QVersionNumber>

#include <libtorrent/alert_types.hpp>
#include <libtorrent/download_priority.hpp>
#include <libtorrent/fingerprint.hpp>
#include <libtorrent/load_torrent.hpp>
#include <libtorrent/magnet_uri.hpp>
#include <libtorrent/peer_request.hpp>
#include <libtorrent/read_resume_data.hpp>
#include <libtorrent/session.hpp>
#include <libtorrent/session_params.hpp>
#include <libtorrent/settings_pack.hpp>
#include <libtorrent/torrent_flags.hpp>
#include <libtorrent/torrent_info.hpp>
#include <libtorrent/torrent_status.hpp>
#include <libtorrent/write_resume_data.hpp>

#include <chrono>
#include <vector>

namespace lt = libtorrent;

namespace The {
    static TorrentSession* s_TorrentSession_instance = nullptr;

    TorrentSession* torrentSession()
    {
        if( !s_TorrentSession_instance )
            s_TorrentSession_instance = new TorrentSession();

        return s_TorrentSession_instance;
    }
}

namespace {

/** how long the shutdown waits for the resume data before giving up */
const int resumeTimeoutMs = 10000;

/** how often the peers and the byte counts are refreshed */
const int updateIntervalMs = 1000;

/** the shortest interval between two automatic resume writes of one torrent */
const qint64 resumeMinIntervalMs = 30000;

/**
 * How long a magnet is given to produce a file list before the user is told it
 * will not. The torrent is not removed: a swarm that is silent now may answer
 * later, and a row that vanishes on its own is a row the user has lost.
 */
const qint64 metadataTimeoutMs = 60000;

QString hexString( const lt::sha1_hash &hash )
{
    return QString::fromLatin1( QByteArray( hash.data(), int( hash.size() ) ).toHex() );
}

/**
 * The name a torrent goes by. get_best() prefers the v2 hash, so a hybrid
 * torrent is filed under its v2 hash and a plain one under its v1 hash.
 */
QString bestHash( const lt::info_hash_t &hashes )
{
    if( !hashes.has_v1() && !hashes.has_v2() )
        return QString();

    return hexString( hashes.get_best() );
}

QByteArray readWholeFile( const QString &fileName )
{
    QFile file( fileName );
    if( !file.open( QIODevice::ReadOnly ) )
        return QByteArray();

    return file.readAll();
}

bool writeWholeFile( const QString &fileName, const std::vector<char> &buffer )
{
    // QSaveFile because a half written resume blob is worse than none: it costs
    // a full recheck of the files it claims to describe.
    QSaveFile file( fileName );
    if( !file.open( QIODevice::WriteOnly ) )
        return false;

    file.write( buffer.data(), qint64( buffer.size() ) );
    return file.commit();
}

} // namespace

class TorrentSession::Private
{
public:
    explicit Private( TorrentSession *session )
        : q( session )
    {}

    /** the pack is rebuilt at every start, so the stored one never goes stale */
    lt::settings_pack defaultSettings( Mode mode ) const;
    void applyProxy( lt::settings_pack &pack ) const;

    void applyDefaults( lt::add_torrent_params &params, const QString &infoHash ) const;

    /**
     * Which of the names we already know this torrent by. A hybrid torrent
     * carries two hashes and only one of them is the key we filed it under, so
     * both are tried before giving up.
     */
    QString keyFor( const lt::info_hash_t &hashes ) const;

    QString infoHashFor( const lt::torrent_handle &handle ) const;

    QString resumeFileFor( const QString &infoHash ) const;

    /**
     * Where the files of a torrent being deleted live, by every hash it could
     * come back under. Emptied by torrent_deleted_alert, which is the only
     * moment at which the directory is certainly nobody's any more.
     */
    QHash<QString, QString> pendingDeletes;

    void handleAlert( const lt::alert *alert );

    /**
     * Turns the file list into TrackInfo and stops the torrent from fetching
     * anything. Called once per torrent, from whichever of the two alerts
     * brings the metadata in first.
     */
    void publishMetadata( const QString &infoHash, const lt::torrent_handle &handle );

    void saveResumeData( const lt::torrent_handle &handle );

    /** the periodic save, kept from turning into one write per second */
    void maybeSaveResumeData( const QString &infoHash, const lt::torrent_handle &handle );

    TorrentSession *q;

    std::unique_ptr<lt::session> session;

    QString cacheRoot;
    QString torrentsPath;
    QString resumePath;
    QString sessionFile;

    QHash<QString, lt::torrent_handle> handles;

    /** the magnet each torrent came from, so that a track can carry it around */
    QHash<QString, QString> magnets;

    /** names of the torrents added but not acknowledged yet */
    QSet<QString> pendingAdds;

    /** torrents whose file list has already been handed out */
    QSet<QString> published;

    QHash<QString, qint64> uploaded;

    /** when each torrent last wrote its resume blob, epoch milliseconds */
    QHash<QString, qint64> lastResumeSave;

    /** torrents still waiting for their file list, and since when */
    QHash<QString, qint64> awaitingMetadata;

    QTimer *updateTimer = nullptr;

    int pendingResumes = 0;
};

lt::settings_pack TorrentSession::Private::defaultSettings( Mode mode ) const
{
    QSettings settings;
    const bool connected = mode == Connected;

    lt::settings_pack pack;

    const QVersionNumber version = QVersionNumber::fromString( QLatin1String( GROOVEOFF_VERSION ) );
    pack.set_str( lt::settings_pack::user_agent,
                  std::string( "GrooveOff/" GROOVEOFF_VERSION ) );
    pack.set_str( lt::settings_pack::peer_fingerprint,
                  lt::generate_fingerprint( "GO",
                                            version.majorVersion(),
                                            version.minorVersion(),
                                            version.microVersion() ) );

    // No upload, and here is the trap: in libtorrent a rate limit of 0 means
    // unlimited, not zero. What actually stops the upload is never unchoking
    // anybody; the rate limit is only a safety net behind it.
    pack.set_int( lt::settings_pack::unchoke_slots_limit, 0 );
    pack.set_int( lt::settings_pack::upload_rate_limit, 1024 );

    const int downloadLimit = settings.value( Settings::downloadRateLimit, Settings::downloadRateLimitDefault ).toInt();
    pack.set_int( lt::settings_pack::download_rate_limit, downloadLimit * 1024 );

    const lt::alert_category_t categories = lt::alert_category::status
                                            | lt::alert_category::error
                                            | lt::alert_category::storage
                                            | lt::alert_category::piece_progress;
    pack.set_int( lt::settings_pack::alert_mask,
                  static_cast<int>( static_cast<std::uint32_t>( categories ) ) );
    pack.set_int( lt::settings_pack::alert_queue_size, 4000 );

    // A torrent that is being shut down should not keep the application waiting
    // on a tracker that is not answering.
    pack.set_int( lt::settings_pack::stop_tracker_timeout, 1 );

    // Without DHT a good half of the magnets never resolve at all, so it stays
    // on; what does not happen is giving anything back, see above.
    pack.set_bool( lt::settings_pack::enable_dht, connected );
    pack.set_bool( lt::settings_pack::enable_lsd, connected );
    pack.set_bool( lt::settings_pack::enable_upnp, connected );
    pack.set_bool( lt::settings_pack::enable_natpmp, connected );

    if( connected ) {
        const int port = settings.value( Settings::listenPort, Settings::listenPortDefault ).toInt();
        pack.set_str( lt::settings_pack::listen_interfaces,
                      QStringLiteral( "0.0.0.0:%1,[::]:%1" ).arg( port ).toStdString() );
    } else {
        pack.set_str( lt::settings_pack::listen_interfaces, "127.0.0.1:0" );
    }

    applyProxy( pack );

    return pack;
}

/*!
  \brief applyProxy: routes the traffic through a proxy, when there is one

  This is the only setting in the whole application that actually hides the
  address from the swarm, which is why the two lines at the bottom matter more
  than the three at the top: without proxy_peer_connections and
  proxy_tracker_connections libtorrent sends part of the traffic straight out,
  and a proxy that covers half the connections is worse than none — it is the
  same exposure with the confidence of being covered.
*/
void TorrentSession::Private::applyProxy( lt::settings_pack &pack ) const
{
    QSettings settings;
    const QString type = settings.value( Settings::proxyType, Settings::proxyTypeNone ).toString();
    const QString host = settings.value( Settings::proxyHost ).toString();

    if( type == Settings::proxyTypeNone || host.isEmpty() )
        return;

    pack.set_int( lt::settings_pack::proxy_type,
                  type == Settings::proxyTypeSocks5 ? lt::settings_pack::socks5
                                                    : lt::settings_pack::http );
    pack.set_str( lt::settings_pack::proxy_hostname, host.toStdString() );
    pack.set_int( lt::settings_pack::proxy_port,
                  settings.value( Settings::proxyPort, Settings::proxyPortDefault ).toInt() );

    pack.set_bool( lt::settings_pack::proxy_peer_connections, true );
    pack.set_bool( lt::settings_pack::proxy_tracker_connections, true );

    // A proxy that leaks the address through a dht lookup or a local
    // announcement is not a proxy. Losing the dht costs some magnets; getting
    // it wrong costs the thing the proxy was turned on for.
    pack.set_bool( lt::settings_pack::proxy_hostnames, true );
    pack.set_bool( lt::settings_pack::enable_lsd, false );
}

void TorrentSession::Private::applyDefaults( lt::add_torrent_params &params, const QString &infoHash ) const
{
    params.save_path = QDir( torrentsPath ).absoluteFilePath( infoHash ).toStdString();

    // upload_mode first, priorities later: between the metadata arriving and
    // the priorities being set libtorrent would otherwise have all the time it
    // needs to ask for a few pieces of something nobody chose.
    params.flags |= lt::torrent_flags::upload_mode;

    // The queueing logic exists to share bandwidth between torrents that are
    // downloading. Here a torrent is either the one being listened to or idle,
    // so letting libtorrent pause and resume them behind our back only makes
    // the state harder to read.
    //
    // paused goes with it, and forgetting that is a trap with no error message:
    // default_flags holds both, an auto managed torrent is started by the queue
    // manager, and one that is merely not auto managed any more stays paused for
    // ever. The symptom is a torrent that sits at zero peers and never resolves
    // its magnet, which looks like a network problem and is not.
    params.flags &= ~( lt::torrent_flags::auto_managed | lt::torrent_flags::paused );
}

QString TorrentSession::Private::keyFor( const lt::info_hash_t &hashes ) const
{
    QStringList candidates;
    if( hashes.has_v1() )
        candidates << hexString( hashes.v1 );
    if( hashes.has_v2() )
        candidates << hexString( hashes.get( lt::protocol_version::V2 ) );

    for( const QString &candidate : candidates ) {
        if( pendingAdds.contains( candidate ) || handles.contains( candidate ) )
            return candidate;
    }

    return bestHash( hashes );
}

QString TorrentSession::Private::infoHashFor( const lt::torrent_handle &handle ) const
{
    for( auto it = handles.constBegin(); it != handles.constEnd(); ++it ) {
        if( it.value() == handle )
            return it.key();
    }

    return QString();
}

QString TorrentSession::Private::resumeFileFor( const QString &infoHash ) const
{
    return QDir( resumePath ).absoluteFilePath( infoHash + QLatin1String( ".resume" ) );
}

void TorrentSession::Private::publishMetadata( const QString &infoHash, const lt::torrent_handle &handle )
{
    if( infoHash.isEmpty() || published.contains( infoHash ) )
        return;

    const std::shared_ptr<const lt::torrent_info> info = handle.torrent_file();
    if( !info )
        return;

    published.insert( infoHash );
    awaitingMetadata.remove( infoHash );

    const lt::file_storage &files = info->layout();

    // A torrent reopened from its resume file may have arrived without anybody
    // saying where it came from. The bare magnet is worth less than the one the
    // catalogue holds, having no trackers in it, but it is never nothing.
    const QString magnet = magnets.value( infoHash,
                                          QLatin1String( "magnet:?xt=urn:btih:" ) + infoHash );

    QList<TrackInfoPtr> tracks;
    std::vector<lt::download_priority_t> priorities( size_t( files.num_files() ), lt::dont_download );

    for( int index = 0; index < files.num_files(); ++index ) {
        const lt::file_index_t fileIndex { index };

        // Pad files are alignment, not content: showing them would be showing
        // the user something that is not in the torrent as far as they know.
        if( files.pad_file_at( fileIndex ) )
            continue;

        TrackInfoPtr track( new TrackInfo );
        track->setInfoHash( infoHash );
        track->setMagnetUri( magnet );
        track->setFileIndex( index );
        track->setFilePath( QString::fromStdString( files.file_path( fileIndex ) ) );
        track->setFileSize( files.file_size( fileIndex ) );
        track->setIsAudio( TrackInfo::isAudioFile( track->filePath() ) );

        tracks.append( track );
    }

    // Nothing is fetched until somebody asks for a track, so adding a torrent
    // costs the metadata and not a byte more. This is the "remote playlist" of
    // the whole idea, and it is what du on the cache directory has to confirm.
    handle.prioritize_files( priorities );
    handle.unset_flags( lt::torrent_flags::upload_mode );

    emit q->metadataReady( infoHash, QString::fromStdString( info->name() ), tracks );
}

void TorrentSession::Private::saveResumeData( const lt::torrent_handle &handle )
{
    if( !handle.is_valid() )
        return;

    // save_info_dict is the flag the whole restart story hangs on: with it the
    // resume file carries the metadata, so at the next start the file list comes
    // off the disk, with no network and no dht.
    handle.save_resume_data( lt::torrent_handle::save_info_dict
                             | lt::torrent_handle::flush_disk_cache );
    ++pendingResumes;
}

void TorrentSession::Private::maybeSaveResumeData( const QString &infoHash, const lt::torrent_handle &handle )
{
    const qint64 now  = QDateTime::currentMSecsSinceEpoch();
    const qint64 last = lastResumeSave.value( infoHash, 0 );

    // libtorrent asks for a save far more often than it is worth writing one,
    // so the floor is here: a crash costs half a minute of progress, and the
    // disk is not written to once a second while a track is streaming.
    if( last != 0 && now - last < resumeMinIntervalMs )
        return;

    lastResumeSave.insert( infoHash, now );
    saveResumeData( handle );
}

void TorrentSession::Private::handleAlert( const lt::alert *alert )
{
    if( const auto *added = lt::alert_cast<lt::add_torrent_alert>( alert ) ) {
        const QString infoHash = keyFor( added->params.info_hashes );
        pendingAdds.remove( infoHash );

        if( added->error ) {
            emit q->torrentError( infoHash, QString::fromStdString( added->error.message() ) );
            return;
        }

        handles.insert( infoHash, added->handle );
        awaitingMetadata.insert( infoHash, QDateTime::currentMSecsSinceEpoch() );
        emit q->torrentAdded( infoHash );

        // A .torrent, or a magnet reopened from its resume file, arrives with
        // the metadata already in hand and no metadata_received_alert follows.
        publishMetadata( infoHash, added->handle );
        return;
    }

    if( const auto *metadata = lt::alert_cast<lt::metadata_received_alert>( alert ) ) {
        publishMetadata( infoHashFor( metadata->handle ), metadata->handle );
        return;
    }

    if( const auto *failed = lt::alert_cast<lt::metadata_failed_alert>( alert ) ) {
        emit q->torrentError( infoHashFor( failed->handle ),
                              QString::fromStdString( failed->error.message() ) );
        return;
    }

    if( const auto *piece = lt::alert_cast<lt::piece_finished_alert>( alert ) ) {
        emit q->pieceReady( infoHashFor( piece->handle ), static_cast<int>( piece->piece_index ) );
        return;
    }

    if( const auto *update = lt::alert_cast<lt::state_update_alert>( alert ) ) {
        for( const lt::torrent_status &status : update->status ) {
            const QString infoHash = infoHashFor( status.handle );
            if( infoHash.isEmpty() )
                continue;

            uploaded.insert( infoHash, status.total_upload );
            emit q->statsUpdated( infoHash,
                                  status.num_peers,
                                  status.num_seeds,
                                  status.download_rate,
                                  status.total_done );

            // libtorrent itself says when its resume data went stale. In 2.1
            // it says it here, in the status, and no longer with an alert of
            // its own. Taking it up means a crash costs the last half minute
            // rather than everything.
            if( status.need_save_resume_data )
                maybeSaveResumeData( infoHash, status.handle );
        }
        return;
    }

    if( const auto *resume = lt::alert_cast<lt::save_resume_data_alert>( alert ) ) {
        const QString infoHash = infoHashFor( resume->handle );
        if( pendingResumes > 0 )
            --pendingResumes;

        if( infoHash.isEmpty() )
            return;

        if( writeWholeFile( resumeFileFor( infoHash ), lt::write_resume_data_buf( resume->params ) ) )
            emit q->resumeDataSaved( infoHash );
        else
            qDebug() << "GrooveOff ::" << "could not write the resume data of" << infoHash;

        return;
    }

    if( const auto *resumeFailed = lt::alert_cast<lt::save_resume_data_failed_alert>( alert ) ) {
        // Counted down here as well, or the shutdown would wait the whole
        // timeout every time a torrent has no metadata to save.
        if( pendingResumes > 0 )
            --pendingResumes;

        qDebug() << "GrooveOff ::" << "no resume data for"
                 << infoHashFor( resumeFailed->handle )
                 << QString::fromStdString( resumeFailed->error.message() );
        return;
    }

    if( const auto *torrentError = lt::alert_cast<lt::torrent_error_alert>( alert ) ) {
        emit q->torrentError( infoHashFor( torrentError->handle ),
                              QString::fromStdString( torrentError->error.message() ) );
        return;
    }

    if( const auto *fileError = lt::alert_cast<lt::file_error_alert>( alert ) ) {
        emit q->storageError( infoHashFor( fileError->handle ),
                              QString::fromStdString( fileError->error.message() ) );
        return;
    }

    if( const auto *deleted = lt::alert_cast<lt::torrent_deleted_alert>( alert ) ) {
        // libtorrent takes away the files and the directories it made inside
        // the save path, and leaves the save path itself: it did not create it.
        // Waiting for this alert rather than deleting the tree along with the
        // torrent is what keeps us off the disk thread while it is still on it
        // — and an empty directory per evicted torrent is cruft that nothing
        // else would ever come back for.
        //
        // The path is looked up rather than derived: by now the handle is gone
        // from the map, and a hybrid torrent may not answer with the same hash
        // the directory was named after.
        QString directory;
        if( deleted->info_hashes.has_v1() )
            directory = pendingDeletes.take( hexString( deleted->info_hashes.v1 ) );
        if( directory.isEmpty() && deleted->info_hashes.has_v2() )
            directory = pendingDeletes.take( hexString( deleted->info_hashes.get( lt::protocol_version::V2 ) ) );

        if( !directory.isEmpty() )
            QDir( directory ).removeRecursively();

        return;
    }
}

TorrentSession::TorrentSession()
    : QObject( nullptr )
    , d( new Private( this ) )
{
}

TorrentSession::~TorrentSession()
{
    shutdown();
}

bool TorrentSession::start( const QString &cacheRoot, Mode mode )
{
    if( d->session )
        return true;

    d->cacheRoot    = cacheRoot;
    d->torrentsPath = QDir( cacheRoot ).absoluteFilePath( QLatin1String( "torrents" ) );
    d->resumePath   = QDir( cacheRoot ).absoluteFilePath( QLatin1String( "resume" ) );
    d->sessionFile  = QDir( cacheRoot ).absoluteFilePath( QLatin1String( "session.dat" ) );

    if( !QDir().mkpath( d->torrentsPath ) || !QDir().mkpath( d->resumePath ) ) {
        qDebug() << "GrooveOff ::" << "cannot create the torrent cache under" << cacheRoot;
        return false;
    }

    lt::session_params params;

    // What is worth keeping across runs is the dht routing table: with it a new
    // magnet resolves in seconds instead of tens of them. The settings are not
    // restored, they are rebuilt below, so a stored pack can never outlive the
    // code that wrote it.
    const QByteArray stored = readWholeFile( d->sessionFile );
    if( !stored.isEmpty() ) {
        try {
            params = lt::read_session_params( lt::span<const char>( stored.constData(), stored.size() ),
                                              lt::session_handle::save_dht_state );
        } catch( const std::exception &error ) {
            qDebug() << "GrooveOff ::" << "ignoring an unreadable session.dat:" << error.what();
            params = lt::session_params();
        }
    }

    params.settings = d->defaultSettings( mode );

    // Braces, not parentheses: with parentheses this is a function declaration,
    // and the error it produces points anywhere but here.
    d->session = std::unique_ptr<lt::session>{ new lt::session{ std::move( params ) } };

    d->session->set_alert_notify( [this] {
        // Called from libtorrent's own thread. Waking the gui thread up is the
        // only thing that may happen here; everything else is done in
        // drainAlerts(), which runs where the signals are safe to emit.
        QMetaObject::invokeMethod( this, &TorrentSession::drainAlerts, Qt::QueuedConnection );
    } );

    if( !d->updateTimer ) {
        d->updateTimer = new QTimer( this );
        d->updateTimer->setInterval( updateIntervalMs );
        connect( d->updateTimer, &QTimer::timeout, this, &TorrentSession::postUpdates );
    }
    d->updateTimer->start();

    return true;
}

void TorrentSession::shutdown()
{
    if( !d->session )
        return;

    if( d->updateTimer )
        d->updateTimer->stop();

    d->session->pause();

    // The counter is not reset: a periodic save may still be in flight, and
    // its alert has to be waited for like the others.
    for( auto it = d->handles.constBegin(); it != d->handles.constEnd(); ++it )
        d->saveResumeData( it.value() );

    // Skipping this wait is the classic way of losing resume data: the session
    // destructor does not wait for the requests in flight, they disappear in
    // silence, and the symptom shows up much later as "sometimes the cache
    // looks empty after a restart".
    QElapsedTimer clock;
    clock.start();
    while( d->pendingResumes > 0 && clock.elapsed() < resumeTimeoutMs ) {
        d->session->wait_for_alert( std::chrono::milliseconds( 200 ) );
        drainAlerts();
    }

    if( d->pendingResumes > 0 )
        qDebug() << "GrooveOff ::" << d->pendingResumes << "torrents did not save their resume data in time";

    if( !writeWholeFile( d->sessionFile,
                         lt::write_session_params_buf( d->session->session_state( lt::session_handle::save_dht_state ),
                                                       lt::session_handle::save_dht_state ) ) ) {
        qDebug() << "GrooveOff ::" << "could not write" << d->sessionFile;
    }

    d->session.reset();
    d->handles.clear();
    d->pendingAdds.clear();
    d->published.clear();
    d->awaitingMetadata.clear();
    d->magnets.clear();
    d->uploaded.clear();
}

bool TorrentSession::isRunning() const
{
    return d->session != nullptr;
}

QString TorrentSession::torrentsPath() const
{
    return d->torrentsPath;
}

QString TorrentSession::resumePath() const
{
    return d->resumePath;
}

QString TorrentSession::addMagnet( const QString &magnetUri, QString *errorMessage )
{
    if( !d->session ) {
        if( errorMessage )
            *errorMessage = tr( "The torrent session is not running." );
        return QString();
    }

    lt::error_code error;
    lt::add_torrent_params params = lt::parse_magnet_uri( magnetUri.toStdString(), error );
    if( error ) {
        if( errorMessage )
            *errorMessage = QString::fromStdString( error.message() );
        return QString();
    }

    const QString infoHash = bestHash( params.info_hashes );
    if( infoHash.isEmpty() ) {
        if( errorMessage )
            *errorMessage = tr( "The magnet link carries no info hash." );
        return QString();
    }

    d->magnets.insert( infoHash, magnetUri );

    if( d->handles.contains( infoHash ) || d->pendingAdds.contains( infoHash ) )
        return infoHash;

    d->applyDefaults( params, infoHash );
    d->pendingAdds.insert( infoHash );
    d->session->async_add_torrent( std::move( params ) );

    return infoHash;
}

QString TorrentSession::addTorrentFile( const QString &fileName, QString *errorMessage )
{
    if( !d->session ) {
        if( errorMessage )
            *errorMessage = tr( "The torrent session is not running." );
        return QString();
    }

    lt::error_code error;
    lt::add_torrent_params params = lt::load_torrent_file( QDir::toNativeSeparators( fileName ).toStdString(),
                                                          error,
                                                          lt::load_torrent_limits() );
    if( error || !params.ti ) {
        if( errorMessage )
            *errorMessage = error ? QString::fromStdString( error.message() )
                                  : tr( "The torrent file holds no metadata." );
        return QString();
    }

    const QString infoHash = bestHash( params.ti->info_hashes() );
    if( infoHash.isEmpty() ) {
        if( errorMessage )
            *errorMessage = tr( "The torrent file carries no info hash." );
        return QString();
    }

    // A track has to be able to reopen its torrent even when neither this file
    // nor the resume blob is around any more, so a magnet is derived now and
    // travels with it.
    d->magnets.insert( infoHash, QString::fromStdString( lt::make_magnet_uri( params ) ) );

    if( d->handles.contains( infoHash ) || d->pendingAdds.contains( infoHash ) )
        return infoHash;

    d->applyDefaults( params, infoHash );
    d->pendingAdds.insert( infoHash );
    d->session->async_add_torrent( std::move( params ) );

    return infoHash;
}

bool TorrentSession::reopen( const QString &infoHash, const QString &magnetUri, QString *errorMessage )
{
    if( !d->session ) {
        if( errorMessage )
            *errorMessage = tr( "The torrent session is not running." );
        return false;
    }

    if( d->handles.contains( infoHash ) || d->pendingAdds.contains( infoHash ) )
        return true;

    lt::add_torrent_params params;
    bool fromResume = false;

    const QByteArray resume = readWholeFile( d->resumeFileFor( infoHash ) );
    if( !resume.isEmpty() ) {
        lt::error_code error;
        params = lt::read_resume_data( lt::span<const char>( resume.constData(), resume.size() ), error );
        fromResume = !error;

        if( error ) {
            // Not an error the user has to hear about: the magnet below does
            // the same job, it only costs a round of dht.
            qDebug() << "GrooveOff ::" << "unusable resume data for" << infoHash
                     << QString::fromStdString( error.message() );
        }
    }

    if( !fromResume ) {
        if( magnetUri.isEmpty() ) {
            if( errorMessage )
                *errorMessage = tr( "Neither resume data nor a magnet link for this torrent." );
            return false;
        }

        lt::error_code error;
        params = lt::parse_magnet_uri( magnetUri.toStdString(), error );
        if( error ) {
            if( errorMessage )
                *errorMessage = QString::fromStdString( error.message() );
            return false;
        }
    }

    if( !magnetUri.isEmpty() )
        d->magnets.insert( infoHash, magnetUri );

    d->applyDefaults( params, infoHash );
    d->pendingAdds.insert( infoHash );
    d->session->async_add_torrent( std::move( params ) );

    return true;
}

bool TorrentSession::isActive( const QString &infoHash ) const
{
    return d->handles.contains( infoHash );
}

QStringList TorrentSession::activeTorrents() const
{
    return d->handles.keys();
}

QString TorrentSession::magnetUri( const QString &infoHash ) const
{
    return d->magnets.value( infoHash );
}

bool TorrentSession::hasResume( const QString &infoHash ) const
{
    return QFile::exists( d->resumeFileFor( infoHash ) );
}

void TorrentSession::remove( const QString &infoHash, bool deleteFiles )
{
    const QString directory = QDir( d->torrentsPath ).absoluteFilePath( infoHash );

    const lt::torrent_handle handle = d->session ? d->handles.take( infoHash ) : lt::torrent_handle();
    const bool held = handle.is_valid();

    if( held ) {
        if( deleteFiles ) {
            const lt::info_hash_t hashes = handle.info_hashes();

            if( hashes.has_v1() )
                d->pendingDeletes.insert( hexString( hashes.v1 ), directory );
            if( hashes.has_v2() )
                d->pendingDeletes.insert( hexString( hashes.get( lt::protocol_version::V2 ) ), directory );
        }

        d->session->remove_torrent( handle, deleteFiles ? lt::session::delete_files : lt::remove_flags_t() );
    }

    d->pendingAdds.remove( infoHash );
    d->published.remove( infoHash );
    d->awaitingMetadata.remove( infoHash );
    d->uploaded.remove( infoHash );

    if( !deleteFiles )
        return;

    // A torrent the session holds has its files unlinked by libtorrent on its
    // own disk thread, and the empty save path follows on torrent_deleted_alert.
    // One that was never reopened is a directory nobody else would ever come
    // back for, so it goes now.
    //
    // This lives here, and not in whoever asked, because the two callers are
    // deliberately kept apart: evicting from the cache and removing from the
    // catalogue are different acts with different consequences. What they do
    // share is the layout of the disk, and that belongs to the session.
    if( !held && QDir( directory ).exists() )
        QDir( directory ).removeRecursively();

    // A resume file whose data are gone is worse than no resume file at all:
    // libtorrent would recheck files that do not exist any more.
    QFile::remove( d->resumeFileFor( infoHash ) );
}

void TorrentSession::requestResumeData( const QString &infoHash )
{
    const auto handle = d->handles.constFind( infoHash );
    if( handle != d->handles.constEnd() )
        d->saveResumeData( handle.value() );
}

bool TorrentSession::hasMetadata( const QString &infoHash ) const
{
    const auto handle = d->handles.constFind( infoHash );
    return handle != d->handles.constEnd() && handle.value().is_valid() && handle.value().torrent_file();
}

QString TorrentSession::filePath( const QString &infoHash, int fileIndex ) const
{
    const auto handle = d->handles.constFind( infoHash );
    if( handle == d->handles.constEnd() )
        return QString();

    const std::shared_ptr<const lt::torrent_info> info = handle.value().torrent_file();
    if( !info || fileIndex < 0 || fileIndex >= info->num_files() )
        return QString();

    const std::string savePath = QDir( d->torrentsPath ).absoluteFilePath( infoHash ).toStdString();
    return QString::fromStdString( info->layout().file_path( lt::file_index_t { fileIndex }, savePath ) );
}

qint64 TorrentSession::fileSize( const QString &infoHash, int fileIndex ) const
{
    const auto handle = d->handles.constFind( infoHash );
    if( handle == d->handles.constEnd() )
        return 0;

    const std::shared_ptr<const lt::torrent_info> info = handle.value().torrent_file();
    if( !info || fileIndex < 0 || fileIndex >= info->num_files() )
        return 0;

    return info->layout().file_size( lt::file_index_t { fileIndex } );
}

TorrentSession::PieceLocation TorrentSession::locate( const QString &infoHash,
                                                      int fileIndex,
                                                      qint64 fileOffset ) const
{
    PieceLocation location;

    const auto handle = d->handles.constFind( infoHash );
    if( handle == d->handles.constEnd() )
        return location;

    const std::shared_ptr<const lt::torrent_info> info = handle.value().torrent_file();
    if( !info || fileIndex < 0 || fileIndex >= info->num_files() )
        return location;

    const lt::file_storage &files = info->layout();
    const lt::file_index_t index { fileIndex };
    const qint64 size = files.file_size( index );

    if( fileOffset < 0 || fileOffset >= size )
        return location;

    // map_file answers with the piece, the offset inside it and the length
    // left. What a reader needs on top of the piece number is where that piece
    // ends in file coordinates, so that it can read up to there and stop.
    const lt::peer_request request = files.map_file( index, fileOffset, 0 );

    location.piece    = static_cast<int>( request.piece );
    location.pieceEnd = qMin( fileOffset + ( info->piece_length() - request.start ), size );

    return location;
}

bool TorrentSession::havePiece( const QString &infoHash, int piece ) const
{
    const auto handle = d->handles.constFind( infoHash );
    if( handle == d->handles.constEnd() || piece < 0 )
        return false;

    return handle.value().have_piece( lt::piece_index_t { piece } );
}

qint64 TorrentSession::bufferedBytes( const QString &infoHash, int fileIndex ) const
{
    const qint64 size = fileSize( infoHash, fileIndex );
    if( size <= 0 )
        return 0;

    qint64 offset = 0;
    while( offset < size ) {
        const PieceLocation location = locate( infoHash, fileIndex, offset );
        if( location.piece < 0 || !havePiece( infoHash, location.piece ) )
            break;

        offset = location.pieceEnd;
    }

    return offset;
}

void TorrentSession::setFilePriority( const QString &infoHash, int fileIndex, FilePriority priority )
{
    const auto handle = d->handles.constFind( infoHash );
    if( handle == d->handles.constEnd() )
        return;

    lt::download_priority_t value = lt::dont_download;
    switch( priority ) {
        case NormalFile: value = lt::default_priority; break;
        case TopFile:    value = lt::top_priority;     break;
        case SkipFile:   break;
    }

    handle.value().file_priority( lt::file_index_t { fileIndex }, value );
}

TorrentSession::FilePriority TorrentSession::filePriority( const QString &infoHash, int fileIndex ) const
{
    const auto handle = d->handles.constFind( infoHash );
    if( handle == d->handles.constEnd() )
        return SkipFile;

    const lt::download_priority_t priority = handle.value().file_priority( lt::file_index_t { fileIndex } );

    if( priority == lt::dont_download )
        return SkipFile;

    return priority >= lt::top_priority ? TopFile : NormalFile;
}

void TorrentSession::setPieceDeadline( const QString &infoHash, int piece, int milliseconds )
{
    const auto handle = d->handles.constFind( infoHash );
    if( handle == d->handles.constEnd() || piece < 0 )
        return;

    handle.value().set_piece_deadline( lt::piece_index_t { piece }, milliseconds );
}

void TorrentSession::resetPieceDeadline( const QString &infoHash, int piece )
{
    const auto handle = d->handles.constFind( infoHash );
    if( handle == d->handles.constEnd() || piece < 0 )
        return;

    handle.value().reset_piece_deadline( lt::piece_index_t { piece } );
}

void TorrentSession::clearPieceDeadlines( const QString &infoHash )
{
    const auto handle = d->handles.constFind( infoHash );
    if( handle != d->handles.constEnd() )
        handle.value().clear_piece_deadlines();
}

qint64 TorrentSession::totalUpload() const
{
    qint64 total = 0;
    for( qint64 bytes : d->uploaded )
        total += bytes;

    return total;
}

TorrentSession::UploadPolicy TorrentSession::uploadPolicy() const
{
    UploadPolicy policy;
    if( !d->session )
        return policy;

    const lt::settings_pack settings = d->session->get_settings();
    policy.unchokeSlots    = settings.get_int( lt::settings_pack::unchoke_slots_limit );
    policy.uploadRateLimit = settings.get_int( lt::settings_pack::upload_rate_limit );

    return policy;
}

void TorrentSession::drainAlerts()
{
    if( !d->session )
        return;

    std::vector<lt::alert*> alerts;
    d->session->pop_alerts( &alerts );

    for( const lt::alert *alert : alerts )
        d->handleAlert( alert );
}

void TorrentSession::postUpdates()
{
    if( !d->session || d->handles.isEmpty() )
        return;

    d->session->post_torrent_updates();

    // A magnet that never resolves has to say so at some point, or the row
    // waits for ever with nothing to show for it. The torrent stays where it
    // is: this is a report, not a removal.
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    const QList<QString> waiting = d->awaitingMetadata.keys();

    for( const QString &infoHash : waiting ) {
        if( now - d->awaitingMetadata.value( infoHash ) < metadataTimeoutMs )
            continue;

        d->awaitingMetadata.remove( infoHash );
        emit torrentError( infoHash, tr( "Could not get the track list of this torrent." ) );
    }
}
