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
#include <../libgrooveshark/apirequest.h>
#include "AudioEngine.h"
#include "Playlist.h"
#include "MainWindow.h"

#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QHBoxLayout>
#include <QDir>
#include <QDebug>
#include <QDesktopServices>
#include <QGraphicsDropShadowEffect>

using namespace GrooveShark;

/*!
  \brief Class representing an item in download list
*/
DownloadItem::DownloadItem( const PlaylistItemPtr &playlistItemPtr, QWidget *parent, const GrooveOff::Context &context )
    : QWidget( parent )
    , ui( new Ui::DownloadItem )
    , m_playlistItem( playlistItemPtr )
    , m_oneShot( true )
    , m_context( context )
{
    ui->setupUi( this );
    connect( m_playlistItem.data(), SIGNAL(reloadCover()), 
                                    SLOT(loadCover())
           );
    connect( m_playlistItem.data(), SIGNAL(stateChanged(Phonon::State)), 
                                    SLOT(setPlayerState(Phonon::State))
           );

    setupUi();

    setupConnections();

    if( m_context == GrooveOff::Track ) {
        m_downloadState = GrooveOff::FinishedState;
    } else {
        if( QFile::exists( m_playlistItem->path() + m_playlistItem->fileName() ) ) {
            downloadFinished( true );
        }
        else {
            m_downloadState = GrooveOff::QueuedState;
            // qDebug() << "GrooveOff ::" << "Queued download of" << m_playlistItem->song()->songName();
        }
    }

    m_playerState = Phonon::StoppedState;
    stateChanged();
}

/*!
  \brief DownloadItem: this is the DownloadItem destructor.
*/
DownloadItem::~DownloadItem()
{
    if( ui->timerButton->isCountdownStarted() ) {
        ui->timerButton->stopCountdown();
        if( m_context == GrooveOff::Track )
            emit removeMeFromSession( m_playlistItem->song()->songID() );
        else
            removeSong();
    }

    if( m_downloadState == GrooveOff::DownloadingState ) {
        m_downloader->stopDownload();
    }

    if( m_context == GrooveOff::Download )
        removeEmptyFolder( QFileInfo( m_playlistItem->path() + m_playlistItem->fileName() ).absoluteDir() );
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

    ui->coverLabel->setToolTip( m_playlistItem->fileName() );

    QGraphicsDropShadowEffect *coverShadow = new QGraphicsDropShadowEffect( this );
    coverShadow->setBlurRadius( 15.0 );
    coverShadow->setColor( palette().color( QPalette::Shadow ) );
    coverShadow->setOffset( 0.0 );

    ui->coverLabel->setGraphicsEffect( coverShadow );

    ui->titleLabel->setFont( Utility::font( QFont::Bold ) );
    ui->titleLabel->setText( m_playlistItem->song()->songName() );
    ui->titleLabel->setToolTip( m_playlistItem->song()->songName() );
    ui->titleLabel->setSizePolicy( QSizePolicy::MinimumExpanding, QSizePolicy::Preferred ); // fix hidden label

    ui->artist_albumLabel->setText( m_playlistItem->song()->artistName() + " - " + m_playlistItem->song()->albumName() );
    ui->artist_albumLabel->setToolTip( m_playlistItem->song()->songName() );
    ui->artist_albumLabel->setSizePolicy( QSizePolicy::MinimumExpanding, QSizePolicy::Preferred ); // fix hidden label
    
    // set shadow
    ui->titleLabel->enableShadow( true );
    ui->artist_albumLabel->enableShadow( true );
    
    if( m_context == GrooveOff::Track ) {
        ui->playWidget->setVisible( false );
        ui->progressBar->setVisible( false );
    } else {
        ui->playButton->setButtonEnabled( false );
        ui->playButton->setPlaying( false );

        ui->progressBar->setMinimum( 0 );
        ui->progressBar->setValue( 0 );
        ui->progressBar->setFixedWidth( 100 );
        ui->progressBar->setFixedHeight( 22 );
    }
    
    ui->playButton->setType( IconButton::PlayPause );
    ui->playButton->setToolTip( trUtf8( "Play" ) );
    
    ui->unqueueButton->setType( IconButton::Remove );
    ui->unqueueButton->setToolTip( trUtf8( "Remove track from queue" ) );
    
    ui->stopButton->setType( IconButton::Stop );
    ui->stopButton->setToolTip( trUtf8( "Stop track download" ) );
    
    ui->deleteButton->setType( IconButton::Trash );
    ui->deleteButton->setToolTip( trUtf8( "Delete track from disk" ) );
    
    ui->queueButton->setType( IconButton::Redownload );
    ui->queueButton->setToolTip( trUtf8( "Redownload track" ) );
    
    ui->openFolderButton->setType( IconButton::Browse );
    ui->openFolderButton->setToolTip( trUtf8( "Open folder" ) );
    
    ui->infoIcon->setType( IconButton::Clock );
    
    ui->timerButton->setFixedSize( QSize( Utility::buttonSize,Utility::buttonSize ) );
    ui->barsWidget->setFixedSize( QSize( Utility::buttonSize,Utility::buttonSize ) );
    ui->unqueueButton->setFixedSize( QSize( Utility::buttonSize,Utility::buttonSize ) );
    ui->stopButton->setFixedSize( QSize( Utility::buttonSize,Utility::buttonSize ) );
    ui->deleteButton->setFixedSize( QSize( Utility::buttonSize,Utility::buttonSize ) );
    ui->queueButton->setFixedSize( QSize( Utility::buttonSize,Utility::buttonSize ) );
    ui->openFolderButton->setFixedSize( QSize( Utility::buttonSize,Utility::buttonSize ) );
    ui->infoIcon->setFixedSize( QSize( Utility::buttonSize,Utility::buttonSize ) );
    ui->playButton->setFixedSize( QSize( Utility::buttonSize,Utility::buttonSize ) );

    // seems that gtk DEs use different default values than Qt...
    ui->animationLayout->setContentsMargins( 0,4,0,4 );
    ui->mainLayout->setContentsMargins( 4,4,4,4 );
    ui->timerLayout->setContentsMargins( 0,4,0,4 );
    ui->infoIconLayout->setContentsMargins( 0,4,0,4 );
    ui->infoMessageLayout->setContentsMargins( 0,4,0,4 );
    ui->openFolderLayout->setContentsMargins( 0,4,0,4 );
    ui->progressLayout->setContentsMargins( 0,4,0,4 );
    ui->stopLayout->setContentsMargins( 0,4,0,4 );
    ui->deleteLayout->setContentsMargins( 0,4,0,4 );
    ui->queueLayout->setContentsMargins( 0,4,0,4 );
    ui->unqueueLayout->setContentsMargins( 0,4,0,4 );
    ui->playLayout->setContentsMargins( 0,4,0,4 );
    ui->songWidgetLayout->setContentsMargins( 1,1,0,2 );
    ui->songWidgetLayout->setHorizontalSpacing( 5 );
}

/*!
  \brief setupConnections: setup widgets connections
  \return void
*/
void DownloadItem::setupConnections()
{
    if( m_context == GrooveOff::Download ) {
        connect( ui->playButton, SIGNAL(buttonClicked()), 
                                 SLOT(playSong())
               );
        
        connect( ui->openFolderButton, SIGNAL(buttonClicked()), 
                                       SLOT(openFolder())
               );
    }

    connect( ui->timerButton, SIGNAL(clicked()), 
                              SLOT(timerButtonClicked())
           );
    
    connect( ui->timerButton, SIGNAL(countdownFinished()), 
                              SLOT(removeSong())
           );
    
    connect( ui->unqueueButton, SIGNAL(buttonClicked()),
                                SLOT(unqueueItem())
           );
    
    connect( ui->stopButton, SIGNAL(buttonClicked()),
                             SLOT(stopDownload())
           );
    
    connect( ui->deleteButton, SIGNAL(buttonClicked()),
                               SLOT(deleteItem())
           );
    
    connect( ui->queueButton, SIGNAL(buttonClicked()),
                              SLOT(queueItem())
           );
}

/*!
  \brief stateChanged: handle state changes
  \return void
*/
void DownloadItem::stateChanged()
{
    emit stateChangedSignal();

    switch( m_downloadState ) {
        case GrooveOff::QueuedState:
            ui->timerWidget->setVisible( false );
            ui->animationWidget->setVisible( false );
            ui->barsWidget->stopAnimation();
            ui->stopWidget->setVisible( false );
            ui->deleteWidget->setVisible( false );
            ui->queueWidget->setVisible( false );
            ui->playWidget->setVisible( false );
            ui->progressWidget->setVisible( false );
            ui->openFolderWidget->setVisible( false );
            
            ui->infoIconWidget->setVisible( true );
            ui->infoIcon->setType( IconButton::Clock );
            
            ui->infoMessage->setText( trUtf8( "Queued" ) );
            ui->infoMessageWidget->setVisible( false );  // shown when mouse is hovering
            ui->unqueueWidget->setVisible( false );      // shown when mouse is hovering
            
            break;
            
        case GrooveOff::DownloadingState:
            ui->timerWidget->setVisible( false );
            ui->unqueueWidget->setVisible( false );
            ui->deleteWidget->setVisible( false );
            ui->queueWidget->setVisible( false );
            ui->infoIconWidget->setVisible( false );
            ui->infoMessageWidget->setVisible( false );
            ui->openFolderWidget->setVisible( false );
            
            if( m_playerState == Phonon::StoppedState ) {
                ui->animationWidget->setVisible( false );
                ui->barsWidget->stopAnimation();
                ui->playWidget->setVisible( true );
            } else if( m_playerState == Phonon::PlayingState ) {
                ui->playWidget->setVisible( false );
                ui->animationWidget->setVisible( true );
                ui->barsWidget->startAnimation();
            } else {
                ui->playWidget->setVisible( false );
                ui->animationWidget->setVisible( true );
                ui->barsWidget->stopAnimation();
            }
            
            ui->progressWidget->setVisible( true );
            ui->stopWidget->setVisible( true );
            break;
            
        case GrooveOff::FinishedState:
            ui->stopWidget->setVisible( false );
            ui->unqueueWidget->setVisible( false );
            ui->queueWidget->setVisible( false );
            ui->progressWidget->setVisible( false );
            ui->infoIconWidget->setVisible( false );
            ui->infoMessageWidget->setVisible( false );
            
            
            if( m_context == GrooveOff::Track ) {
                ui->playWidget->setVisible( false );
                ui->animationWidget->setVisible( false );
            } else {
                ui->playButton->setButtonEnabled( true );
                if( m_playerState == Phonon::StoppedState ) {
                    ui->playWidget->setVisible( true );
                    ui->animationWidget->setVisible( false );
                    ui->barsWidget->stopAnimation();
                } else if( m_playerState == Phonon::PlayingState ) {
                    ui->playWidget->setVisible( false );
                    ui->animationWidget->setVisible( true );
                    ui->barsWidget->startAnimation();
                } else {
                    ui->playWidget->setVisible( false );
                    ui->animationWidget->setVisible( true );
                    ui->barsWidget->stopAnimation();
                }
            }
            
            ui->timerWidget->setVisible( false );      // shown when countdown is on
            ui->openFolderWidget->setVisible( false ); // shown when mouse is hovering
            ui->deleteWidget->setVisible( false );     // shown when mouse is hovering
            
            break;
            
        case GrooveOff::AbortedState:
            ui->playWidget->setVisible( false );
            ui->animationWidget->setVisible( false );
            ui->barsWidget->stopAnimation();
            ui->progressWidget->setVisible( false );
            ui->timerWidget->setVisible( false );
            ui->stopWidget->setVisible( false );
            ui->unqueueWidget->setVisible( false );
            ui->deleteWidget->setVisible( false );
            ui->openFolderWidget->setVisible( false );
            
            ui->infoMessage->setText( trUtf8( "Aborted" ) );
            ui->infoMessageWidget->setVisible( false );    // shown when mouse is hovering
            ui->queueWidget->setVisible( false );          // shown when mouse is hovering
            
            ui->infoIcon->setType( IconButton::Aborted );
            ui->infoIconWidget->setVisible( true );

            break;
            
        default: // error state
            ui->playWidget->setVisible( false );
            ui->animationWidget->setVisible( false );
            ui->barsWidget->stopAnimation();
            ui->progressWidget->setVisible( false );
            ui->timerWidget->setVisible( false );
            ui->openFolderWidget->setVisible( false );
            
            ui->stopWidget->setVisible( false );
            ui->unqueueWidget->setVisible( false );
            ui->deleteWidget->setVisible( false );
            
            ui->infoMessage->setText( trUtf8( "Network or Server error" ) );
            ui->infoMessageWidget->setVisible( false );     // shown when mouse is hovering
            
            ui->infoIcon->setType( IconButton::Warning );
            ui->infoIconWidget->setVisible( true );
            
            ui->queueWidget->setVisible( false );           // shown when mouse is hovering
    }
    
    update();
}

/*!
  \brief startDownload: start song download
  \return void
*/
void DownloadItem::startDownload()
{
    m_oneShot = true;
    m_downloadState = GrooveOff::DownloadingState;
    stateChanged();
    m_downloader = ApiRequest::instance()->downloadSong( m_playlistItem->path(),
                                                         m_playlistItem->fileName(),
                                                         m_playlistItem->song()->songID(),
                                                         Utility::token );

    connect( m_downloader.data(), SIGNAL(progress(qint64,qint64)),
                                  SLOT(setProgress(qint64,qint64))
           );

    connect( m_downloader.data(), SIGNAL(downloadCompleted(bool)),
                                  SLOT(downloadFinished(bool))
           );

//    qDebug() << "GrooveOff ::" << "Started download of" << m_playlistItem->song()->songName();
}

/*!
  \brief downloadFinished: slot called once cover download finished
  \param ok: status of cover download
  \return void
*/
void DownloadItem::downloadFinished( bool ok )
{
    //...and show others if download was successful
    if( ok ) {
        m_downloadState = GrooveOff::FinishedState;
//        qDebug() << "GrooveOff ::" << "Finished downloading" << m_playlistItem->song()->songName();
        emit reloadPlaylist();
    } else {
        removeEmptyFolder( QFileInfo( m_playlistItem->path() + m_playlistItem->fileName() ).absoluteDir() );
        m_downloadState = GrooveOff::ErrorState;
        qDebug() << "GrooveOff :: Error downloading" << m_playlistItem->song()->songName() << "::" << m_downloader->errorString();
    }

    stateChanged();

    emit downloadFinished();
    
    if( The::mainWindow() )
        The::mainWindow()->reloadItemsDownloadButtons();
}

/*!
  \brief setProgress: update progressbar
  \param bytesReceived: number of bytes received
  \param bytesTotal: number of total bytes
  \return void
*/
void DownloadItem::setProgress( const qint64 &bytesReceived, const qint64 &bytesTotal )
{
    if( m_oneShot ) {
        if( The::mainWindow() )
            The::mainWindow()->reloadItemsDownloadButtons();
        m_oneShot = false;
    }
    // enable play button if downloaded at least 1MiB
    if( bytesReceived > 1024*1024 )
        ui->playButton->setButtonEnabled( true );

    ui->progressBar->setValue( bytesReceived );
    ui->progressBar->setMaximum( bytesTotal );
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

void DownloadItem::queueItem()
{
    m_downloadState = GrooveOff::QueuedState;
    stateChanged();
    emit addToQueue( this );
}

void DownloadItem::deleteItem()
{
    if( m_context == GrooveOff::Download ) {
        //check if file exists before starting countdown
        if( !QFile::exists( songFile() ) ) {
            qDebug() << "GrooveOff ::"  << songFile() << "not found";
            emit removeMe( this );
            emit reloadPlaylist();
            return;
        }
    }
    
    ui->timerWidget->setVisible( true );
    ui->timerButton->setToolTip( trUtf8( "Abort deletion" ) );
    ui->timerButton->startCountdown();
    ui->deleteWidget->setVisible( false );
}

void DownloadItem::stopDownload()
{
    m_downloader->stopDownload();
    ui->progressBar->setValue( 0 );
    m_downloadState = GrooveOff::AbortedState;
    stateChanged();
}

void DownloadItem::unqueueItem()
{
    m_downloadState = GrooveOff::AbortedState;
    stateChanged();
}


bool DownloadItem::operator==( DownloadItem& right ) const
{
    if( m_playlistItem->song()->songID() == right.playlistItem()->song()->songID() )
        return true;
    return false;
}


/*!
  \brief removeSong: remove a song
  \return void
*/
void DownloadItem::removeSong()
{
    if( m_context == GrooveOff::Track ) {
        emit removeMeFromSession( m_playlistItem->song()->songID() );
        return;
    }

    The::audioEngine()->removingTrack( m_playlistItem );
    The::playlist()->removeItem( m_playlistItem );

    if( QFile::exists( songFile() ) ) {
        if( !QFile::remove( songFile() ) )
            qDebug() << "GrooveOff ::"  << "Error removing" << songFile();
    } else {
        qDebug() << "GrooveOff ::"  << songFile() << "not found";
    }

    removeEmptyFolder( QFileInfo( m_playlistItem->path() + m_playlistItem->fileName() ).absoluteDir() );

    emit removeMe( this );
    
    if( The::mainWindow() )
        The::mainWindow()->reloadItemsDownloadButtons();
    
    emit reloadPlaylist();
}

/*!
  \brief songFile: return absolute song file name
  \return string
*/
QString DownloadItem::songFile()
{
    return m_playlistItem->path() + m_playlistItem->fileName();
}


/*!
  \brief enterEvent: manage mouse movement
  \return void
*/
void DownloadItem::enterEvent( QEvent* event )
{
    switch( m_downloadState ) {
        case GrooveOff::FinishedState:
            ui->timerWidget->setVisible( ui->timerButton->isCountdownStarted() );
            ui->deleteWidget->setVisible( !ui->timerButton->isCountdownStarted() );
            if( m_context == GrooveOff::Download )
                ui->openFolderWidget->setVisible( true );
            break;
        case GrooveOff::QueuedState:
            ui->infoMessageWidget->setVisible( true );
            ui->unqueueWidget->setVisible( true );
            break;
        case GrooveOff::AbortedState:
        case GrooveOff::ErrorState:
            ui->infoMessageWidget->setVisible( true );
            ui->queueWidget->setVisible( true );
            break;
        default:
            break;
    }

    QWidget::enterEvent( event );
}

/*!
  \brief leaveEvent: manage mouse movement
  \return void
*/
void DownloadItem::leaveEvent( QEvent* event )
{
    switch( m_downloadState ) {
        case GrooveOff::FinishedState:
            ui->timerWidget->setVisible( ui->timerButton->isCountdownStarted() );
            ui->deleteWidget->setVisible( false );
            if( m_context == GrooveOff::Download )
                ui->openFolderWidget->setVisible( false );
            break;
        case GrooveOff::QueuedState:
            ui->infoMessageWidget->setVisible( false );
            ui->unqueueWidget->setVisible( false );
            break;
        case GrooveOff::AbortedState:
        case GrooveOff::ErrorState:
            ui->infoMessageWidget->setVisible( false );
            ui->queueWidget->setVisible( false );
            break;
        default:
            //do nothing
            break;
    }

    QWidget::leaveEvent( event );
}

/*!
  \brief setPlayerState: this function sets the state of the player
         for icon synchronization
  \return void
*/
void DownloadItem::setPlayerState( Phonon::State state )
{
    m_playerState = state;
    stateChanged();
}

/*!
  \brief openFolder: open folder containing downloaded song
  \return void
*/
void DownloadItem::openFolder()
{
    QFileInfo fileInfo( m_playlistItem->path() + m_playlistItem->fileName() );
    QDesktopServices::openUrl( QUrl( QLatin1String( "file://" ) + fileInfo.absolutePath(), QUrl::TolerantMode ) );
}

void DownloadItem::loadCover()
{
    if( !m_playlistItem->song()->coverArtFilename().isEmpty()
        && QFile::exists( Utility::coversCachePath + m_playlistItem->song()->coverArtFilename() )
        && m_playlistItem->song()->coverArtFilename() != "0" )
        ui->coverLabel->setPixmap( QPixmap( Utility::coversCachePath + m_playlistItem->song()->coverArtFilename() ) );
    else
        ui->coverLabel->setPixmap( QIcon::fromTheme( QLatin1String( "media-optical" ),
                                                     QIcon( QLatin1String( ":/resources/media-optical.png" ) )
                                                   ).pixmap( Utility::coverSize ) );
}

void DownloadItem::abortDownload()
{
    if( m_downloadState == GrooveOff::DownloadingState ) {
        m_downloader->stopDownload();
        ui->progressBar->setValue( 0 );
    }

    m_downloadState = GrooveOff::AbortedState;
    stateChanged();

    // FIXME: seems that file remove is slower than this call
    // and the icon is updated as expected
    if( The::mainWindow() )
        The::mainWindow()->reloadItemsDownloadButtons();

    removeEmptyFolder( QFileInfo( m_playlistItem->path() + m_playlistItem->fileName() ).absoluteDir() );
}

void DownloadItem::removeEmptyFolder( QDir folder )
{
    if( !folder.exists() )
        return;
    
#if QT_VERSION >= QT_VERSION_CHECK( 5, 0, 0 )
    if( folder.path() == QStandardPaths::writableLocation( QStandardPaths::MusicLocation ) ||
       folder.path() == Utility::destinationPath
    )
        return;
#else
    if( folder.path() == QDesktopServices::storageLocation( QDesktopServices::MusicLocation ) ||
       folder.path() == Utility::destinationPath
    )
        return;
#endif

    if( folder.entryList( QDir::NoDotAndDotDot ).count() == 0 ) {
        folder.rmdir( folder.path() );
        if( folder.cdUp() )
            removeEmptyFolder( folder );
    }
}
