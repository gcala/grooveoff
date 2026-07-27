/*
    GrooveOff - Offline Grooveshark.com music
    Copyright ( C ) 2013-2014  Giuseppe Calà <jiveaxe@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    ( at your option ) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/


#include "DownloadItem.h"
#include "widgets/TimerButton.h"
#include "Utility.h"
#include "ui_DownloadItem.h"
#include "AudioEngine.h"
#include "Playlist.h"
#include "MainWindow.h"
#include "../db/PlaylistStore.h"
#include "../torrent/TorrentSession.h"

#include "TrackTags.h"

#include <QContextMenuEvent>
#include <QDebug>
#include <QFile>
#include <QGraphicsDropShadowEffect>
#include <QLocale>
#include <QHash>
#include <QMenu>

/*!
  \brief Class representing a track in a playlist
*/
DownloadItem::DownloadItem( const PlaylistItemPtr &playlistItemPtr, QWidget *parent )
    : QWidget( parent )
    , ui( new Ui::DownloadItem )
    , m_playlistItem( playlistItemPtr )
    , m_trackState( GrooveOff::DormantState )
    , m_playerState( GrooveOff::PlayerState::Stopped )
{
    ui->setupUi( this );
    setupUi();
    setupConnections();

    // Whatever is already in the cache can be played straight away; everything
    // else waits for its torrent to be woken up.
    // Whether the file exists says nothing: the cache is made of sparse files,
    // so one arrived piece creates a file of the full size with almost nothing
    // in it. What a row can honestly say is whether its torrent is awake.
    if( m_playlistItem->track()
        && The::torrentSession()->hasMetadata( m_playlistItem->track()->infoHash() ) ) {
        m_trackState = GrooveOff::ReadyState;
    }

    stateChanged();
}

DownloadItem::~DownloadItem()
{
    if( ui->timerButton->isCountdownStarted() ) {
        ui->timerButton->stopCountdown();
        removeSong();
    }

    delete ui;
}

/*!
  \brief setupUi: setup widgets
  \return void
*/
void DownloadItem::setupUi()
{
    ui->coverLabel->setScaledContents( true );
    ui->coverLabel->setFixedSize( QSize( Utility::coverSize,Utility::coverSize ) );
    loadCover();

    ui->coverLabel->setToolTip( m_playlistItem->track()->filePath() );

    QGraphicsDropShadowEffect *coverShadow = new QGraphicsDropShadowEffect( this );
    coverShadow->setBlurRadius( 15.0 );
    coverShadow->setColor( palette().color( QPalette::Shadow ) );
    coverShadow->setOffset( 0.0 );

    ui->coverLabel->setGraphicsEffect( coverShadow );

    ui->titleLabel->setFont( Utility::font( QFont::Bold ) );
    ui->titleLabel->setText( m_playlistItem->displayTitle() );
    ui->titleLabel->setToolTip( m_playlistItem->displayTitle() );
    ui->titleLabel->setSizePolicy( QSizePolicy::MinimumExpanding, QSizePolicy::Preferred ); // fix hidden label

    ui->artist_albumLabel->setText( m_playlistItem->displaySubtitle() );
    ui->artist_albumLabel->setToolTip( m_playlistItem->displaySubtitle() );
    ui->artist_albumLabel->setSizePolicy( QSizePolicy::MinimumExpanding, QSizePolicy::Preferred ); // fix hidden label

    // set shadows
    ui->titleLabel->enableShadow( true );
    ui->artist_albumLabel->enableShadow( true );

    ui->playButton->setButtonEnabled( false );
    ui->playButton->setPlaying( false );

    ui->progressBar->setMinimum( 0 );
    ui->progressBar->setValue( 0 );
    ui->progressBar->setFixedWidth( 100 );
    ui->progressBar->setFixedHeight( 22 );

    ui->playButton->setType( IconButton::PlayPause );
    ui->playButton->setToolTip( tr( "Play" ) );

    ui->deleteButton->setType( IconButton::Trash );
    ui->deleteButton->setToolTip( tr( "Remove track from the playlist" ) );

    ui->infoIcon->setType( IconButton::Clock );

    ui->timerButton->setFixedSize( QSize( Utility::buttonSize,Utility::buttonSize ) );
    ui->barsWidget->setFixedSize( QSize( Utility::buttonSize,Utility::buttonSize ) );
    ui->deleteButton->setFixedSize( QSize( Utility::buttonSize,Utility::buttonSize ) );
    ui->infoIcon->setFixedSize( QSize( Utility::buttonSize,Utility::buttonSize ) );
    ui->playButton->setFixedSize( QSize( Utility::buttonSize,Utility::buttonSize ) );

    // seems that gtk DEs use different default values than Qt...
    ui->animationLayout->setContentsMargins( 0,4,0,4 );
    ui->mainLayout->setContentsMargins( 4,4,4,4 );
    ui->timerLayout->setContentsMargins( 0,4,0,4 );
    ui->infoIconLayout->setContentsMargins( 0,4,0,4 );
    ui->infoMessageLayout->setContentsMargins( 0,4,0,4 );
    ui->progressLayout->setContentsMargins( 0,4,0,4 );
    ui->deleteLayout->setContentsMargins( 0,4,0,4 );
    ui->playLayout->setContentsMargins( 0,4,0,4 );
    ui->songWidgetLayout->setContentsMargins( 1,1,0,2 );
    ui->songWidgetLayout->setHorizontalSpacing( 5 );

    ui->timerWidget->setVisible( false );
    ui->deleteWidget->setVisible( false );
    ui->infoMessageWidget->setVisible( false );
}

/*!
  \brief setupConnections: setup widgets connections
  \return void
*/
void DownloadItem::setupConnections()
{
    connect( m_playlistItem.data(), &PlaylistItem::reloadCover, this, &DownloadItem::loadCover );
    connect( m_playlistItem.data(), &PlaylistItem::stateChanged, this, &DownloadItem::setPlayerState );

    connect( ui->playButton, &SvgButton::buttonClicked, this, &DownloadItem::playSong );
    connect( ui->timerButton, &TimerButton::clicked, this, &DownloadItem::timerButtonClicked );
    connect( ui->timerButton, &TimerButton::countdownFinished, this, &DownloadItem::removeSong );
    connect( ui->deleteButton, &SvgButton::buttonClicked, this, &DownloadItem::deleteItem );
}

void DownloadItem::setTrackState( GrooveOff::TrackState state )
{
    if( m_trackState == state )
        return;

    m_trackState = state;
    stateChanged();
}

void DownloadItem::setBuffered( qint64 bytesAvailable, qint64 bytesTotal )
{
    // The bar counts in kilobytes: a QProgressBar works on ints, and a forty
    // megabyte flac would overflow one.
    ui->progressBar->setMaximum( bytesTotal > 0 ? int( bytesTotal / 1024 ) : 1 );
    ui->progressBar->setValue( int( bytesAvailable / 1024 ) );

    ui->progressBar->setToolTip( tr( "%1 of %2 buffered" )
                                     .arg( QLocale().formattedDataSize( bytesAvailable ),
                                           QLocale().formattedDataSize( bytesTotal ) ) );

    m_fullyBuffered = bytesTotal > 0 && bytesAvailable >= bytesTotal;

    // While a track streams, how far ahead of the player the file has got is
    // the one thing worth looking at, and hiding it the moment playback starts
    // takes it away exactly then. It goes when there is nothing left to say —
    // when the whole file is in the cache — and not a second earlier.
    if( m_trackState == GrooveOff::ReadyState )
        ui->progressWidget->setVisible( !m_fullyBuffered );
}

void DownloadItem::setPeerInfo( int peers, int seeds )
{
    if( m_trackState != GrooveOff::BufferingState && m_trackState != GrooveOff::StalledState )
        return;

    ui->infoMessage->setText( peers == 0 ? tr( "Waiting for peers" )
                                         : tr( "%n peer(s)", "", peers )
                                           + QLatin1String( ", " )
                                           + tr( "%n seed(s)", "", seeds ) );
}

/*!
  \brief stateChanged: handle state changes
  \return void
*/
void DownloadItem::stateChanged()
{
    emit stateChangedSignal();

    switch( m_trackState ) {
        case GrooveOff::DormantState:
            setDormantState();
            break;

        case GrooveOff::IdleState:
            setIdleState();
            break;

        case GrooveOff::BufferingState:
            setBufferingState();
            break;

        case GrooveOff::ReadyState:
            setReadyState();
            break;

        case GrooveOff::StalledState:
            setStalledState();
            break;

        default: // error state
            setErrorState();
    }

    update();
}

void DownloadItem::setDormantState()
{
    ui->animationWidget->setVisible( false );
    ui->barsWidget->stopAnimation();
    ui->progressWidget->setVisible( false );

    // Playable in the sense that pressing play wakes the torrent up.
    ui->playWidget->setVisible( true );
    ui->playButton->setButtonEnabled( true );

    ui->infoIconWidget->setVisible( false );
}

void DownloadItem::setIdleState()
{
    ui->animationWidget->setVisible( false );
    ui->barsWidget->stopAnimation();
    ui->progressWidget->setVisible( false );

    ui->playWidget->setVisible( true );
    ui->playButton->setButtonEnabled( true );

    ui->infoIconWidget->setVisible( true );
    ui->infoIcon->setType( IconButton::Clock );
    ui->infoMessage->setText( tr( "Not buffered" ) );
}

void DownloadItem::setBufferingState()
{
    // The bar measures the buffer, not a download: what it fills up with is
    // how much of the track can be listened to without waiting again.
    ui->infoIconWidget->setVisible( true );
    ui->infoIcon->setType( IconButton::Clock );
    ui->progressWidget->setVisible( true );

    if( m_playerState == GrooveOff::PlayerState::Stopped ) {
        ui->playWidget->setVisible( true );
        ui->animationWidget->setVisible( false );
        ui->barsWidget->stopAnimation();
    } else if( m_playerState == GrooveOff::PlayerState::Playing ) {
        ui->playWidget->setVisible( false );
        ui->animationWidget->setVisible( true );
        ui->barsWidget->startAnimation();
    } else {
        ui->playWidget->setVisible( false );
        ui->animationWidget->setVisible( true );
        ui->barsWidget->stopAnimation();
    }
}

void DownloadItem::setReadyState()
{
    ui->progressWidget->setVisible( !m_fullyBuffered );
    ui->infoIconWidget->setVisible( false );

    ui->playButton->setButtonEnabled( true );

    if( m_playerState == GrooveOff::PlayerState::Stopped ) {
        ui->playWidget->setVisible( true );
        ui->animationWidget->setVisible( false );
        ui->barsWidget->stopAnimation();
    } else if( m_playerState == GrooveOff::PlayerState::Playing ) {
        ui->playWidget->setVisible( false );
        ui->animationWidget->setVisible( true );
        ui->barsWidget->startAnimation();
    } else {
        ui->playWidget->setVisible( false );
        ui->animationWidget->setVisible( true );
        ui->barsWidget->stopAnimation();
    }
}

void DownloadItem::setStalledState()
{
    ui->animationWidget->setVisible( false );
    ui->barsWidget->stopAnimation();
    ui->progressWidget->setVisible( true );
    ui->playWidget->setVisible( false );

    // Not an error: the swarm may well wake up.
    ui->infoIcon->setType( IconButton::Clock );
    ui->infoIconWidget->setVisible( true );
    ui->infoMessage->setText( tr( "Waiting for peers" ) );
}

void DownloadItem::setErrorState()
{
    ui->playWidget->setVisible( false );
    ui->animationWidget->setVisible( false );
    ui->barsWidget->stopAnimation();
    ui->progressWidget->setVisible( false );

    ui->infoMessage->setText( tr( "Track not available" ) );

    ui->infoIcon->setType( IconButton::Warning );
    ui->infoIconWidget->setVisible( true );
}

/*!
  \brief playSong: play song
  \return void
*/
void DownloadItem::playSong() const
{
    The::audioEngine()->playItem( m_playlistItem );
}

/*!
  \brief startRemoving: prepare to remove a song
  \return void
*/
void DownloadItem::timerButtonClicked()
{
    ui->timerButton->stopCountdown();
    ui->timerWidget->setVisible( false );
    ui->deleteWidget->setVisible( true );
}

void DownloadItem::deleteItem()
{
    ui->timerWidget->setVisible( true );
    ui->timerButton->setToolTip( tr( "Abort removal" ) );
    ui->timerButton->startCountdown();
    ui->deleteWidget->setVisible( false );
}

bool DownloadItem::operator==( DownloadItem& right ) const
{
    return *m_playlistItem == *right.playlistItem();
}

/*!
  \brief removeSong: take the track out of the playlist
  \return void
*/
void DownloadItem::removeSong()
{
    // Nothing is deleted from disk here: the cache decides on its own when to
    // let go of the bytes, and the track stays in the catalogue.
    The::audioEngine()->removingTrack( m_playlistItem );
    The::playlist()->removeItem( m_playlistItem );

    emit removeMe( this );
    emit reloadPlaylist();
}

/*!
  \brief contextMenuEvent: the two removals, told apart

  Taking a track out of a playlist and refusing a file are different acts with
  different consequences the next time round, so they are two entries and never
  one. The bin button on the row is the first of the two.
*/
void DownloadItem::contextMenuEvent( QContextMenuEvent *event )
{
    QMenu menu( this );

    QAction *playAction = menu.addAction( QIcon::fromTheme( QLatin1String( "media-playback-start" ) ),
                                          tr( "Play" ) );

    // Every custom playlist, this one included when it is the open one: adding
    // a track twice to a list made by hand is a choice, not a mistake to stop.
    QHash<const QAction *, qint64> playlistOf;
    QMenu *addMenu = menu.addMenu( QIcon::fromTheme( QLatin1String( "list-add" ) ),
                                   tr( "Add to playlist" ) );

    const QList<PlaylistStore::PlaylistInfo> playlists = The::playlistStore()->visiblePlaylists();
    for( const PlaylistStore::PlaylistInfo &playlist : playlists ) {
        if( playlist.kind != PlaylistStore::CustomPlaylist )
            continue;

        playlistOf.insert( addMenu->addAction( playlist.name ), playlist.id );
    }

    addMenu->setEnabled( !playlistOf.isEmpty() );
    if( playlistOf.isEmpty() )
        addMenu->setTitle( tr( "Add to playlist (none yet)" ) );

    menu.addSeparator();
    QAction *removeAction = menu.addAction( QIcon::fromTheme( QLatin1String( "list-remove" ) ),
                                            tr( "Remove from the playlist" ) );

    QAction *excludeAction = nullptr;
    if( !m_inCustomPlaylist ) {
        excludeAction = menu.addAction( QIcon::fromTheme( QLatin1String( "edit-delete" ) ),
                                        tr( "Exclude this track" ) );
        excludeAction->setToolTip( tr( "Keeps it out of this torrent's playlist until you say otherwise" ) );
    }

    const QAction *chosen = menu.exec( event->globalPos() );

    if( chosen == playAction )
        playSong();
    else if( chosen == removeAction )
        removeSong();
    else if( excludeAction && chosen == excludeAction )
        emit excludeRequested( this );
    else if( chosen && playlistOf.contains( chosen ) )
        emit addToPlaylistRequested( this, playlistOf.value( chosen ) );
}

void DownloadItem::setInCustomPlaylist( bool custom )
{
    m_inCustomPlaylist = custom;
}

/*!
  \brief enterEvent: manage mouse movement
  \return void
*/
void DownloadItem::enterEvent( QEnterEvent* event )
{
    ui->deleteWidget->setVisible( !ui->timerButton->isCountdownStarted() );

    if( m_trackState != GrooveOff::ReadyState && m_trackState != GrooveOff::DormantState )
        ui->infoMessageWidget->setVisible( true );

    QWidget::enterEvent( event );
}

/*!
  \brief leaveEvent: manage mouse movement
  \return void
*/
void DownloadItem::leaveEvent( QEvent* event )
{
    ui->deleteWidget->setVisible( false );
    ui->infoMessageWidget->setVisible( false );

    QWidget::leaveEvent( event );
}

/*!
  \brief setPlayerState: this function sets the state of the player
         for icon synchronization
  \return void
*/
void DownloadItem::setPlayerState( GrooveOff::PlayerState state )
{
    m_playerState = state;
    stateChanged();
}

/*!
  \brief refreshLabels: takes up the tags read off the file

  Called once the head of the file is in the cache, which is the first moment
  at which there is anything to read. Until then a row shows the file name,
  which is a poor title but never a wrong one.
*/
void DownloadItem::refreshLabels()
{
    ui->titleLabel->setText( m_playlistItem->displayTitle() );
    ui->titleLabel->setToolTip( m_playlistItem->displayTitle() );

    ui->artist_albumLabel->setText( m_playlistItem->displaySubtitle() );
    ui->artist_albumLabel->setToolTip( m_playlistItem->displaySubtitle() );

    loadCover();
}

void DownloadItem::loadCover()
{
    // The picture frame of the file itself. Covers used to come from a
    // Grooveshark cdn that no longer exists, and there is no other source that
    // would not mean asking a stranger what the user is listening to.
    const QImage cover = TrackTags::cover( m_playlistItem->localPath() );

    if( !cover.isNull() ) {
        ui->coverLabel->setPixmap( QPixmap::fromImage( cover ).scaled( Utility::coverSize,
                                                                       Utility::coverSize,
                                                                       Qt::KeepAspectRatio,
                                                                       Qt::SmoothTransformation ) );
        return;
    }

    ui->coverLabel->setPixmap( QIcon::fromTheme( QLatin1String( "media-optical" ),
                                                 QIcon( QLatin1String( ":/resources/media-optical.png" ) )
                                               ).pixmap( Utility::coverSize ) );
}
