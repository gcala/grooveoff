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
#include <QFileInfo>
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

/** how long a drop waits for libtorrent to let go of the torrent */
const int removalTimeoutMs = 5000;

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
     * Takes the torrent out of the session and forgets what we were tracking
     * about it, without touching a byte on the disk. Shared by the removal and
     * by the dropping of single files so that the bookkeeping cannot drift
     * apart between them; what happens to the files afterwards is the caller's.
     *
     * @return whether the session was holding it.
     */
    bool detach( const QString &infoHash, bool deleteFiles );

    /**
     * Waits until libtorrent has actually let go of a torrent detach() asked it
     * to drop. remove_torrent() only asks: the work happens on the session's
     * own thread, and adding the same torrent again before it is done gets back
     * the very torrent that was being removed.
     */
    void waitForRemoval( const QString &infoHash );

    /** torrents asked to leave and not gone yet, under every hash they answer to */
    QSet<QString> pendingRemovals;

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

    /** peers the last announce of each torrent came back with */
    QHash<QString, int> trackerPeers;

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
    //
    // With sharing on the two settings have to move together, or the gift is
    // not one: a peer unchoked and then fed a kilobyte a second gets nothing
    // worth having and chokes us back anyway.
    // slotCount and not slots: "slots" is one of Qt's own keywords, and a
    // variable by that name disappears into the preprocessor.
    const bool sharing = settings.value( Settings::sharing, Settings::sharingDefault ).toBool();
    const int slotCount = sharing
                          ? settings.value( Settings::uploadSlots, Settings::uploadSlotsDefault ).toInt()
                          : 0;

    pack.set_int( lt::settings_pack::unchoke_slots_limit, qMax( 0, slotCount ) );

    if( slotCount > 0 ) {
        const int limit = settings.value( Settings::uploadRateLimit, Settings::uploadRateLimitDefault ).toInt();
        pack.set_int( lt::settings_pack::upload_rate_limit, qMax( 0, limit ) * 1024 );
    } else {
        pack.set_int( lt::settings_pack::upload_rate_limit, 1024 );
    }

    const int downloadLimit = settings.value( Settings::downloadRateLimit, Settings::downloadRateLimitDefault ).toInt();
    pack.set_int( lt::settings_pack::download_rate_limit, downloadLimit * 1024 );

    // tracker is in here for one reason: a tracker that answers "nobody" and a
    // swarm that is genuinely empty look the same from the outside, and they
    // are not the same problem. tracker_reply_alert carries the number of
    // peers the announce came back with, which is exactly the missing fact.
    const lt::alert_category_t categories = lt::alert_category::status
                                            | lt::alert_category::error
                                            | lt::alert_category::storage
                                            | lt::alert_category::tracker
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
    // How many peers the announce came back with. Without this number a
    // tracker that answers "nobody" and a tracker that does not answer at all
    // look the same, and so does a swarm that is genuinely empty — three
    // different situations with three different things to do about them.
    if( const auto *replied = lt::alert_cast<lt::tracker_reply_alert>( alert ) ) {
        const QString infoHash = infoHashFor( replied->handle );

        trackerPeers.insert( infoHash, replied->num_peers );
        emit q->trackerReplied( infoHash, replied->num_peers );
        return;
    }

    if( const auto *added = lt::alert_cast<lt::add_torrent_alert>( alert ) ) {
        const QString infoHash = keyFor( added->params.info_hashes );
        pendingAdds.remove( infoHash );

        if( added->error ) {
            emit q->torrentError( infoHash, QString::fromStdString( added->error.message() ) );
            return;
        }

        handles.insert( infoHash, added->handle );
        awaitingMetadata.insert( infoHash, QDateTime::currentMSecsSinceEpoch() );

        // A .torrent, or a magnet reopened from its resume file, arrives with
        // the metadata already in hand and no metadata_received_alert follows.
        publishMetadata( infoHash, added->handle );

        // And only now is the torrent announced, which is the whole of an
        // invariant worth stating: nobody outside this class hears about a
        // torrent before its files are at dont_download. The other way round it
        // worked by luck. DownloadQueue answers torrentAdded by handing out the
        // priorities of the tracks somebody chose, and publishMetadata wiped
        // them a line later — the fetching after a restart came back only
        // because metadataReady is emitted at the end of that function and the
        // queue listens to that too. Anybody moving either emission, or trusting
        // one signal alone, would have left a restart fetching nothing at all,
        // in silence and with nothing in the log.
        emit q->torrentAdded( infoHash );
        return;
    }

    if( const auto *removed = lt::alert_cast<lt::torrent_removed_alert>( alert ) ) {
        if( removed->info_hashes.has_v1() )
            pendingRemovals.remove( hexString( removed->info_hashes.v1 ) );
        if( removed->info_hashes.has_v2() )
            pendingRemovals.remove( hexString( removed->info_hashes.get( lt::protocol_version::V2 ) ) );

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

bool TorrentSession::Private::detach( const QString &infoHash, bool deleteFiles )
{
    const lt::torrent_handle handle = session ? handles.take( infoHash ) : lt::torrent_handle();
    const bool held = handle.is_valid();

    if( held ) {
        const lt::info_hash_t hashes = handle.info_hashes();
        const QString directory = QDir( torrentsPath ).absoluteFilePath( infoHash );

        // Under both names it could come back as, the way pendingDeletes does:
        // a hybrid torrent carries two hashes and the alert may name either.
        if( hashes.has_v1() ) {
            pendingRemovals.insert( hexString( hashes.v1 ) );
            if( deleteFiles )
                pendingDeletes.insert( hexString( hashes.v1 ), directory );
        }
        if( hashes.has_v2() ) {
            pendingRemovals.insert( hexString( hashes.get( lt::protocol_version::V2 ) ) );
            if( deleteFiles )
                pendingDeletes.insert( hexString( hashes.get( lt::protocol_version::V2 ) ), directory );
        }

        session->remove_torrent( handle, deleteFiles ? lt::session::delete_files : lt::remove_flags_t() );
    }

    pendingAdds.remove( infoHash );
    published.remove( infoHash );
    awaitingMetadata.remove( infoHash );
    uploaded.remove( infoHash );

    return held;
}

void TorrentSession::Private::waitForRemoval( const QString &infoHash )
{
    if( !session )
        return;

    QElapsedTimer clock;
    clock.start();

    while( pendingRemovals.contains( infoHash ) && clock.elapsed() < removalTimeoutMs ) {
        session->wait_for_alert( std::chrono::milliseconds( 20 ) );
        q->drainAlerts();
    }

    if( pendingRemovals.contains( infoHash ) )
        qWarning() << "GrooveOff ::" << infoHash << "did not leave the session in time";
}

void TorrentSession::remove( const QString &infoHash, bool deleteFiles )
{
    const QString directory = QDir( d->torrentsPath ).absoluteFilePath( infoHash );

    const bool held = d->detach( infoHash, deleteFiles );

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

qint64 TorrentSession::dropFiles( const QString &infoHash, const QList<int> &fileIndexes )
{
    if( infoHash.isEmpty() || fileIndexes.isEmpty() || d->torrentsPath.isEmpty() )
        return -1;

    // Before anything else, so libtorrent stops asking for them and nothing is
    // left queued for its disk thread against a file about to go. Almost all of
    // the race in which a block already in flight recreates the file is closed
    // right here.
    for( int fileIndex : fileIndexes )
        setFilePriority( infoHash, fileIndex, SkipFile );

    const QString resumeFile = d->resumeFileFor( infoHash );

    // A torrent whose bytes came from a check of its files rather than from a
    // download may not have written a resume blob yet, and without one there is
    // no map to correct: the drop would delete nothing and say nothing. Asking
    // for it and waiting is the bounded pump the shutdown already uses, and
    // this is the only other place in this class that waits on purpose — it is
    // a gesture somebody made, not a signal arriving.
    const auto pending = d->handles.constFind( infoHash );
    if( d->session && pending != d->handles.constEnd() && !QFile::exists( resumeFile ) ) {
        d->saveResumeData( pending.value() );

        QElapsedTimer clock;
        clock.start();
        while( !QFile::exists( resumeFile ) && clock.elapsed() < resumeTimeoutMs ) {
            d->session->wait_for_alert( std::chrono::milliseconds( 50 ) );
            drainAlerts();
        }
    }

    const bool held = d->detach( infoHash, false );

    // remove_torrent() only asks. The torrent leaves on libtorrent's own thread,
    // and until it has gone, adding it again hands back the very torrent being
    // corrected — with its piece map whole. What follows then looks like it
    // worked: the file is unlinked and the resume we write is overwritten by
    // the old map a moment later, so libtorrent goes on believing it has those
    // pieces, nothing is ever fetched again however often the file is ticked,
    // and the track plays as silence. This wait is the whole difference.
    if( held )
        d->waitForRemoval( infoHash );

    const QByteArray stored = readWholeFile( resumeFile );
    if( stored.isEmpty() )
        return -1;

    lt::error_code error;
    lt::add_torrent_params params =
        lt::read_resume_data( lt::span<const char>( stored.constData(), stored.size() ), error );

    if( error || !params.ti ) {
        qDebug() << "GrooveOff ::" << "cannot drop files of" << infoHash
                 << "without a usable resume:" << QString::fromStdString( error.message() );
        return -1;
    }

    const lt::file_storage &layout = params.ti->layout();
    const int fileCount = params.ti->num_files();
    const std::string savePath = QDir( d->torrentsPath ).absoluteFilePath( infoHash ).toStdString();

    QSet<int> dropped;
    for( int fileIndex : fileIndexes ) {
        if( fileIndex >= 0 && fileIndex < fileCount )
            dropped.insert( fileIndex );
    }

    if( dropped.isEmpty() )
        return -1;

    // Every piece the dropped files touch, the ones they share with a neighbour
    // included. Keeping a shared piece looks careful and is the opposite: a
    // piece straddling two files is stored across both of them, so unlinking
    // one takes away its half — and a piece left claimed is a piece nobody will
    // ever ask for again. The head of the file, which is where the tags and the
    // cover live, is exactly what sits in that first shared piece: the track
    // came back as tens of kilobytes of holes followed by music, with no cover
    // and no header, and no amount of re-ticking it ever repaired that.
    //
    // Clearing it costs the neighbour up to one piece at each end, fetched
    // again the next time anything wants it. That is a cost; the other way was
    // a lie.
    QSet<int> goneWith;

    for( int fileIndex : std::as_const( dropped ) ) {
        const lt::file_index_t index { fileIndex };
        const qint64 size = layout.file_size( index );
        if( size <= 0 )
            continue;

        const int first = int( layout.map_file( index, 0, 0 ).piece );
        const int last  = int( layout.map_file( index, size - 1, 0 ).piece );

        for( int piece = first; piece <= last; ++piece )
            goneWith.insert( piece );
    }

    for( int piece : std::as_const( goneWith ) ) {
        const lt::piece_index_t index { piece };

        if( piece < params.have_pieces.size() )
            params.have_pieces.clear_bit( index );
        if( piece < params.verified_pieces.size() )
            params.verified_pieces.clear_bit( index );

        params.unfinished_pieces.erase( index );
    }

    // And the files themselves have to be expected missing, or the next open
    // validates the resume against a file that is not there and rechecks the
    // whole torrent to find out what we already know.
    // dont_download and not default_priority for the ones the blob says nothing
    // about: everything in this program starts at dont_download and the queue
    // raises what was chosen at its next pass, so filling with anything else
    // would have a torrent come back fetching files nobody asked for — which is
    // the one thing the whole design is built to prevent.
    if( int( params.file_priorities.size() ) < fileCount )
        params.file_priorities.resize( fileCount, lt::dont_download );

    for( int fileIndex : std::as_const( dropped ) ) {
        params.file_priorities[fileIndex] = lt::dont_download;
        QFile::remove( QString::fromStdString( layout.file_path( lt::file_index_t { fileIndex },
                                                                savePath ) ) );
    }

    if( !writeWholeFile( resumeFile, lt::write_resume_data_buf( params ) ) )
        qWarning() << "GrooveOff ::" << "could not write back the resume of" << infoHash;

    // What libtorrent itself will report at its next status update, and not the
    // space that came back on the disk: bytes_done has never been a disk
    // measurement, and writing the real figure would look right for one second
    // and then jump when the status arrives.
    qint64 remaining = 0;
    for( int piece = 0; piece < params.have_pieces.size(); ++piece ) {
        const lt::piece_index_t index { piece };
        if( params.have_pieces[index] )
            remaining += params.ti->piece_size( index );
    }

    if( held )
        reopen( infoHash, d->magnets.value( infoHash ) );

    return remaining;
}

void TorrentSession::requestResumeData( const QString &infoHash )
{
    const auto handle = d->handles.constFind( infoHash );
    if( handle != d->handles.constEnd() )
        d->saveResumeData( handle.value() );
}

int TorrentSession::trackerPeers( const QString &infoHash ) const
{
    // Below zero is not a count: it is the answer to a different question, the
    // one about a torrent whose trackers have said nothing at all yet.
    return d->trackerPeers.value( infoHash, -1 );
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

/*!
  \brief cachedFile: what the resume file says, without adding the torrent

  A torrent that has already fetched everything somebody chose has nothing left
  to do on the network, and handing it to the session in order to play it
  announces it to a tracker and to the dht to accomplish precisely nothing —
  and then, with sharing off, refuses to give anything back.

  But the bytes cannot simply be read off the disk, because the disk does not
  know which of them are real: a sparse file is the full size of the file
  whether it is whole or all holes, so nothing about it tells the two apart.

  What does tell them apart is the resume blob, and that is the point of doing
  it this way. The piece map in it is the very one libtorrent would answer
  have_piece() from, had the torrent been added — so this is not a second truth
  about the cache kept alongside the first, it is the same truth read from the
  same file, off the network. Measured at a tenth of a millisecond an answer.

  The save path is ours and is not the one in the resume, which holds the
  absolute path the torrent had when it was written: a cache that has been
  moved or copied would otherwise be looked for where it used to be. That is
  not hypothetical — it is what the resume files of a copied cache say.
*/
TorrentSession::CachedFile TorrentSession::cachedFile( const QString &infoHash, int fileIndex ) const
{
    CachedFile file;

    if( fileIndex < 0 || d->torrentsPath.isEmpty() )
        return file;

    const QByteArray resume = readWholeFile( d->resumeFileFor( infoHash ) );
    if( resume.isEmpty() )
        return file;

    lt::error_code error;
    const lt::add_torrent_params params =
        lt::read_resume_data( lt::span<const char>( resume.constData(), resume.size() ), error );

    // A resume written before the metadata arrived carries no file layout at
    // all, so there is nothing there that could be whole.
    if( error || !params.ti || fileIndex >= params.ti->num_files() )
        return file;

    const lt::file_storage &layout = params.ti->layout();
    const lt::file_index_t index { fileIndex };

    file.size = layout.file_size( index );
    if( file.size <= 0 )
        return file;

    const std::string savePath = QDir( d->torrentsPath ).absoluteFilePath( infoHash ).toStdString();
    file.path = QString::fromStdString( layout.file_path( index, savePath ) );

    // The map says what arrived, the disk says whether it is still there. A
    // cache directory deleted by hand leaves the resume file behind, and
    // believing it would mean answering a request with a status line and then
    // nothing at all. Saying "not whole" instead sends the caller down the road
    // that repairs itself: the torrent is woken, libtorrent rechecks, and what
    // is missing is fetched again.
    const QFileInfo onDisk( file.path );
    if( !onDisk.exists() || onDisk.size() != file.size )
        return file;

    const lt::peer_request first = layout.map_file( index, 0, 0 );
    const lt::peer_request last  = layout.map_file( index, file.size - 1, 0 );

    // Every piece the file touches, ends included: the first and the last are
    // shared with the files either side, and a file whose neighbour's half of
    // a piece never arrived is not whole.
    file.complete = params.have_pieces.size() > 0;
    for( lt::piece_index_t piece = first.piece; piece <= last.piece && file.complete; ++piece )
        file.complete = int( piece ) < params.have_pieces.size() && params.have_pieces[piece];

    return file;
}

void TorrentSession::setFilePriority( const QString &infoHash, int fileIndex, FilePriority priority )
{
    const auto handle = d->handles.constFind( infoHash );
    if( handle == d->handles.constEnd() )
        return;

    lt::download_priority_t value = lt::dont_download;
    switch( priority ) {
        // Between "do not fetch this" and "fetch it when you can" there is now
        // a step, and the whole order of the download queue lives in it: within
        // one torrent libtorrent serves the higher priority first and, at equal
        // priority, in piece order — which is track order. Three levels are
        // therefore enough to say "this one, then the ones after it, then the
        // ones left behind" without a single piece being bookkept by hand.
        case QueuedFile: value = lt::low_priority;     break;
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

    if( priority >= lt::top_priority )
        return TopFile;

    return priority <= lt::low_priority ? QueuedFile : NormalFile;
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
