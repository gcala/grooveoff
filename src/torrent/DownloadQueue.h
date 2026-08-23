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

#ifndef DOWNLOADQUEUE_H
#define DOWNLOADQUEUE_H

#include "TorrentSession.h"

#include <QHash>
#include <QList>
#include <QObject>
#include <QString>

class DownloadQueue;
class QTimer;

namespace The {
    DownloadQueue* downloadQueue();
}

/**
 * @class DownloadQueue
 * @short What is fetched next, and in what order.
 *
 * The application used to fetch a track when somebody asked to hear it and not
 * a byte before. That is the right thing to do on a full disk and the wrong
 * thing on an empty one: if the cache has room, waiting buys nothing and costs
 * the wait at every track. So the tracks a torrent's playlist holds are fetched
 * as soon as they are chosen, in order, one after another.
 *
 * The order is a cursor rather than a list. It starts at the first track and
 * walks forward; when somebody plays the sixth, the cursor moves there and
 * goes on with the seventh, the eighth, the ninth — and only when there is
 * nothing left ahead does it come back for the third, fourth and fifth. The
 * reason is that people listen forwards: what follows the track being played
 * is what will be wanted next, and a queue that insisted on filling its holes
 * first would be fetching the past.
 *
 * None of that is bookkept piece by piece. Within a torrent libtorrent serves
 * the higher file priority first and, at equal priority, in piece order, which
 * is track order — so the whole rule is three priorities: the track being
 * played, the ones ahead of the cursor, the ones left behind. Across torrents
 * file priorities say nothing at all, and nothing is said here either: what
 * keeps the track being listened to ahead of every queue in the program is the
 * deadlines TorrentTrack puts on the pieces in front of the reader. The queue
 * sets none, which is exactly how it is told to use the line left over.
 *
 * The cache is the brake, and the rule about it is the same one the prefetch
 * has always had: bytes nobody has asked for **never cause an eviction**. When
 * the usage comes near the ceiling the queue stops, and what it did not reach
 * stays exactly as playable as it was before — fetched on demand, the old way.
 */
class DownloadQueue : public QObject
{
    Q_OBJECT
    friend DownloadQueue* The::downloadQueue();

public:
    ~DownloadQueue() override;

    /**
     * The tracks of one torrent's playlist, in the order they are shown in.
     * File indexes and not track ids: this side of the application has never
     * heard of the catalogue.
     */
    void setSelection( const QString &infoHash, const QList<int> &fileIndexes );

    /** the torrent is gone from the catalogue, and so is its queue */
    void forget( const QString &infoHash );

    /**
     * Empties every queue. The sidebar is rebuilt whole whenever anything
     * changes, and the selections are pushed again with it: without this a
     * torrent that has just been removed would keep its place here, and a
     * playlist somebody emptied would go on being fetched.
     */
    void forgetAll();

    /** the track being listened to: the cursor moves here and looks forward */
    void setPlaying( const QString &infoHash, int fileIndex );

    void clearPlaying();

    /** whether fetching ahead is switched on at all; read from the settings */
    bool isEnabled() const;

    /** re-reads the setting, then applies it */
    void reloadSettings();

    /**
     * What this file should be asking of the swarm right now. The rule lives
     * here and nowhere else, which is what makes it something a test can ask
     * about without a network, a disk or a window.
     */
    TorrentSession::FilePriority priorityFor( const QString &infoHash, int fileIndex ) const;

    /** hands every priority to the session again */
    void refresh();

Q_SIGNALS:
    /**
     * The queue has gone back to handing out real priorities after a spell of
     * handing out SkipFile: the cache has room again, or the fetching was
     * switched on.
     *
     * Whoever can wake a torrent has to hear about it, and that is the whole
     * reason this signal exists. A priority handed to a torrent the session is
     * not holding goes nowhere and nothing repeats it, so a torrent left asleep
     * while the queue was stopped would go on sleeping through the moment it
     * was allowed to fetch again — and nothing anywhere would say so. This
     * class cannot wake anything itself: it knows info hashes and file indexes
     * and has never heard of a playlist, let alone of the magnet a torrent is
     * reopened from.
     */
    void fetchingResumed();

private Q_SLOTS:
    /**
     * The cache filling up is not an event anybody sends: it happens while
     * bytes arrive. So it is looked at on a timer, and the priorities are
     * handed out again only when the answer has actually changed.
     */
    void checkCacheRoom();

private:
    DownloadQueue();

    /** where a file sits in its torrent's queue, or -1 if it is not in it */
    int positionOf( const QString &infoHash, int fileIndex ) const;

    QHash<QString, QList<int>> m_selections;

    QString m_playingHash;
    int     m_playingFile = -1;

    bool m_enabled = true;
    bool m_hadRoom = true;

    QTimer *m_roomTimer;
};

#endif // DOWNLOADQUEUE_H
