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

#include <QDebug>
#include <QFile>
#include <QGraphicsDropShadowEffect>

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
    if( QFile::exists( m_playlistItem->localPath() ) )
        m_trackState = GrooveOff::ReadyState;

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

    // Buttons of the download era, gone with it. The .ui file is rewritten
    // together with the rest of the interface.
    ui->openFolderWidget->setVisible( false );
    ui->unqueueWidget->setVisible( false );
    ui->queueWidget->setVisible( false );
    ui->stopDownloadWidget->setVisible( false );
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
    ui->progressBar->setMaximum( bytesTotal > 0 ? bytesTotal : 1 );
    ui->progressBar->setValue( bytesAvailable );
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
    ui->infoIconWidget->setVisible( false );
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
    ui->progressWidget->setVisible( false );
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

void DownloadItem::loadCover()
{
    // Covers used to come from a Grooveshark cdn. They will come from the
    // ID3v2 picture frame once tags are read off the buffered file.
    ui->coverLabel->setPixmap( QIcon::fromTheme( QLatin1String( "media-optical" ),
                                                 QIcon( QLatin1String( ":/resources/media-optical.png" ) )
                                               ).pixmap( Utility::coverSize ) );
}
