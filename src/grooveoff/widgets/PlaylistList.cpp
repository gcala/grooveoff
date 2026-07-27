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

#include "PlaylistList.h"

#include "../Roles.h"
#include "../../db/PlaylistStore.h"

#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>

PlaylistList::PlaylistList( QWidget *parent )
    : QListWidget( parent )
{
    setAcceptDrops( true );
    setDragEnabled( false );
    setDragDropMode( QAbstractItemView::DropOnly );
}

PlaylistList::~PlaylistList() = default;

qint64 PlaylistList::playlistAt( const QPoint &position ) const
{
    const QListWidgetItem *row = itemAt( position );

    return row ? row->data( PlaylistRoles::Id ).toLongLong() : 0;
}

qint64 PlaylistList::currentPlaylistId() const
{
    const QListWidgetItem *row = currentItem();

    return row ? row->data( PlaylistRoles::Id ).toLongLong() : 0;
}

void PlaylistList::selectPlaylist( qint64 playlistId )
{
    for( int i = 0; i < count(); ++i ) {
        if( item( i )->data( PlaylistRoles::Id ).toLongLong() != playlistId )
            continue;

        // Without the block this would look like the user clicking the row,
        // and the window would answer by opening a playlist that is already
        // open — rebuilding the list under the track that is playing.
        const bool blocked = blockSignals( true );
        setCurrentRow( i );
        blockSignals( blocked );
        return;
    }
}

bool PlaylistList::acceptsTracks( qint64 playlistId ) const
{
    for( int i = 0; i < count(); ++i ) {
        const QListWidgetItem *row = item( i );
        if( row->data( PlaylistRoles::Id ).toLongLong() != playlistId )
            continue;

        return row->data( PlaylistRoles::Kind ).toInt() == PlaylistStore::CustomPlaylist;
    }

    return false;
}

void PlaylistList::dragEnterEvent( QDragEnterEvent *event )
{
    if( event->mimeData()->hasFormat( GrooveOffMime::tracks ) )
        event->acceptProposedAction();
    else
        event->ignore();
}

/*!
  \brief dragMoveEvent: says no over a row that cannot take the drop

  Refusing here rather than in dropEvent is what makes the cursor say so before
  the button is released. A drop that is silently thrown away afterwards reads
  as a bug, and there is no way for whoever tried it to tell the difference.
*/
void PlaylistList::dragMoveEvent( QDragMoveEvent *event )
{
    const qint64 playlistId = playlistAt( event->position().toPoint() );

    if( !event->mimeData()->hasFormat( GrooveOffMime::tracks ) || !acceptsTracks( playlistId ) ) {
        event->ignore();
        return;
    }

    event->acceptProposedAction();
}

void PlaylistList::dropEvent( QDropEvent *event )
{
    const qint64 playlistId = playlistAt( event->position().toPoint() );
    const QList<qint64> ids = GrooveOffMime::decodeTracks( event->mimeData() );

    if( !acceptsTracks( playlistId ) || ids.isEmpty() ) {
        event->ignore();
        return;
    }

    event->acceptProposedAction();
    emit tracksDropped( playlistId, ids );
}
