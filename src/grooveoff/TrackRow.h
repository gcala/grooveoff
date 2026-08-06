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

    /**
     * Which kind of playlist this row is being shown in. It decides one thing
     * only: excluding a file is a refusal aimed at its torrent, so the entry is
     * not offered while looking at a list the user built by hand, where the
     * track is there because it was put there on purpose.
     */
    void setInCustomPlaylist( bool custom );

    bool operator==( TrackRow & ) const;

public Q_SLOTS:
    void setPlayerState( GrooveOff::PlayerState );

Q_SIGNALS:
    void pauseResumePlaying();
    void stateChangedSignal();
    void play( QString );
    void reloadPlaylist();
    void removeMe( TrackRow * );

    /** the user refused this file for good, which is not the same as removing it */
    void excludeRequested( TrackRow * );

    /** copy this track into a custom playlist, leaving it where it is */
    void addToPlaylistRequested( TrackRow *, qint64 playlistId );

protected:
    void leaveEvent( QEvent *event ) override;
    void enterEvent( QEnterEvent *event ) override;
    void contextMenuEvent( QContextMenuEvent *event ) override;

private Q_SLOTS:
    void playSong() const;
    void timerButtonClicked();
    void removeSong();
    void loadCover();
    void deleteItem();

private:
    Ui::TrackRow      *ui;
    PlaylistItemPtr        m_playlistItem;
    GrooveOff::TrackState  m_trackState;
    GrooveOff::PlayerState m_playerState;
    bool                   m_inCustomPlaylist = false;

    /**
     * Whether the whole file is in the cache, hence whether the bar has
     * anything left to say. True until told otherwise on purpose: a row nobody
     * has measured has nothing to show, and starting at false would put an
     * empty bar on every row of the list the moment its torrent woke up.
     */
    bool                   m_fullyBuffered = true;

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
