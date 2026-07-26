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


#include "DownloadList.h"
#include "DownloadItem.h"
#include "Utility.h"
#include "Playlist.h"
#include "AudioEngine.h"
#include "ActionCollection.h"
#include "PlaylistAction.h"

#include <QDir>
#include <QMenu>

#define ITEM( x )  ( (DownloadItem * )itemWidget( item( x ) ) )

DownloadList::DownloadList( QWidget *parent ) :
    QListWidget( parent )
{
    connect( model(), &QAbstractItemModel::layoutChanged, this, &DownloadList::reloadPlaylist );
}

void DownloadList::reloadPlaylist()
{
    The::playlist()->clear();
    
    const QList< QAction * > playlistActions = The::actionCollection()->getMenu( "playlistMenu" )->actions();
    for( QAction *action : playlistActions ) {
        The::actionCollection()->getMenu( "playlistMenu" )->removeAction( action );
        delete action;
    }

    // Every track goes into the playlist, whatever its state: a dormant one is
    // still playable, pressing play is what wakes its torrent up.
    for( int i = 0; i < count(); i++ ) {
        if( ITEM( i )->trackState() == GrooveOff::ErrorState )
            continue;

        The::playlist()->appendItem( ITEM( i )->playlistItem() );
        PlaylistAction *action = new PlaylistAction( ITEM( i )->playlistItem(), The::actionCollection()->getMenu( "playlistMenu" ) );
        The::actionCollection()->getMenu( "playlistMenu" )->addAction( (QAction *)action );

        connect( action, &QAction::triggered, this, &DownloadList::actionTriggered );
    }
}

/*!
  \brief removeErrored : remove the tracks that will not play however long one waits
  \return void
*/
void DownloadList::removeErrored()
{
    removeItemsFromState( GrooveOff::ErrorState );
}

/*!
  \brief removeAll : empty the list
  \return void
*/
void DownloadList::removeAll()
{
    The::audioEngine()->stop();

    for( int i = count() - 1; i >= 0; i-- ) {
        The::audioEngine()->removingTrack( ITEM( i )->playlistItem() );
        QListWidgetItem *item = takeItem( i );
        removeItemWidget( item );
        delete item;
    }

    The::playlist()->clear();
}

void DownloadList::removeItemsFromState( GrooveOff::TrackState state )
{
    for( int i = count() - 1; i >= 0; i-- ) {
        if( ITEM( i )->trackState() == state ) {
            The::audioEngine()->removingTrack( ITEM( i )->playlistItem() );
            QListWidgetItem *item = takeItem( i );
            removeItemWidget( item );
            delete item;
        }
    }

    reloadPlaylist();
}

QList< PlaylistItemPtr > DownloadList::playlistItems() const
{
    QList< PlaylistItemPtr > tracks;
    for( int i = 0; i < count(); i++ ) {
        tracks << ITEM( i )->playlistItem();
    }

    return tracks;
}

void DownloadList::removeItem( DownloadItem* downItem )
{
    The::audioEngine()->removingTrack( downItem->playlistItem() );
    
    for( int i = 0; i < count(); i++ ) {
        if( ITEM( i ) == downItem ) {
            QListWidgetItem *it = takeItem( i );
            removeItemWidget( it );
            delete it;
        }
    }
}

void DownloadList::actionTriggered()
{
    PlaylistAction *action = ( PlaylistAction * )QObject::sender();
    The::audioEngine()->playItem( action->track() );
}

