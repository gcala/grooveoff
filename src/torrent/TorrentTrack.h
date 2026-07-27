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


#ifndef TORRENTTRACK_H
#define TORRENTTRACK_H

#include <QObject>
#include <QString>

/**
  * @class TorrentTrack
  * @short One file of a torrent, while somebody is reading it.
  *
  * Holds the streaming policy of a single file: it raises its priority, asks
  * for the pieces a reader is about to need, and puts everything back the way
  * it was when it goes away. Which bytes may be read is answered here too, so
  * that the server never reads a piece that has not arrived.
  *
  * A track exists only while it is wanted. Creating one costs a change of
  * priorities, destroying one gives the bandwidth back, and that pair is what
  * keeps ten skipped tracks from all downloading at once.
  */
class TorrentTrack : public QObject
{
    Q_OBJECT

public:
    /**
     * A foreground track is the one being listened to and never gives way. A
     * background one is the next in the list, taken with whatever bandwidth is
     * left over: normal priority instead of top, and deadlines far enough away
     * that libtorrent treats them as "when you can". Getting these two the
     * wrong way round gives a track that stutters while the network is fine,
     * which is a symptom that sends you looking anywhere but here.
     */
    enum Mode {
        Foreground,
        Background
    };

    TorrentTrack( const QString &infoHash, int fileIndex,
                  Mode mode = Foreground, QObject *parent = nullptr );
    ~TorrentTrack() override;

    Mode mode() const;

    /**
     * How many bytes of a file are worth @p seconds of listening. A guess, and
     * a deliberately generous one: it assumes more bitrate than nearly any
     * music has, so the answer errs towards buffering too much rather than too
     * little. Shared with the gate in front of the player so that the amount
     * waited for and the amount asked for are the same number.
     */
    static qint64 bytesForSeconds( int seconds );

    QString infoHash() const;
    int fileIndex() const;

    /** the size the file has in the torrent, whatever is on disk so far */
    qint64 size() const;

    /** where the sparse file lives */
    QString localPath() const;

    bool isValid() const;

    /** whether the piece holding @p offset has arrived and can be read */
    bool isReadableAt( qint64 offset ) const;

    /** the offset just past the piece holding @p offset: where a reader stops */
    qint64 pieceEnd( qint64 offset ) const;

    int pieceAt( qint64 offset ) const;

    /**
     * Says that a reader is waiting at @p offset. Brings the pieces from there
     * forward with deadlines that grow with the distance, so the piece needed
     * next is asked for first.
     *
     * Called again for an offset already inside the window it does nothing:
     * re-arming the same deadlines at every stall would make the request
     * pattern worse, not better.
     */
    void wantAt( qint64 offset );

private:
    Q_DISABLE_COPY( TorrentTrack )

    /** puts deadlines on the pieces from @p offset forward, and drops the old ones */
    void armWindow( qint64 offset );

    QString m_infoHash;
    int     m_fileIndex;
    Mode    m_mode;
    qint64  m_size;

    /** the pieces the current deadlines cover, both ends included */
    int m_windowFirst;
    int m_windowLast;
};

#endif // TORRENTTRACK_H
