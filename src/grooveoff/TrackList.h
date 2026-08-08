/*
    GrooveOff - Offline Grooveshark.com music
    Copyright (C) 2013-2014  Giuseppe Calà <jiveaxe@gmail.com>

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


#ifndef TRACKLIST_H
#define TRACKLIST_H

#include "PlaylistItem.h"
#include "GrooveOffNamespace.h"

#include <QListWidget>
#include "GrooveOffNamespace.h"

class TrackRow;

/**
  * @class TrackList
  * @short The tracks of the playlist being looked at, one TrackRow each.
  *
  * The counterpart of PlaylistList: that one lists the playlists, this one
  * lists what is inside the chosen one. Which playlist that is has nothing to
  * do with which one is playing — the music goes on in the list it started in,
  * and this may well be showing another.
  */
class TrackList : public QListWidget
{
    Q_OBJECT
public:
    explicit TrackList( QWidget *parent = 0 );

    QList< PlaylistItemPtr > playlistItems() const;


public Q_SLOTS:
    void reloadPlaylist();

    /**
     * Empties the list to fill it again, which is not the same as removing
     * what is in it: nothing is announced to the engine, so a track being
     * played goes on being played.
     */
    void releaseRows();

    /** takes one track away for good, and tells the engine so */

Q_SIGNALS:
    /**
     * These rows were dropped back into this list, and belong at @p targetRow.
     * The move is reported rather than performed, and that is deliberate: the
     * rows carry a widget each, set with setItemWidget, and letting the view
     * move them means letting it destroy and recreate rows whose widget it
     * knows nothing about. Writing the new order and rebuilding the list from
     * it goes through the one path that is already right.
     *
     * Rows and not track ids, because a custom playlist may legitimately hold
     * the same track twice, and moving one copy must not move the other.
     */
    void reorderRequested( const QList<int> &rows, int targetRow );

protected:
    /**
     * The catalogue ids of the dragged tracks, added to what QListWidget puts
     * there anyway. Both formats travel together: the sidebar reads ours, and
     * Qt's own is what makes the view draw the drag at all.
     */
    QMimeData* mimeData( const QList<QListWidgetItem *> &items ) const override;
    QStringList mimeTypes() const override;

    /**
     * Carries a picture of the rows and does not remove them afterwards.
     * Both are things the base class gets wrong here, and for the same reason:
     * it knows the items, and everything about these rows is in a widget put
     * on top of them.
     */
    void startDrag( Qt::DropActions supportedActions ) override;

    void dragEnterEvent( QDragEnterEvent *event ) override;
    void dragMoveEvent( QDragMoveEvent *event ) override;
    void dropEvent( QDropEvent *event ) override;

private:
    /** the rows themselves, drawn into the picture the cursor carries */
    QPixmap dragPixmap( const QList<QListWidgetItem *> &items ) const;

private Q_SLOTS:
    void actionTriggered();
};

#endif // TRACKLIST_H
