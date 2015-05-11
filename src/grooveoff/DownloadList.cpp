/*
    GrooveOff - Offline Grooveshark.com music
    Copyright (C) 2013-2015  Giuseppe Calà <jiveaxe@gmail.com>

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
    connect( model(), SIGNAL(layoutChanged()), SLOT(reloadPlaylist()) );
}

void DownloadList::reloadPlaylist()
{
    The::playlist()->clear();
    
    foreach( QAction *action, The::actionCollection()->getMenu( "playlistMenu" )->actions() ) {
        The::actionCollection()->getMenu( "playlistMenu" )->removeAction( action );
        delete action;
    }

    for( int i = 0; i < count(); i++ ) {
        if( ITEM( i )->downloadState() == GrooveOff::FinishedState ) {
            The::playlist()->appendItem( ITEM( i )->playlistItem() );
            PlaylistAction *action = new PlaylistAction( ITEM( i )->playlistItem(), The::actionCollection()->getMenu( "playlistMenu" ) );
            The::actionCollection()->getMenu( "playlistMenu" )->addAction( (QAction *)action );
            
            connect( action, SIGNAL(triggered()),  SLOT(actionTriggered()) );
        }
    }
}

/*!
  \brief removeFailedDeletedAborted : remove failed/deleted/aborted items from download list
  \return void
*/
void DownloadList::removeFailedAborted()
{
    removeItemsFromState( GrooveOff::AbortedState) ;
    removeItemsFromState( GrooveOff::ErrorState );
}

/*!
  \brief removeDownloaded : remove successfully downloaded items from download list
  \return void
*/
void DownloadList::removeDownloaded()
{
    removeItemsFromState( GrooveOff::FinishedState );
}

void DownloadList::removeItemsFromState( GrooveOff::DownloadState state )
{
    // stop player if we're removing successfull downloaded tracks
    if( state == GrooveOff::FinishedState )
        The::audioEngine()->stop();
    
    for( int i = count() - 1; i >= 0; i-- ) {
        GrooveOff::DownloadState currentState = ITEM( i )->downloadState();
        if( currentState == state ) {
            The::audioEngine()->removingTrack( ITEM( i )->playlistItem() );
            QListWidgetItem *item = takeItem( i );
            removeItemWidget( item );
            delete item;
        }
    }

    // clear playlist if we're removing successfull downloaded tracks
    if( state == GrooveOff::FinishedState )
        The::playlist()->clear();
}

void DownloadList::abortAllDownloads()
{
    for( int i = count() - 1; i >= 0; i-- ) {
        GrooveOff::DownloadState state = ITEM( i )->downloadState();
        if( state == GrooveOff::DownloadingState || state == GrooveOff::QueuedState ) {
            ITEM( i )->abortDownload();
        }
    }
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

