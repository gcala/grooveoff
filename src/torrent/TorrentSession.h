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


#ifndef TORRENTSESSION_H
#define TORRENTSESSION_H

#include "../grooveoff/TrackInfo.h"

#include <QObject>
#include <QStringList>

#include <memory>

class TorrentSession;

namespace The {
    TorrentSession* torrentSession();
}

/**
  * @class TorrentSession
  * @short The one place that talks to the BitTorrent network.
  *
  * Owns the libtorrent session, turns its alerts into Qt signals and keeps the
  * fast resume data on disk. No libtorrent type appears in this header on
  * purpose: the rest of the application sees info hashes as strings, file
  * indexes as ints and TrackInfoPtr, exactly as it sees no QSql type outside
  * src/db. That is what makes it possible to change engine later without
  * touching the interface.
  *
  * The session does not know about the catalogue and never writes to it. It
  * reports what happens on the network and hands back track lists; storing
  * them, and deciding which torrents deserve to be reopened, belongs to the
  * caller.
  */
class TorrentSession : public QObject
{
    Q_OBJECT

public:
    friend TorrentSession* The::torrentSession();

    ~TorrentSession() override;

    /**
     * Offline shuts DHT, LSD and port mapping off and listens on the loopback
     * only. It exists for the tests: a test run must never announce anything
     * to anybody.
     */
    enum Mode {
        Connected,
        Offline
    };

    /** the two settings that together mean "no upload", read back from the session */
    struct UploadPolicy
    {
        int unchokeSlots     = -1;
        int uploadRateLimit  = -1;
    };

    /** what a byte of a file falls into, in the piece grid of its torrent */
    struct PieceLocation
    {
        int    piece    = -1;
        /** offset in the file just past that piece, so a reader knows where to stop */
        qint64 pieceEnd = 0;
    };

    enum FilePriority {
        SkipFile,     /**< dont_download: the resting state of every file */
        NormalFile,   /**< what the next track gets while the current one plays */
        TopFile       /**< the track being listened to, which never gives way */
    };

    /**
     * Creates the session under @p cacheRoot, which holds torrents/, resume/
     * and session.dat. Everything below it can be deleted by hand without
     * losing anything that cannot be fetched again.
     */
    bool start( const QString &cacheRoot, Mode mode = Connected );

    /**
     * Saves the resume data of every torrent, waits for the alerts to come
     * back and only then destroys the session. Idempotent, and called by the
     * destructor if nobody did.
     */
    void shutdown();

    bool isRunning() const;

    /** the directory holding one sparse tree per info hash */
    QString torrentsPath() const;

    /** the flat directory holding the fast resume blobs */
    QString resumePath() const;

    /**
     * Adds a magnet and returns the info hash it will be known by, or an empty
     * string on a malformed uri. Nothing but the metadata is fetched: the
     * torrent is added in upload mode and every file is set to
     * "do not download" the moment the file list is known.
     */
    QString addMagnet( const QString &magnetUri, QString *errorMessage = nullptr );

    /** the same for a .torrent file, whose metadata are available at once */
    QString addTorrentFile( const QString &fileName, QString *errorMessage = nullptr );

    /**
     * Brings a torrent the catalogue already knows back into the session, from
     * its resume file when there is one and from @p magnetUri otherwise.
     *
     * The info hash is passed in rather than derived, and that is deliberate: a
     * hybrid torrent has two hashes, which one libtorrent calls the best one
     * changes as metadata arrive, and a torrent that came back under a
     * different name would be a second row in the catalogue for the same
     * content.
     */
    bool reopen( const QString &infoHash, const QString &magnetUri, QString *errorMessage = nullptr );

    bool isActive( const QString &infoHash ) const;
    QStringList activeTorrents() const;

    /** whether a resume file exists, hence whether reopening needs no network */
    bool hasResume( const QString &infoHash ) const;

    /**
     * Takes the torrent out of the session, optionally with its files and its
     * resume blob. Used both by the cache eviction and by the removal from the
     * catalogue, which are two different things everywhere else.
     */
    void remove( const QString &infoHash, bool deleteFiles );

    /** asks for the resume data of one torrent; the write happens on the alert */
    void requestResumeData( const QString &infoHash );

    // --- reading a file out of a torrent ------------------------------------

    /** whether the file list of this torrent is known, hence whether it can be read */
    bool hasMetadata( const QString &infoHash ) const;

    /** where the file lives on disk, whether or not any of its bytes are there yet */
    QString filePath( const QString &infoHash, int fileIndex ) const;

    qint64 fileSize( const QString &infoHash, int fileIndex ) const;

    PieceLocation locate( const QString &infoHash, int fileIndex, qint64 fileOffset ) const;

    /**
     * True once the piece has arrived and passed its hash check, which is the
     * only moment at which reading those bytes off the sparse file is honest.
     */
    bool havePiece( const QString &infoHash, int piece ) const;

    void setFilePriority( const QString &infoHash, int fileIndex, FilePriority priority );

    /**
     * Asks for a piece within @p milliseconds. This is not sequential download:
     * libtorrent orders the peers by expected delivery time and re-requests
     * what falls behind. It is the primitive streaming is built on.
     */
    void setPieceDeadline( const QString &infoHash, int piece, int milliseconds );

    /**
     * Takes one piece back out of the hurry. Preferred to clearing them all,
     * which would also drop the deadlines of a second file of the same torrent
     * — the prefetched track, once there is one.
     */
    void resetPieceDeadline( const QString &infoHash, int piece );

    /** for when a whole torrent stops being interesting */
    void clearPieceDeadlines( const QString &infoHash );

    /** bytes sent to peers since the session started; expected to stay at zero */
    qint64 totalUpload() const;

    UploadPolicy uploadPolicy() const;

Q_SIGNALS:
    void torrentAdded( const QString &infoHash );
    void torrentError( const QString &infoHash, const QString &message );

    /** the file list of a torrent, every file of it, audio or not */
    void metadataReady( const QString &infoHash, const QString &name, const QList<TrackInfoPtr> &tracks );

    void pieceReady( const QString &infoHash, int pieceIndex );
    void statsUpdated( const QString &infoHash, int peers, int seeds, qint64 downloadRate, qint64 bytesDone );

    void resumeDataSaved( const QString &infoHash );

private Q_SLOTS:
    /** runs in the gui thread and translates the alerts into the signals above */
    void drainAlerts();

    void postUpdates();

private:
    TorrentSession();
    Q_DISABLE_COPY( TorrentSession )

    class Private;
    std::unique_ptr<Private> d;
};

#endif // TORRENTSESSION_H
