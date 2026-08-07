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
#include "../Utility.h"
#include "../../torrent/TorrentSession.h"

#include <QDragEnterEvent>
#include <QDropEvent>
#include <QIcon>
#include <QMimeData>
#include <QTimer>

namespace {
    /**
     * Half a second and a bit. Fast enough to read as activity, slow enough not
     * to be the thing you end up watching instead of using the program.
     */
    const int blinkIntervalMs = 600;
}

PlaylistList::PlaylistList( QWidget *parent )
    : QListWidget( parent ),
      m_blinkTimer( new QTimer( this ) )
{
    setAcceptDrops( true );
    setDragEnabled( false );
    setDragDropMode( QAbstractItemView::DropOnly );

    m_blinkTimer->setInterval( blinkIntervalMs );
    connect( m_blinkTimer, &QTimer::timeout, this, &PlaylistList::blink );

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
    QListWidgetItem *row = new QListWidgetItem( Utility::playlistName( playlist ) );

    row->setData( PlaylistRoles::Id, playlist.id );
    row->setData( PlaylistRoles::Kind, static_cast<int>( playlist.kind ) );
    row->setData( PlaylistRoles::InfoHash, playlist.infoHash );
    row->setData( PlaylistRoles::TrackCount, playlist.trackCount );

    if( playlist.pinned ) {
        row->setIcon( QIcon::fromTheme( QLatin1String( "media-playlist-append" ),
                                        QIcon::fromTheme( QLatin1String( "view-media-playlist" ),
                                                          QIcon( QLatin1String( ":/resources/Playlist.png" ) ) ) ) );
    } else {
        row->setIcon( playlist.kind == PlaylistStore::CustomPlaylist
                      ? QIcon::fromTheme( QLatin1String( "view-media-playlist" ),
                                          QIcon( QLatin1String( ":/resources/Playlist.png" ) ) )
                      : QIcon::fromTheme( QLatin1String( "network-workgroup" ),
                                          QIcon( QLatin1String( ":/resources/grooveoff.png" ) ) ) );
    }

    row->setToolTip( tr( "%n track(s)", "", playlist.trackCount ) );

    addItem( row );

    // A playlist made by hand holds tracks of several torrents, so the health
    // of "the" source is a question without an answer for it, and it is given
    // no Health at all: the delegate draws a led for the rows that have one.
    if( playlist.kind != PlaylistStore::CustomPlaylist )
        updateHealth( playlist.infoHash );
}

/*!
  \brief addPending: the row of a magnet that has not answered yet

  It looks like a torrent playlist because that is what it is about to become,
  and it carries no id because there is nothing in the catalogue to point at.
  Everything that asks a row what playlist it is gets 0 and steps aside.
*/
void PlaylistList::addPending( const QString &infoHash, const QString &name )
{
    if( infoHash.isEmpty() || hasRow( infoHash ) )
        return;

    QListWidgetItem *row = new QListWidgetItem( name );

    row->setData( PlaylistRoles::Id, static_cast<qint64>( 0 ) );
    row->setData( PlaylistRoles::Kind, static_cast<int>( PlaylistStore::TorrentPlaylist ) );
    row->setData( PlaylistRoles::InfoHash, infoHash );
    row->setData( PlaylistRoles::TrackCount, 0 );
    row->setData( PlaylistRoles::Searching, true );

    row->setIcon( QIcon::fromTheme( QLatin1String( "network-workgroup" ),
                                    QIcon( QLatin1String( ":/resources/grooveoff.png" ) ) ) );

    addItem( row );

    updatePendingRow( row );
    updateBlinking();
}

bool PlaylistList::hasRow( const QString &infoHash ) const
{
    if( infoHash.isEmpty() )
        return false;

    for( int i = 0; i < count(); ++i ) {
        if( item( i )->data( PlaylistRoles::InfoHash ).toString() == infoHash )
            return true;
    }

    return false;
}

void PlaylistList::setPendingSearching( const QString &infoHash, bool searching )
{
    for( int i = 0; i < count(); ++i ) {
        QListWidgetItem *row = item( i );
        if( !isPending( row ) || row->data( PlaylistRoles::InfoHash ).toString() != infoHash )
            continue;

        row->setData( PlaylistRoles::Searching, searching );
        updatePendingRow( row );
    }

    updateBlinking();
}

bool PlaylistList::isPending( const QListWidgetItem *row )
{
    // No playlist of the catalogue has an id of 0, and no custom playlist has
    // an info hash, so the pair says it without a role of its own.
    return row && row->data( PlaylistRoles::Id ).toLongLong() == 0
               && !row->data( PlaylistRoles::InfoHash ).toString().isEmpty();
}

/*!
  \brief updatePendingRow: the led and the tooltip of a row that is still waiting

  Kept apart from updateHealth() because the two answer different questions.
  There, "asleep" and "nobody there" are the interesting states; here the
  torrent is by definition awake and the interesting thing is whether anybody
  has answered at all. And there is no track count to put at the top of the
  tooltip, because a track list is exactly what is missing.
*/
void PlaylistList::updatePendingRow( QListWidgetItem *row )
{
    const QString infoHash = row->data( PlaylistRoles::InfoHash ).toString();
    const QPair<int, int> counts = m_peers.value( infoHash, qMakePair( 0, 0 ) );
    const bool searching = row->data( PlaylistRoles::Searching ).toBool();

    // The led of a row that is waiting answers one question and it is not "are
    // there peers": it is "am I still looking". Peers were tried as the answer
    // and measured, and the number is no use for this — a magnet with a made-up
    // info hash, one nobody in the world is sharing, reports five peers within
    // two seconds, because the count includes every connection being attempted.
    // Hung on that, the blinking stopped after one tick and nobody would ever
    // have seen it. So it runs until the track list arrives or the wait is
    // called off, and the peer count says what it means in the tooltip, where
    // it is labelled.
    //
    // While it blinks the led belongs to the timer, and writing here would put
    // it back to whatever this call thinks between two ticks. The one exception
    // is the very first moment: the delegate draws no led at all where the role
    // is unset, so without this the row would stand there bare until the first
    // tick six hundred milliseconds later.
    if( !searching )
        row->setData( PlaylistRoles::Health, static_cast<int>( GrooveOff::SilentSource ) );
    else if( !row->data( PlaylistRoles::Health ).isValid() )
        row->setData( PlaylistRoles::Health, static_cast<int>( GrooveOff::UnknownSource ) );

    QStringList lines;
    lines << ( searching ? tr( "Looking for the track list…" )
                         : tr( "The track list has not arrived" ) );

    if( counts.first > 0 ) {
        lines << tr( "%n peer(s)", "", counts.first ) + QLatin1String( ", " )
                 + tr( "%n seed(s)", "", counts.second );
    }

    const int fromTracker = The::torrentSession()->trackerPeers( infoHash );
    if( fromTracker >= 0 )
        lines << tr( "%n peer(s) from the tracker", "", fromTracker );

    row->setToolTip( lines.join( QLatin1Char( '\n' ) ) );
}

void PlaylistList::blink()
{
    m_blinkOn = !m_blinkOn;

    for( int i = 0; i < count(); ++i ) {
        QListWidgetItem *row = item( i );
        if( !isPending( row ) || !row->data( PlaylistRoles::Searching ).toBool() )
            continue;

        // Between the two states that are already drawn, rather than a third
        // one: what a blink has to be distinguishable from is a led that is
        // simply off, and alternating with it is the clearest way to say so.
        row->setData( PlaylistRoles::Health, static_cast<int>( m_blinkOn ? GrooveOff::SilentSource
                                                                         : GrooveOff::UnknownSource ) );
    }
}

void PlaylistList::updateBlinking()
{
    bool searching = false;
    for( int i = 0; i < count() && !searching; ++i ) {
        const QListWidgetItem *row = item( i );
        searching = isPending( row ) && row->data( PlaylistRoles::Searching ).toBool();
    }

    if( searching == m_blinkTimer->isActive() )
        return;

    if( searching ) {
        m_blinkTimer->start();
        return;
    }

    // Or it goes on ticking over a sidebar where nothing is waiting for
    // anything, for as long as the program is open.
    m_blinkTimer->stop();
    m_blinkOn = false;
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

        if( isPending( row ) ) {
            updatePendingRow( row );
            updateBlinking();
            continue;
        }

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
    }
}

void PlaylistList::setPlayingPlaylist( qint64 playlistId )
{
    for( int i = 0; i < count(); ++i ) {
        QListWidgetItem *row = item( i );
        const bool playing = row->data( PlaylistRoles::Id ).toLongLong() == playlistId && playlistId > 0;

        if( row->data( PlaylistRoles::Playing ).toBool() != playing )
            row->setData( PlaylistRoles::Playing, playing );
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

QString PlaylistList::pendingAt( const QPoint &position ) const
{
    const QListWidgetItem *row = itemAt( position );

    return isPending( row ) ? row->data( PlaylistRoles::InfoHash ).toString() : QString();
}

QString PlaylistList::currentPendingHash() const
{
    const QListWidgetItem *row = currentItem();

    return isPending( row ) ? row->data( PlaylistRoles::InfoHash ).toString() : QString();
}

void PlaylistList::selectPending( const QString &infoHash )
{
    for( int i = 0; i < count(); ++i ) {
        const QListWidgetItem *row = item( i );
        if( !isPending( row ) || row->data( PlaylistRoles::InfoHash ).toString() != infoHash )
            continue;

        // Blocked for the reason selectPlaylist() blocks: this brings a row
        // forward, it does not stand in for somebody clicking it.
        const bool blocked = blockSignals( true );
        setCurrentRow( i );
        blockSignals( blocked );
        return;
    }
}

void PlaylistList::selectPlaylist( qint64 playlistId )
{
    // Nothing open is not a playlist to go and select, and since provisional
    // rows carry an id of 0 the loop below would find one and bring it forward
    // — putting the sidebar on a row nobody asked to look at.
    if( playlistId <= 0 )
        return;

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
