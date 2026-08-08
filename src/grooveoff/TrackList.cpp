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


#include "TrackList.h"
#include "TrackRow.h"
#include "Utility.h"
#include "Playlist.h"
#include "AudioEngine.h"
#include "ActionCollection.h"
#include "PlaylistAction.h"
#include "Roles.h"

#include <QDir>

#include <algorithm>
#include <QCursor>
#include <QDrag>
#include <QMenu>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QPainter>

#define ROW( x )  ( (TrackRow *)itemWidget( item( x ) ) )

TrackList::TrackList( QWidget *parent ) :
    QListWidget( parent )
{
    connect( model(), &QAbstractItemModel::layoutChanged, this, &TrackList::reloadPlaylist );
}

/*!
  \brief reloadPlaylist: rebuilds the menu that lists what is on screen

  It used to fill the queue the engine walks as well, which is what tied the
  two together: changing what was shown changed what would be played next. The
  queue belongs to the playlist playback started in and is built by the window,
  which is the only place that knows which one that is.
*/
void TrackList::reloadPlaylist()
{
    
    const QList< QAction * > playlistActions = The::actionCollection()->getMenu( "tracksMenu" )->actions();
    for( QAction *action : playlistActions ) {
        The::actionCollection()->getMenu( "tracksMenu" )->removeAction( action );
        delete action;
    }

    // Every track goes into the playlist, whatever its state: a dormant one is
    // still playable, pressing play is what wakes its torrent up.
    for( int i = 0; i < count(); i++ ) {
        if( ROW( i )->trackState() == GrooveOff::ErrorState )
            continue;

        PlaylistAction *action = new PlaylistAction( ROW( i )->playlistItem(), The::actionCollection()->getMenu( "tracksMenu" ) );
        The::actionCollection()->getMenu( "tracksMenu" )->addAction( (QAction *)action );

        connect( action, &QAction::triggered, this, &TrackList::actionTriggered );
    }
}

/*!
  \brief releaseRows: lets go of every row without anybody losing a track

  Emptying the list to draw another playlist in it is not removing tracks, and
  it used to be told as if it were: the engine was stopped and then handed each
  row in turn as one being taken away. Handed the row it was playing, it did
  what that means — stopped, dropped the track and moved to the next one — so
  clicking a second playlist while the first one played either killed the music
  or started another track of the playlist being left. Two symptoms, one cause.

  Nothing is lost by saying nothing. The engine holds its own reference to the
  track it is playing, so the track outlives the row showing it; what follows
  when it ends is decided by canGoNext(), which finds the track is not in the
  open playlist any more and stops there. Leaving a playlist is a good reason
  for nothing to follow.

  There used to be a second way out, removeAll(), which announced every row to
  the engine as one being taken away. It went with the Downloads menu that was
  its only caller: emptying the view is not emptying the playlist, and a track
  removed from the one and not the other came back at the next rebuild.

  And there used to be a third, removeRow(), for the gesture that takes a track
  away for good. That one went too: a removal now writes to lists nobody is
  looking at, so what is on screen is rebuilt from what was written rather than
  edited to match — the road every other change to a playlist already took.
*/
void TrackList::releaseRows()
{
    for( int i = count() - 1; i >= 0; i-- ) {
        QListWidgetItem *item = takeItem( i );
        removeItemWidget( item );
        delete item;
    }
}

QList< PlaylistItemPtr > TrackList::playlistItems() const
{
    QList< PlaylistItemPtr > tracks;
    for( int i = 0; i < count(); i++ ) {
        tracks << ROW( i )->playlistItem();
    }

    return tracks;
}

QStringList TrackList::mimeTypes() const
{
    return QListWidget::mimeTypes() << QString( GrooveOffMime::tracks );
}

QMimeData* TrackList::mimeData( const QList<QListWidgetItem *> &items ) const
{
    QMimeData *data = QListWidget::mimeData( items );
    if( !data )
        return nullptr;

    QList<qint64> ids;
    for( const QListWidgetItem *item : items ) {
        const qint64 id = item->data( SongRoles::TrackId ).toLongLong();
        if( id > 0 )
            ids << id;
    }

    if( ids.isEmpty() )
        return data;

    data->setData( GrooveOffMime::tracks, GrooveOffMime::encodeTracks( ids ) );

    return data;
}

/*!
  \brief dragPixmap: what the cursor carries, drawn from the rows themselves

  The view renders the *items* to make that picture, and the items of this list
  are empty: what a row looks like is a widget put on top of it with
  setItemWidget, and the view knows nothing about it. So the drag came out as a
  blank rectangle the size of the row — the right shape, with nothing in it.

  Grabbing the widgets is the whole answer. They are laid out where the view
  says they are, so several selected rows keep their spacing and come out as
  one block rather than a pile.
*/
QPixmap TrackList::dragPixmap( const QList<QListWidgetItem *> &items ) const
{
    QRect area;
    for( const QListWidgetItem *item : items )
        area = area.united( visualItemRect( item ) );

    if( area.isEmpty() )
        return QPixmap();

    const qreal ratio = devicePixelRatioF();
    QPixmap pixmap( area.size() * ratio );
    pixmap.setDevicePixelRatio( ratio );

    // The rows are drawn on the colour they are drawn on in the list. Left
    // transparent, the text of a row would be dragged over the desktop with
    // nothing behind it, and on a dark background it would disappear.
    pixmap.fill( viewport()->palette().color( viewport()->backgroundRole() ) );

    QPainter painter( &pixmap );
    for( QListWidgetItem *item : items ) {
        if( QWidget *row = itemWidget( item ) )
            painter.drawPixmap( visualItemRect( item ).topLeft() - area.topLeft(), row->grab() );
    }
    painter.end();

    return pixmap;
}

/*!
  \brief startDrag: the same as the base class, minus the one thing it does at the end

  QAbstractItemView removes the dragged rows itself when the drop is accepted
  as a move, and both drops here are accepted as moves. Neither wants it: a
  reorder is reported and the list is rebuilt from the order that was written,
  and a track dropped on a playlist of the sidebar is copied into it — it stays
  where it was, which is what dragging one there means.
*/
void TrackList::startDrag( Qt::DropActions supportedActions )
{
    const QList<QListWidgetItem *> items = selectedItems();
    if( items.isEmpty() )
        return;

    QMimeData *data = mimeData( items );
    if( !data )
        return;

    QRect area;
    for( const QListWidgetItem *item : items )
        area = area.united( visualItemRect( item ) );

    QDrag *drag = new QDrag( this );
    drag->setMimeData( data );
    drag->setPixmap( dragPixmap( items ) );

    // Where the picture sits under the cursor. Without it the block jumps so
    // that its corner is under the pointer, and what was grabbed by its middle
    // is suddenly held by an edge.
    drag->setHotSpot( viewport()->mapFromGlobal( QCursor::pos() ) - area.topLeft() );

    drag->exec( supportedActions, defaultDropAction() );
}

void TrackList::dragEnterEvent( QDragEnterEvent *event )
{
    // Only its own tracks: a list belongs to one playlist, and a track dragged
    // in from somewhere else would have to come from a playlist that is not
    // open, which there is no way to be looking at.
    if( event->source() == this && event->mimeData()->hasFormat( GrooveOffMime::tracks ) )
        event->acceptProposedAction();
    else
        event->ignore();
}

void TrackList::dragMoveEvent( QDragMoveEvent *event )
{
    if( event->source() != this ) {
        event->ignore();
        return;
    }

    // The base class draws the drop indicator, which is the only thing telling
    // the user where the row is about to land.
    QListWidget::dragMoveEvent( event );
}

void TrackList::dropEvent( QDropEvent *event )
{
    // The drag came from here, so the selection is what is being dragged, and
    // reading it back is exact where decoding the payload would not be.
    QList<int> rows;
    const QList<QListWidgetItem *> dragged = selectedItems();
    for( const QListWidgetItem *item : dragged )
        rows << row( item );

    if( event->source() != this || rows.isEmpty() ) {
        event->ignore();
        return;
    }

    std::sort( rows.begin(), rows.end() );

    const QModelIndex index = indexAt( event->position().toPoint() );

    int targetRow = index.isValid() ? index.row() : count();
    if( index.isValid() && dropIndicatorPosition() == QAbstractItemView::BelowItem )
        ++targetRow;

    event->acceptProposedAction();

    emit reorderRequested( rows, targetRow );
}

void TrackList::actionTriggered()
{
    PlaylistAction *action = ( PlaylistAction * )QObject::sender();
    The::audioEngine()->playItem( action->track() );
}

