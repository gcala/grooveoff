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

#ifndef PLAYLISTLIST_H
#define PLAYLISTLIST_H

#include "../../db/PlaylistStore.h"

#include <QHash>
#include <QListWidget>
#include <QPair>

/**
  * @class PlaylistList
  * @short The sidebar: every playlist worth showing, of both kinds.
  *
  * Drop only. Tracks are dragged onto a playlist from the list of tracks, never
  * the other way round, and a playlist is not dragged anywhere at all: the
  * order of the sidebar is the order of the query, so there is nothing here for
  * a drag to rearrange.
  *
  * A drop is refused on a torrent playlist, and that refusal is the rule the
  * whole design rests on rather than an omission: what a torrent playlist holds
  * is chosen in the file selector, from the files that torrent actually has.
  * Letting a track of another torrent be dropped in would make it a list of
  * things its own torrent cannot play.
  */
class PlaylistList : public QListWidget
{
    Q_OBJECT

public:
    explicit PlaylistList( QWidget *parent = nullptr );
    ~PlaylistList() override;

    /**
     * Adds one row, icon, tooltip and all. The window says which playlists
     * exist and this says what a row of them looks like — including the led,
     * which the window has no reason to know about: the health of a source is
     * a property of the torrent behind the row, and the row is the only thing
     * that has to hear about it changing.
     */
    void addPlaylist( const PlaylistStore::PlaylistInfo &playlist );

    /** the playlist under the cursor, or 0 where there is no row */
    qint64 playlistAt( const QPoint &position ) const;

    qint64 currentPlaylistId() const;

    /** brings the row of this playlist forward without asking to open it again */
    void selectPlaylist( qint64 playlistId );

    /**
     * Whether tracks may be dropped on this playlist. Told apart from the drag
     * handling on purpose: what may be dropped where is a rule of the
     * application, and asking it directly is the only way to check it without
     * standing in for the window system.
     */
    bool acceptsTracks( qint64 playlistId ) const;

protected:
    void dragEnterEvent( QDragEnterEvent *event ) override;
    void dragMoveEvent( QDragMoveEvent *event ) override;
    void dropEvent( QDropEvent *event ) override;

Q_SIGNALS:
    void tracksDropped( qint64 playlistId, const QList<qint64> &trackIds );

private:
    /** the led of one torrent, and the tooltip that spells out what it means */
    void updateHealth( const QString &infoHash );

    /** peers and seeds of every torrent the session is holding */
    QHash<QString, QPair<int, int>> m_peers;
};

#endif // PLAYLISTLIST_H
