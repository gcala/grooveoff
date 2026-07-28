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

#include "../GrooveOffNamespace.h"
#include "../Roles.h"
#include "../../torrent/TorrentSession.h"

#include <QDragEnterEvent>
#include <QDropEvent>
#include <QIcon>
#include <QMimeData>

PlaylistList::PlaylistList( QWidget *parent )
    : QListWidget( parent )
{
    setAcceptDrops( true );
    setDragEnabled( false );
    setDragDropMode( QAbstractItemView::DropOnly );

    // Every active torrent reports every second, whether or not anything of it
    // is being listened to, and that is what the leds are made of: the row
    // worth looking at is the one being considered rather than the one already
    // playing.
    connect( The::torrentSession(), &TorrentSession::statsUpdated, this,
             [this]( const QString &infoHash, int peers, int seeds, qint64, qint64 ) {
        m_peers.insert( infoHash, qMakePair( peers, seeds ) );
        updateHealth( infoHash );
    } );

    // An announce is answered long after the play that caused it, so the count
    // has to come to the row rather than wait to be asked for.
    connect( The::torrentSession(), &TorrentSession::trackerReplied, this,
             [this]( const QString &infoHash, int ) {
        updateHealth( infoHash );
    } );
}

PlaylistList::~PlaylistList() = default;

void PlaylistList::addPlaylist( const PlaylistStore::PlaylistInfo &playlist )
{
    QListWidgetItem *row = new QListWidgetItem( playlist.name );

    row->setData( PlaylistRoles::Id, playlist.id );
    row->setData( PlaylistRoles::Kind, static_cast<int>( playlist.kind ) );
    row->setData( PlaylistRoles::InfoHash, playlist.infoHash );
    row->setData( PlaylistRoles::TrackCount, playlist.trackCount );

    row->setIcon( playlist.kind == PlaylistStore::CustomPlaylist
                  ? QIcon::fromTheme( QLatin1String( "view-media-playlist" ),
                                      QIcon( QLatin1String( ":/resources/Playlist.png" ) ) )
                  : QIcon::fromTheme( QLatin1String( "network-workgroup" ),
                                      QIcon( QLatin1String( ":/resources/grooveoff.png" ) ) ) );

    row->setToolTip( tr( "%n track(s)", "", playlist.trackCount ) );

    addItem( row );

    // A playlist made by hand holds tracks of several torrents, so the health
    // of "the" source is a question without an answer for it, and it is given
    // no Health at all: the delegate draws a led for the rows that have one.
    if( playlist.kind != PlaylistStore::CustomPlaylist )
        updateHealth( playlist.infoHash );
}

/*!
  \brief updateHealth: what one torrent's led says, and why

  Three states, and the third is the reason this can be shown at all: a torrent
  that has never been handed to the session has no peers to count, and saying
  so is not the same as saying it has none. Reactivation is lazy, so right
  after start up that is nearly every row.
*/
void PlaylistList::updateHealth( const QString &infoHash )
{
    if( infoHash.isEmpty() )
        return;

    for( int i = 0; i < count(); ++i ) {
        QListWidgetItem *row = item( i );
        if( row->data( PlaylistRoles::InfoHash ).toString() != infoHash )
            continue;

        const QPair<int, int> counts = m_peers.value( infoHash, qMakePair( 0, 0 ) );
        const bool active = The::torrentSession()->isActive( infoHash );

        GrooveOff::SourceHealth health = GrooveOff::UnknownSource;
        if( active )
            health = counts.first > 0 ? GrooveOff::LiveSource : GrooveOff::SilentSource;

        row->setData( PlaylistRoles::Health, static_cast<int>( health ) );

        QStringList lines;
        lines << tr( "%n track(s)", "", row->data( PlaylistRoles::TrackCount ).toInt() );

        if( !active ) {
            lines << tr( "Asleep: it wakes at the first play" );
        } else {
            lines << tr( "%n peer(s)", "", counts.first ) + QLatin1String( ", " )
                     + tr( "%n seed(s)", "", counts.second );

            // Where those peers came from. A torrent whose tracker answered has
            // somewhere to go back to when the ones it holds go quiet, and
            // without this a tracker that knows nobody reads like an empty swarm.
            const int fromTracker = The::torrentSession()->trackerPeers( infoHash );
            if( fromTracker >= 0 )
                lines << tr( "%n peer(s) from the tracker", "", fromTracker );
        }

        row->setToolTip( lines.join( QLatin1Char( '\n' ) ) );
        return;
    }
}

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
