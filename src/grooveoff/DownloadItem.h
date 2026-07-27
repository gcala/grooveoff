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


#ifndef DOWNLOADITEM_H
#define DOWNLOADITEM_H

#include "GrooveOffNamespace.h"
#include "PlaylistItem.h"

#include <QWidget>

namespace Ui {
class DownloadItem;
}

class QEnterEvent;

/**
  * @class DownloadItem
  * @short One track in a playlist.
  *
  * The progress bar no longer measures a download: it will show how much of
  * the file is buffered out of the torrent. The name stays for now, the widget
  * is renamed together with the rest of the interface.
  */
class DownloadItem : public QWidget
{
    Q_OBJECT

public:
    explicit DownloadItem( const PlaylistItemPtr &playlistItemPtr, QWidget *parent = nullptr );
    ~DownloadItem() override;

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

    bool operator==( DownloadItem & ) const;

public Q_SLOTS:
    void setPlayerState( GrooveOff::PlayerState );

Q_SIGNALS:
    void pauseResumePlaying();
    void stateChangedSignal();
    void play( QString );
    void reloadPlaylist();
    void removeMe( DownloadItem * );

    /** the user refused this file for good, which is not the same as removing it */
    void excludeRequested( DownloadItem * );

    /** copy this track into a custom playlist, leaving it where it is */
    void addToPlaylistRequested( DownloadItem *, qint64 playlistId );

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
    Ui::DownloadItem      *ui;
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

#endif // DOWNLOADITEM_H
