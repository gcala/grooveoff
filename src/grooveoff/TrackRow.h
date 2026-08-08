/*
    GrooveOff - Offline Grooveshark.com music
    Copyright (C) 2013  Giuseppe Calà <jiveaxe@gmail.com>

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


#ifndef TRACKROW_H
#define TRACKROW_H

#include "GrooveOffNamespace.h"
#include "PlaylistItem.h"

#include <QWidget>

namespace Ui {
class TrackRow;
}

class QEnterEvent;

/**
  * @class TrackRow
  * @short One track of the open playlist, as it appears on screen.
  *
  * A row and not an item, which is the distinction the name is chosen for:
  * TrackInfo is what a track is, PlaylistItem is that plus what the player is
  * doing with it, and this is the widget showing them — put into the list with
  * setItemWidget(), so there is no model and no delegate underneath it.
  *
  * Its progress bar measures the buffer and not a download: how much of the
  * track can be listened to without waiting again. It was called DownloadItem
  * when that was the same number.
  */
class TrackRow : public QWidget
{
    Q_OBJECT

public:
    explicit TrackRow( const PlaylistItemPtr &playlistItemPtr, QWidget *parent = nullptr );
    ~TrackRow() override;

    inline const PlaylistItemPtr playlistItem() {
        return m_playlistItem;
    }

    inline GrooveOff::TrackState trackState() {
        return m_trackState;
    }

    void setTrackState( GrooveOff::TrackState state );

    /** how much of the file is available, in bytes */
    void setBuffered( qint64 bytesAvailable, qint64 bytesTotal );

    /** what the swarm looks like, for the row that is being listened to */
    void setPeerInfo( int peers, int seeds );

    /** takes up the tags and the cover once they became readable */
    void refreshLabels();

    /** whether the row is showing the picture out of the file, not the fallback */
    bool hasCover() const;

    /** asks the file for its cover again, for a head that was still arriving */
    void refreshCover();

    bool operator==( TrackRow & ) const;

public Q_SLOTS:
    void setPlayerState( GrooveOff::PlayerState );

Q_SIGNALS:
    void pauseResumePlaying();
    void stateChangedSignal();
    void play( QString );
    void reloadPlaylist();

    /**
     * Take this track out of the playlist. The row asks and does not act: what
     * removing means depends on which kind of list is open and on where else
     * the track sits, and neither is a row's business — it is the window that
     * knows both, and that has to be able to ask one question for a whole
     * selection instead of one per row.
     */
    void removeRequested( TrackRow * );

    /** copy this track into a custom playlist, leaving it where it is */
    void addToPlaylistRequested( TrackRow *, qint64 playlistId );

protected:
    void leaveEvent( QEvent *event ) override;
    void enterEvent( QEnterEvent *event ) override;
    void contextMenuEvent( QContextMenuEvent *event ) override;

private Q_SLOTS:
    void playSong() const;
    void loadCover();

private:
    Ui::TrackRow      *ui;
    PlaylistItemPtr        m_playlistItem;
    GrooveOff::TrackState  m_trackState;
    GrooveOff::PlayerState m_playerState;

    /**
     * Whether the whole file is in the cache, hence whether the bar has
     * anything left to say. True until told otherwise on purpose: a row nobody
     * has measured has nothing to show, and starting at false would put an
     * empty bar on every row of the list the moment its torrent woke up.
     */
    bool                   m_fullyBuffered = true;

    /** the file whose cover is on screen, so that it is decoded once */
    QString                m_coverPath;

    void setupUi();
    void setupConnections();
    void stateChanged();
    void setDormantState();
    void setIdleState();
    void setBufferingState();
    void setReadyState();
    void setStalledState();
    void setErrorState();
};

#endif // TRACKROW_H
