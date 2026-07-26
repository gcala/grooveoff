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


#ifndef STREAMSERVER_H
#define STREAMSERVER_H

#include "../grooveoff/TrackInfo.h"

#include <QObject>
#include <QUrl>

#include <memory>

class StreamServer;

namespace The {
    StreamServer* streamServer();
}

/**
  * @class StreamServer
  * @short Serves a file of a torrent over http, pieces permitting.
  *
  * The player is handed a local url instead of a path, because a path can only
  * be read where the bytes already are and a url can be answered as they
  * arrive. Range is supported, so seeking inside a track is not a special
  * case: it is another request starting somewhere else.
  *
  * Nothing here ever blocks. A request that runs into a piece that has not
  * arrived stops, asks for it, and is woken by TorrentSession::pieceReady.
  * Waiting on the socket instead would freeze the interface exactly while it
  * has to show that it is buffering.
  *
  * The surface this opens is small and closed on purpose: the loopback only, a
  * random token in every url, and the Host header checked against the port we
  * are bound to, which is what keeps a web page from reading the cache through
  * dns rebinding. The url carries an info hash and a number, never a file
  * name, so there is no path to traverse.
  */
class StreamServer : public QObject
{
    Q_OBJECT

public:
    friend StreamServer* The::streamServer();

    ~StreamServer() override;

    /**
     * Listens on 127.0.0.1, on the port from the settings; 0 means the system
     * picks one, which is the sensible default. Falls back to a system port if
     * the configured one is taken.
     */
    bool start();

    void stop();

    bool isRunning() const;
    quint16 port() const;

    /** the url the player is given: http://127.0.0.1:port/<info hash>/<index>?t=<token> */
    QUrl urlFor( const QString &infoHash, int fileIndex ) const;
    QUrl urlFor( const TrackInfoPtr &track ) const;

    /**
     * Whether a player can be pointed at this track yet. ffmpeg reads the head
     * of a file and then probes its tail before it will say anything about it,
     * so those are the two pieces that have to be there. Handing it a url any
     * earlier gets an i/o error rather than a wait, and the track never starts.
     */
    bool isPlayable( const QString &infoHash, int fileIndex ) const;

    /**
     * Asks for the pieces that make a track playable and holds it while they
     * arrive; @c trackPlayable follows. Preparing another track drops the
     * previous preparation, which is what keeps ten skips from turning into
     * ten downloads.
     */
    void prepare( const QString &infoHash, int fileIndex );

    void cancelPreparation();

    /**
     * Takes the opening of the next track with whatever bandwidth is left
     * over, so that the gap between two tracks is not spent waiting. What it
     * mostly hides is not the transfer but the time to resolve a magnet and
     * find peers, which is why it is worth doing across torrents too.
     */
    void prefetch( const QString &infoHash, int fileIndex );

    /**
     * Drops it, priorities and deadlines included. Called at every change of
     * track: without it, ten skips leave ten files downloading at once and a
     * cache full of music nobody listened to.
     */
    void cancelPrefetch();

Q_SIGNALS:
    /** the opening buffer of a prepared track is in */
    void trackPlayable( const QString &infoHash, int fileIndex );

private:
    StreamServer();
    Q_DISABLE_COPY( StreamServer )

    class Private;
    std::unique_ptr<Private> d;
};

#endif // STREAMSERVER_H
