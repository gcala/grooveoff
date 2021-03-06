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


#include "MainWindow.h"
#include "ui_MainWindow.h"
#include "PlayerWidget.h"
#include "DownloadItem.h"
#include "Roles.h"
#include "ConfigDialog.h"
#include "AboutDialog.h"
#include "Utility.h"
#include "GrooveOffNamespace.h"
#include "CoverManager.h"
#include "MatchItem.h"
#include "PlaylistItem.h"
#include "AudioEngine.h"
#include "ActionCollection.h"
#include "PaletteHandler.h"
#include "SvgHandler.h"
#include "Playlist.h"
#include "widgets/Spinner.h"
#include "SessionManager.h"
#include "SessionReaderWriter.h"

#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QAction>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QSettings>
#include <QSplitter>
#include <QListView>
#include <QListWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QDesktopServices>
#include <QInputDialog>

#if QT_VERSION >= QT_VERSION_CHECK( 5, 0, 0 )
#include <QStandardPaths>
#endif

#include <QTime>
#include <QFileDialog>
#include <QDir>
#include <QFile>
#if QT_VERSION < QT_VERSION_CHECK( 5, 0, 0 )
#include <qjson/serializer.h>
#include <qjson/parser.h>
#endif
#include <QMessageBox>
#include <QApplication>
#include <QCompleter>
#include <QDirModel>
#include <QPointer>

using namespace GrooveShark;

// version include
#include <../config-version.h>

Q_DECLARE_METATYPE( QList<int> )

QPointer<MainWindow> MainWindow::s_instance;

namespace The {
    MainWindow* mainWindow() {
        return MainWindow::s_instance.data();
    }
}

/*!
  \brief MainWindow: this is the MainWindow constructor.
  \param parent: The Parent Widget
*/
MainWindow::MainWindow( QWidget *parent )
    : QMainWindow( parent )
    , ui( new Ui::MainWindow )
    , m_playerWidget( new PlayerWidget( this ) )
    , m_coverManager( new CoverManager( this ) )
    , m_qncm( new QNetworkConfigurationManager( this ) )
{
    qRegisterMetaType< PlaylistItemPtr >( "PlaylistItemPtr" );
    qRegisterMetaTypeStreamOperators< PlaylistItemPtr >( "PlaylistItemPtr" );

    ui->setupUi( this );
    s_instance = this;

    setupMenus();
    setupUi();
    setupConnections();
    loadSettings();
    loadBootSettings();
    statusBar()->addPermanentWidget( m_playerWidget, 1 );
    statusBar()->setSizeGripEnabled( false );

    m_spinner = new Spinner( ui->spinnerWidget );
    m_spinner->setText( tr( "" ) );
    m_spinner->setType( Spinner::Sun );
    
    setupPaths();

    m_api = ApiRequest::instance();

    setWindowTitle( QString::fromLatin1( "GrooveOff %1" ).arg( GROOVEOFF_VERSION ) );

    setupGuiLayout();

    initRandomNumberGenerator();
    
    if( m_qncm->isOnline() )
        getToken();

    m_parallelDownloadsCount = 0;

    initPathLine();

    if( m_saveSession )
        loadSession();

    // FIXME
    // loadSessions();

    m_mpris = new Mpris( this );
}

MainWindow::~MainWindow()
{
    delete The::paletteHandler();
    delete The::svgHandler();

    m_spinner->stop();

    if( m_saveSession )
        saveSession();
    
    if( m_emptyCache ) {
        QDir coversPath( Utility::coversCachePath );
        QStringList covers = coversPath.entryList( QStringList() << "*.jpg" << "*.png", QDir::Files, QDir::Name );

        foreach( const QString &cover, covers ) {
            QFile::remove( Utility::coversCachePath + QDir::separator() + cover );
        }
    }

    saveSettings();

    delete m_api;
    delete m_mpris;
}

void MainWindow::setupUi()
{
    ui->logoLabel->setPixmap( QPixmap( QLatin1String( ":/resources/grooveoff.png" ) ) );
    ui->logoLabel->setMinimumHeight( 30 );
    ui->logoLabel->setMaximumWidth( 30 );
    ui->logoLabel->setScaledContents( 30 );

    ui->appLabel->setText( trUtf8( "GrooveOff" ) );
    QFontMetrics fmTitle( Utility::font( QFont::Bold, 2 ) );
    ui->appLabel->setFont( Utility::font( QFont::Bold, 2 ) );
    ui->subtitleLabel->setText( trUtf8( "Offline Grooveshark.com music" ) );

    m_playerWidget->showMessage( trUtf8( "Connecting..." ) );

    QFontMetrics fmSystemFont( Utility::font( QFont::Bold ) );
    int fontHeight = fmSystemFont.height();

    ui->label3->setText( trUtf8( "Search:" ) );
    ui->label3->setBuddy( ui->searchLine );
    ui->searchLine->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed );
    ui->searchLine->setFixedHeight( fontHeight > 24 ? fontHeight : 24 );
    ui->searchLine->setToolTip( trUtf8( "Search for songs, artists, genres, playlists" ) );
    ui->searchLine->setPlaceholderText( trUtf8( "Search for songs, artists, genres, playlists" ) );
    
    ui->searchButton->setType( IconButton::Search );
    ui->searchButton->setButtonEnabled( false );
    ui->searchButton->setToolTip( trUtf8( "Start search" ) );
    
    ui->compactMenuButton->setType( IconButton::Settings );
    ui->compactMenuButton->setToolTip( trUtf8( "Main menu" ) );
    ui->compactMenuButton->setMenu( m_compactMainMenu );

    ui->label4->setText( trUtf8( "Save in:" ) );
    ui->label4->setBuddy( ui->pathLine );

    ui->pathLine->setToolTip( trUtf8( "Current save folder" ) );
    ui->pathLine->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed );
    ui->pathLine->setFixedHeight( fontHeight > 24 ? fontHeight : 24 );

    // Completer used to save search history
    QCompleter *completer = new QCompleter( this );
    completer->setModel( new QDirModel( completer ) );
    ui->pathLine->setCompleter( completer );

    ui->browseButton->setType( IconButton::Browse );
    ui->browseButton->setToolTip( trUtf8( "Select save folder" ) );

    ui->spinnerWidget->setFixedHeight( fontHeight > 25 ? fontHeight : 25 );
    ui->spinnerWidget->setVisible( false );

    ui->combosContainer->setVisible( false );

    ui->batchDownloadButton->setType( IconButton::Batch );
    ui->batchDownloadButton->setToolTip( trUtf8( "Download all tracks" ) );

    ui->matchesMessage->setFont( Utility::font( QFont::Bold ) );

    ui->splitter->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Expanding );

    m_playerWidget->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Fixed );
    
    ui->svgLed->setType( IconButton::Offline );
}

void MainWindow::setupGuiLayout()
{
    if( m_guiLayout == Mini ) {
        The::actionCollection()->getAction( QLatin1String( "miniPlayer" ) )->setChecked( true );
        m_playerWidget->showElapsedTimerLabel( true );
    } else if( ui->splitter->orientation() == Qt::Vertical ) {
        The::actionCollection()->getAction( QLatin1String( "actionCompact" ) )->setChecked( true );
        m_playerWidget->showElapsedTimerLabel( false );
    } else {
        The::actionCollection()->getAction( QLatin1String( "actionWide" ) )->setChecked( true );
        m_playerWidget->showElapsedTimerLabel( true );
    }
}

void MainWindow::setupPaths()
{
#if QT_VERSION >= QT_VERSION_CHECK( 5, 0, 0 )
    if( !QFile::exists( QStandardPaths::writableLocation( QStandardPaths::CacheLocation ) ) ) {
        QDir dir;
        dir.mkpath( QStandardPaths::writableLocation( QStandardPaths::CacheLocation ) );
    }

    Utility::coversCachePath = QStandardPaths::writableLocation( QStandardPaths::CacheLocation ) + QDir::separator();
    m_sessionFilePath = QStandardPaths::writableLocation( QStandardPaths::DataLocation ) + QDir::separator();
#else
    if( !QFile::exists( QDesktopServices::storageLocation( QDesktopServices::CacheLocation ) ) ) {
        QDir dir;
        dir.mkpath( QDesktopServices::storageLocation( QDesktopServices::CacheLocation ) );
    }

    Utility::coversCachePath = QDesktopServices::storageLocation( QDesktopServices::CacheLocation ) + QDir::separator();
    m_sessionFilePath = QDesktopServices::storageLocation( QDesktopServices::DataLocation ).remove( QLatin1String( "/data" ) ) + QDir::separator();
#endif
}

void MainWindow::initRandomNumberGenerator()
{
    QTime time = QTime::currentTime();
    qsrand( ( uint )time.msec() );
}

void MainWindow::initPathLine()
{
    QSettings settings;
    settings.setIniCodec( "UTF-8" );

#if QT_VERSION >= QT_VERSION_CHECK( 5, 0, 0 )
    if( m_saveDestination ) {
        ui->pathLine->setText( settings.value( QLatin1String( "destination" ),
                                               QStandardPaths::writableLocation( QStandardPaths::MusicLocation ) 
                                             ).toString() );
        if( ui->pathLine->text().isEmpty() )
            ui->pathLine->setText( QStandardPaths::writableLocation( QStandardPaths::MusicLocation ) );
    } else {
        ui->pathLine->setText( QStandardPaths::writableLocation( QStandardPaths::MusicLocation ) );
    }
#else
    if( m_saveDestination ) {
        ui->pathLine->setText( settings.value( QLatin1String( "destination" ),
                                               QDesktopServices::storageLocation( QDesktopServices::MusicLocation ) 
                                             ).toString() );
        if( ui->pathLine->text().isEmpty() )
            ui->pathLine->setText( QDesktopServices::storageLocation( QDesktopServices::MusicLocation ) );
    } else {
        ui->pathLine->setText( QDesktopServices::storageLocation( QDesktopServices::MusicLocation ) );
    }
#endif
}

void MainWindow::setupMenus()
{
    // Always create a menubar, but only create a compactMenu on Windows and X11
    m_menuBar = The::actionCollection()->createMenuBar( this );
    setMenuBar( m_menuBar );
    m_compactMainMenu = The::actionCollection()->createCompactMenu( this );
}

void MainWindow::setupConnections()
{
    // <Menu Items>
    ActionCollection *ac = The::actionCollection();

    connect( ac->getAction( "actionClose" ), SIGNAL(triggered()), SLOT(close()) );
    connect( ac->getAction( "actionDonate" ), SIGNAL(triggered()), SLOT(donate()) );
    connect( ac->getAction( "actionConfigure" ), SIGNAL(triggered()), SLOT(configure()) );
    connect( ac->getAction( "actionCompact" ), SIGNAL(triggered()), SLOT(setCompactLayout()) );
    connect( ac->getAction( "actionWide" ), SIGNAL(triggered()), SLOT(setWideLayout()) );
    connect( ac->getAction( "miniPlayer" ), SIGNAL(triggered()), SLOT(setMiniPlayerLayout()) );
    connect( ac->getAction( "actionNewToken" ), SIGNAL(triggered()), SLOT(getToken()) );
    connect( ac->getAction( "actionStopDownloads" ), SIGNAL(triggered(bool)), ui->downloadList, SLOT(abortAllDownloads()) );
    connect( ac->getAction( "actionRemoveFailed" ), SIGNAL(triggered(bool)), ui->downloadList, SLOT(removeFailedAborted()) );
    connect( ac->getAction( "actionClearDownloadList" ), SIGNAL(triggered(bool)), ui->downloadList, SLOT(removeDownloaded()) );
    connect( ac->getAction( "actionAbout" ), SIGNAL(triggered()), SLOT(about()) );

//     connect( ac->getAction( "actionSaveSessionAs" ), SIGNAL(triggered()),
//                                                     SLOT(saveSessionAs()) );
// 
//     connect( ac->getAction( "actionManageSessions" ), SIGNAL(triggered()),
//                                                      SLOT(openSessionManager()) );

    connect( ac->getAction( "actionQtAbout" ), SIGNAL(triggered()), qApp, SLOT(aboutQt()) );

    // gui widgets
    connect( ui->searchButton, SIGNAL(buttonClicked()), SLOT(beginSearch()) );
    connect( ui->browseButton, SIGNAL(buttonClicked()), SLOT(selectFolder()) );
    connect( ui->searchLine, SIGNAL(returnPressed()), SLOT(beginSearch()) );
    connect( ui->artistsCB, SIGNAL(activated(int)), SLOT(artistChanged()) );
    connect( ui->albumsCB, SIGNAL(activated(int)), SLOT(albumChanged()) );
    connect( ui->batchDownloadButton, SIGNAL(buttonClicked()), SLOT(batchDownload()) );
    connect( ui->pathLine, SIGNAL(textChanged(QString)), SLOT(changeDestinationPath()) );

    // network
    connect( m_qncm, SIGNAL(onlineStateChanged(bool)), SLOT(onlineStateChanged(bool)) );
}

void MainWindow::loadSettings()
{
    QSettings settings;
    settings.setIniCodec( "UTF-8" );

    m_saveSession     = settings.value( QLatin1String( "saveSession" ), true ).toBool();
    if( m_saveSession )
        m_saveAborted = settings.value( QLatin1String( "saveAborted" ), false ).toBool();
    else
        m_saveAborted = true;
    m_showHistory     = settings.value( QLatin1String( "saveSearches" ), false ).toBool();
    if( m_showHistory ) {
        m_searchSize = settings.value( QLatin1String( "historySize" ), 5 ).toInt();
        if(m_searchList.isEmpty())
            m_searchList << settings.value( QLatin1String( "searchTerms" ), QStringList() ).toStringList();
        setupCompleter();
    }
    m_maxResults      = settings.value( QLatin1String( "numResults" ), 0 ).toInt();
    m_loadCovers      = settings.value( QLatin1String( "loadCovers" ), true ).toBool();
    if( m_loadCovers )
        m_emptyCache = settings.value( QLatin1String( "emptyCache" ), false ).toBool();
    else
        m_emptyCache = true;
    m_maxDownloads    = settings.value( QLatin1String( "maxDownloads" ), 5 ).toInt();
    m_saveDestination = settings.value( QLatin1String( "saveDestination" ), true ).toBool();

    //Naming Schema
    Utility::namingSchema = settings.value( QLatin1String( "namingSchema" ),
                                            QLatin1String( "%artist/%album/%track - %title" ) 
                                          ).toString();
                                          
    setGeometry( settings.value( QLatin1String( "windowGeometry" ), QRect( 100, 100, 350, 600 ) ).toRect() );
}

void MainWindow::loadBootSettings()
{
    QSettings settings;
    settings.setIniCodec( "UTF-8" );
    
    m_guiLayout       = ( GuiLayout )settings.value( QLatin1String( "guiLayout" ), Compact ).toInt();
    
    m_sessionFileName = settings.value( QLatin1String( "sessionFile" ), QLatin1String( "default" ) ).toString();
    
    ui->splitter->setOrientation( (Qt::Orientation )settings.value( QLatin1String( "splitterOrientation" ), Qt::Vertical ).toInt() );
    m_playerWidget->showElapsedTimerLabel( ui->splitter->orientation() == Qt::Vertical ? false : true );
    
    const QList<int> &_sizes = settings.value( QLatin1String( "splitterSizes" ) ).value< QList<int> >();

    if( !_sizes.isEmpty() ) {
        ui->splitter->setSizes( _sizes );
    }

    The::audioEngine()->setVolume( settings.value( QLatin1String( "volume" ), 50 ).toInt() );
    The::audioEngine()->setMuted( settings.value( QLatin1String( "muted" ), false ).toBool() );

    m_playerWidget->setTimerState( (GrooveOff::TimerState )settings.value( QLatin1String( "timerState" ), GrooveOff::ElapsedState ).toInt() );
    
    if( m_guiLayout == Mini )
        setMiniPlayerLayout();
    else
        ui->compactMenuButton->setVisible( false );
}

void MainWindow::saveSettings()
{
    QSettings settings;
    settings.setIniCodec( "UTF-8" );

    // Do not overwrite splittersSizes/windowGeometry if mini layout
    if( m_guiLayout != Mini ) {
        settings.setValue( QLatin1String( "splitterSizes" ), QVariant::fromValue< QList<int> >( ui->splitter->sizes() ) );
        settings.setValue( QLatin1String( "windowGeometry" ), geometry() );
    } else {
        // if mini layout, save only x,y,width
        QRect rect = settings.value( QLatin1String( "windowGeometry" ), QRect( 100,100,350,600 ) ).toRect();
        rect.setWidth( geometry().width() );
        settings.setValue( QLatin1String( "windowGeometry" ), rect );
    }

    if( !m_showHistory )
        settings.setValue( QLatin1String( "searchTerms" ), QStringList() );
    else
        settings.setValue( QLatin1String( "searchTerms" ), m_searchList );

    if( m_saveDestination )
        settings.setValue( QLatin1String( "destination" ), ui->pathLine->text() );
    else
        settings.setValue( QLatin1String( "destination" ), QString() );

    settings.setValue( QLatin1String( "timerState" ), m_playerWidget->getTimerState() );
    settings.setValue( QLatin1String( "muted" ), The::audioEngine()->isMuted() );
    settings.setValue( QLatin1String( "volume" ), The::audioEngine()->volume() );
    settings.setValue( QLatin1String( "guiLayout" ), m_guiLayout );
    settings.setValue( QLatin1String( "sessionFile" ), m_sessionFileName );
}

void MainWindow::selectFolder()
{
    QString dir = QFileDialog::getExistingDirectory( this,
                                                     trUtf8( "Select Directory" ),
                                                     ui->pathLine->text().isEmpty() ? QDir::homePath() : ui->pathLine->text(),
                                                     QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks );

    // be sure that a valid path was selected
    if( QFile::exists( dir ) )
        ui->pathLine->setText( dir );

    changeDestinationPath();
}

void MainWindow::changeDestinationPath()
{
    Utility::destinationPath = ui->pathLine->text();
    reloadItemsDownloadButtons();
}

void MainWindow::beginSearch()
{
    // stop a search if one operation is still in progress
    if( m_searchInProgress )
        return;

    // abort search if search field is empty
    if( ui->searchLine->text().isEmpty() )
        return;

    // if save searches history append last search and remove oldest if history is full
    if( m_showHistory ) {
        if( m_searchList.count() >= m_searchSize )
            m_searchList.removeFirst();

        m_searchList.append( ui->searchLine->text().toLower() );
        m_searchList.removeDuplicates();

        setupCompleter();
    }

    // flag that prevents multiple searches
    m_searchInProgress = true;

    // some ui setups
    ui->searchButton->setVisible( false );

    ui->spinnerWidget->setVisible( true );
    m_spinner->start( 0 );

    // clear cover manager
    m_coverManager->clear();

    // clear listWidget
    for( int i = ui->matchList->count() - 1; i >= 0; i-- ) {
        QListWidgetItem *item = ui->matchList->takeItem( i );
        ui->downloadList->removeItemWidget( item );
        delete item;
    }

    // clear artists, albums containers
    ui->artistsCB->clear();
    ui->artistsCB->addItem( trUtf8( "All Artists" ) );
    ui->albumsCB->clear();
    ui->albumsCB->addItem( trUtf8( "All Albums" ) );
    m_artistsAlbumsContainer.clear();
    
    ui->matchesMessage->setText( trUtf8( "Querying..." ) );

    m_songList = m_api->songs( ui->searchLine->text(), Utility::token );

    connect( m_songList.data(), SIGNAL(finished()),  SLOT(searchFinished()) );
    connect( m_songList.data(), SIGNAL(parseError()), SLOT(searchError()) );
    connect( m_songList.data(), SIGNAL(requestError(QNetworkReply::NetworkError)), SLOT(searchError()) );

    // force downloadList repaint to prevent blank items
    ui->downloadList->repaint();
}

void MainWindow::getToken()
{
    ui->searchLine->setStyleSheet( QLatin1String( "background-color:#E7A2A9;" ) );

    // some ui setups
    ui->svgLed->setType( IconButton::Offline );
    m_playerWidget->showMessage( trUtf8( "Connecting..." ) );

    m_token = m_api->token();

    connect( m_token.data(), SIGNAL(finished()), SLOT(tokenRequestFinished()) );
    connect( m_token.data(), SIGNAL(parseError()), SLOT(tokenRequestError()) );
    connect( m_token.data(), SIGNAL(requestError(QNetworkReply::NetworkError)), SLOT(tokenRequestError()) );
}

void MainWindow::tokenRequestFinished()
{
    // the application is now free to perform a search
    m_searchInProgress = false;
    ui->searchButton->setButtonEnabled( true );

    if( !m_token->isEmpty() ) {
        ui->searchLine->setStyleSheet( QLatin1String( "" ) );

        Utility::token = m_token->content();

        m_playerWidget->showMessage( trUtf8( "Connected to\nGrooveshark" ) );
        ui->svgLed->setToolTip( trUtf8( "You're connected to Grooveshark!" ) );
        ui->svgLed->setType( IconButton::Online );
        ui->matchList->setEnabled( true );

        // start download of queued songs from last session
        unqueue();
    } else {
        m_playerWidget->showMessage( trUtf8( "Connection error!!" ) );
        qDebug() << "GrooveOff ::" << "Token not received!!";
        Utility::token.clear();
    }
}

void MainWindow::tokenRequestError()
{
    m_playerWidget->showMessage( trUtf8( "Connection error!!" ) );
    qDebug() << "GrooveOff ::" << m_token->errorString();
}

void MainWindow::searchFinished()
{
    restoreSearch();

    // check if last search returned results
    if( m_songList->count() == 0 ) {
        qDebug() << "GrooveOff ::" << "Empty result list";
        ui->matchesMessage->setText( trUtf8( "%n song(s) found", "", 0 ) );
        return;
    }

    // 'elementsNumber' contains number of elements to display
    const int elementsNumber = numberOfElementsToShow( m_songList->count() );

    ui->combosContainer->setVisible( true );

    QStringList artists;
    QStringList albums;

    ui->matchesMessage->setText( trUtf8( "%n song(s) found", "", elementsNumber ) );

    for( int i = 0; i < elementsNumber; i++ ) {
        PlaylistItemPtr playlistItem( new PlaylistItem( m_songList->item( i ) ) );

        // Decide if show cover arts
        if( m_loadCovers && !QFile::exists( Utility::coversCachePath + m_songList->item( i )->coverArtFilename() ) ) {
            m_coverManager->appendItem( playlistItem );
        }

        // build a MathItem with all required data
        MatchItem *matchItem = new MatchItem( playlistItem, this );
        QListWidgetItem *wItem = new QListWidgetItem;
        ui->matchList->addItem( wItem );
        ui->matchList->setItemWidget( wItem, matchItem );
        wItem->setSizeHint( QSize( Utility::coverSize + Utility::marginSize * 2,Utility::coverSize + Utility::marginSize * 2 ) );
        
        connect( matchItem, SIGNAL(download(PlaylistItemPtr)), SLOT(downloadPlaylistItem(PlaylistItemPtr)) );

        // don't freeze gui inserting items
        QCoreApplication::processEvents();
        
        appendArtistAlbum( m_songList->item( i ) );
        
        if( !m_songList->item( i )->artistName().isEmpty() )
                artists << m_songList->item( i )->artistName();

        if( !m_songList->item( i )->albumName().isEmpty() )
            albums << m_songList->item( i )->albumName();
    }

    // removing duplicates
    artists.removeDuplicates();
    albums.removeDuplicates();

    // sorting items
    artists.sort();
    albums.sort();

    ui->artistsCB->addItems( artists );
    ui->albumsCB->addItems( albums );
    applyFilter();

    // force downloadList repaint to prevent blank items
    ui->downloadList->repaint();
}

void MainWindow::appendArtistAlbum(const SongPtr& song)
{
    if( m_artistsAlbumsContainer.keys().contains( song->artistName() ) ) {
        QHash<QString, QStringList>::iterator iter = m_artistsAlbumsContainer.find( song->artistName() );
        if( !iter.value().contains( song->albumName() ) )
            iter.value() << song->albumName();
    } else {
        m_artistsAlbumsContainer[ song->artistName() ] = QStringList() << song->albumName();
    }
}


int MainWindow::numberOfElementsToShow(int searchSize)
{
    if( m_maxResults == 0 ) // no results limit
        return searchSize;
    return qMin( m_maxResults, searchSize );
}


void MainWindow::searchError()
{
    qDebug() << "GrooveOff ::" << m_songList->errorString();
    restoreSearch();
}

void MainWindow::downloadPlaylistItem( PlaylistItemPtr playlistItem )
{
    // check if item is already in queue
    if( isItemQueued( playlistItem->song()->songID() ) ) {
        if( !m_batchDownload ) {
            QMessageBox::information( this, trUtf8( "Attention" ), trUtf8( "This song is already in queue." ), QMessageBox::Ok );
        }
        return;
    }
    
    playlistItem->setPath( ui->pathLine->text() + QDir::separator() );
    playlistItem->setNamingSchema( Utility::namingSchema );

    // check if destination folder exists
    if( !QDir( ui->pathLine->text() ).exists() ) {
        QMessageBox::information( this, trUtf8( "Attention" ), trUtf8( "The destination folder does not exists.\nSelect a valid path" ), QMessageBox::Ok );
        
        // if a batch download is in progress show the message above then stop all downloads.
        if( m_batchDownload )
            m_stopBatchDownload = true;
        
        return;
    }
    
    // check file existence
    if( playlistItem->fileInfo().exists() ) {
        if( m_batchDownload )
            return;
        
        int ret = QMessageBox::question( this,
                                         trUtf8( "Overwrite File?" ),
                                         trUtf8( "A file named \"%1\" already exists. Are you sure you want to overwrite it?" ).arg( playlistItem->fileInfo().fileName() ),
                                         QMessageBox::Yes | QMessageBox::Cancel,
                                         QMessageBox::Cancel );
        if( ret == QMessageBox::Yes ) {
            QFile::remove( playlistItem->fileInfo().absoluteFilePath() );
        } else {
            return;
        }
    }

    // check if destination folder is writable
    if( !QFileInfo( ui->pathLine->text() ).isWritable() ) {
        QMessageBox::information( this, 
                                  trUtf8( "Attention" ),
                                  trUtf8( "The destination folder is not writable.\n"
                                          "Select a valid path" ),
                                  QMessageBox::Ok );
        
        // if a batch download is in progress show the message above then stop all downloads.
        if( m_batchDownload )
            m_stopBatchDownload = true;
        
        return;
    }

    addDownloadItem( playlistItem );
}


void MainWindow::addDownloadItem( const PlaylistItemPtr &playlistItem )
{
    // Creating path, if needed
    if( !playlistItem->fileInfo().absoluteDir().exists() ) {
        if( !playlistItem->fileInfo().absoluteDir().mkpath( playlistItem->fileInfo().absolutePath() ) ) {
            if( !m_batchDownload ) {
                QMessageBox::information( this,
                                          trUtf8( "Attention" ),
                                          trUtf8( "Can't create destination path:" ) + QLatin1String( "\n\n" ) + playlistItem->fileInfo().absolutePath() + QLatin1String( "\n\n" ) + trUtf8( "Aborting..." ),
                                          QMessageBox::Ok );
            }
            return;
        }
    }

    // build a DownloadItem with all required data
    DownloadItem *item = new DownloadItem( playlistItem, this );

    connect( item,             SIGNAL(reloadPlaylist()),
             ui->downloadList, SLOT(reloadPlaylist()) 
           );
    
    connect( item,             SIGNAL(removeMe(DownloadItem*)), 
             ui->downloadList, SLOT(removeItem(DownloadItem*)) 
           );
    
    connect( item, SIGNAL(downloadFinished()), 
                   SLOT(freeDownloadSlot()) 
           );
    
    connect( item, SIGNAL(addToQueue(DownloadItem*)), 
                   SLOT(addItemToQueue(DownloadItem*)) 
           );

    if( !playlistItem->fileInfo().exists() ) {
        // check if download m_queue is full
        if( m_parallelDownloadsCount < m_maxDownloads && !m_token->isEmpty() ) {
            m_parallelDownloadsCount++;
            item->startDownload();
        } else {
            m_queue.append( item );
        }
    }

    QListWidgetItem *wi = new QListWidgetItem;
    ui->downloadList->addItem( wi );
    ui->downloadList->setItemWidget( wi, item );
    ui->downloadList->setCurrentItem( wi );
    wi->setSizeHint( QSize( Utility::coverSize + Utility::marginSize * 2,
                            Utility::coverSize + Utility::marginSize * 2 
                          ) 
                   );
}

void MainWindow::setCompactLayout()
{
    QSettings settings;
    settings.setIniCodec( "UTF-8" );

    // if previous layout was Mini, restore saved splitter size/window geometry
    if( m_guiLayout == Mini ) {
        ui->splitter->setVisible( true );
        ui->searchWidgets->setVisible( true );
        setMaximumHeight( 16777215 );
        ui->splitter->setOrientation( (Qt::Orientation )settings.value( QLatin1String( "splitterOrientation" ), Qt::Vertical ).toInt() );
        QList<int> _sizes = settings.value( QLatin1String( "splitterSizes" ) ).value< QList<int> >();

        if( !_sizes.isEmpty() ) {
            ui->splitter->setSizes( _sizes );
        }

        const QRect &rect = settings.value( QLatin1String( "windowGeometry" ), QRect( 100, 100, 350, 600 ) ).toRect();
        setGeometry( geometry().x(), geometry().y(), geometry().width(), rect.height() );
    }

    ui->splitter->setOrientation( Qt::Vertical );
    m_playerWidget->showElapsedTimerLabel( false );

    // save choice
    settings.setValue( QLatin1String( "splitterOrientation" ), ui->splitter->orientation() );

    m_guiLayout = Compact;
    
    ui->compactMenuButton->setVisible( false );
    menuBar()->setVisible( true );
}

void MainWindow::setWideLayout()
{
    QSettings settings;
    settings.setIniCodec( "UTF-8" );

    // if previous layout was Mini, restore saved splitter size/window geometry
    if( m_guiLayout == Mini ) {
        ui->splitter->setVisible( true );
        ui->searchWidgets->setVisible( true );
        setMaximumHeight( 16777215 );
        ui->splitter->setOrientation( ( Qt::Orientation )settings.value( QLatin1String( "splitterOrientation" ), Qt::Vertical ).toInt() );
        QList<int> _sizes = settings.value( QLatin1String( "splitterSizes" ) ).value< QList<int> >();

        if( !_sizes.isEmpty() ) {
            ui->splitter->setSizes( _sizes );
        }

        const QRect &rect = settings.value( QLatin1String( "windowGeometry" ), QRect( 100,100,350,600 ) ).toRect();
        setGeometry( geometry().x(), geometry().y(), geometry().width(), rect.height() );
    }

    ui->splitter->setOrientation( Qt::Horizontal );
    m_playerWidget->showElapsedTimerLabel( true );

    // save choice
    settings.setValue( QLatin1String( "splitterOrientation" ), ui->splitter->orientation() );
    m_guiLayout = Wide;
    
    ui->compactMenuButton->setVisible( false );
    menuBar()->setVisible( true );
}

void MainWindow::setMiniPlayerLayout()
{
    // if previous layout was Compact/Wide, save gemetry/splitter data
    if( m_guiLayout == Compact || m_guiLayout == Wide ) {
        QSettings settings;
        settings.setIniCodec( "UTF-8" );
        settings.setValue( QLatin1String( "windowGeometry" ), geometry() );
        settings.setValue( QLatin1String( "splitterSizes" ), QVariant::fromValue< QList<int> >( ui->splitter->sizes() ) );
    }

    ui->splitter->setVisible( false );
    ui->searchWidgets->setVisible( false );
    ui->compactMenuButton->setVisible( true );
    menuBar()->setVisible( false );
    m_playerWidget->showElapsedTimerLabel( true );
    setMaximumHeight( 40 );
    m_guiLayout = Mini;
}


void MainWindow::about()
{
    AboutDialog dialog;
    dialog.exec();
}

void MainWindow::configure()
{
    ConfigDialog config;
    config.exec();

    loadSettings();
}

void MainWindow::onlineStateChanged( bool isOnline )
{
    if( isOnline ) { // when returning online get a new token
        getToken();
    } else {
        m_playerWidget->showMessage( trUtf8( "Offline" ) );
        ui->searchButton->setButtonEnabled( false );
        ui->svgLed->setType( IconButton::Offline );
        ui->matchList->setEnabled( false );
    }
}

void MainWindow::setupCompleter()
{
    QCompleter *completer = new QCompleter( m_searchList, this );
    completer->setCaseSensitivity( Qt::CaseInsensitive );
    ui->searchLine->setCompleter( completer );
}

void MainWindow::donate()
{
    QDesktopServices::openUrl( QUrl( QLatin1String( "https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=WJNETV7GLTKDG&item_name=" ) + trUtf8( "Donation to Grooveoff's author" ) ) );
}

void MainWindow::applyFilter()
{
    QString artist = ui->artistsCB->currentText();
    QString album = ui->albumsCB->currentText();

    for( int i = 0; i < ui->matchList->count(); i++ ) {
        QString itemArtist = ( (MatchItem * )ui->matchList->itemWidget( ui->matchList->item( i ) ) )->playlistItem()->song()->artistName();
        QString itemAlbum  = ( (MatchItem * )ui->matchList->itemWidget( ui->matchList->item( i ) ) )->playlistItem()->song()->albumName();

        if( ui->artistsCB->currentIndex() == 0  && ui->albumsCB->currentIndex() == 0 ) {
            ui->matchList->setRowHidden( i, false );
        } else if( ui->artistsCB->currentIndex() == 0 ) {
            if( itemAlbum == album )
                ui->matchList->setRowHidden( i, false );
            else
                ui->matchList->setRowHidden( i, true );
        } else if( ui->albumsCB->currentIndex() == 0 ) {
            if( itemArtist == artist )
                ui->matchList->setRowHidden( i, false );
            else
                ui->matchList->setRowHidden( i, true );
        } else {
            if( itemArtist == artist && itemAlbum == album   )
                ui->matchList->setRowHidden( i, false );
            else
                ui->matchList->setRowHidden( i, true );
        }
    }

    ui->matchesMessage->setText( trUtf8( "%n song(s) found", "", visibleItemsCount() ) );
}

// selecting an artist name in combobox triggers this routine
void MainWindow::artistChanged()
{
    ui->albumsCB->clear();
    ui->albumsCB->addItem( trUtf8( "All Albums" ) );
    ui->albumsCB->addItems( m_artistsAlbumsContainer[ ui->artistsCB->currentText() ] );

    applyFilter();
}

// selecting an album name in combobox triggers this routine
void MainWindow::albumChanged()
{
    applyFilter();
}

// this manages the queue; a download starts only when a download slot is available
void MainWindow::unqueue()
{
    // this loop iterates for each free download slot
    for( ; m_parallelDownloadsCount < m_maxDownloads; m_parallelDownloadsCount++ ) {
        // check if m_queue is empty or item already queued
        while( !m_queue.isEmpty() && ( (DownloadItem * )m_queue.at( 0 ) )->downloadState() != GrooveOff::QueuedState ) {
            // discard item
            m_queue.takeFirst();
        }

        if( m_queue.isEmpty() )
            return;

        // valid item found
        ( (DownloadItem * )m_queue.at( 0 ) )->startDownload();
        m_queue.takeFirst();
    }
}

// adds a new item to the queue
void MainWindow::addItemToQueue( DownloadItem* item )
{
    m_queue.append( item );
    unqueue();
}

// decrease the counter and start a new download
void MainWindow::freeDownloadSlot()
{
    m_parallelDownloadsCount--;
    unqueue();
}

// returns true if download list contains a track with given id
bool MainWindow::isItemQueued( const uint &id )
{
    // check if file is currently downloading
    for( int i = 0; i < ui->downloadList->count(); i++ ) {
        if( id == qobject_cast<DownloadItem *>( ui->downloadList->itemWidget( ui->downloadList->item( i ) ) )->playlistItem()->song()->songID() ) {
            return true;
        }
    }
    return false;
}

// returns the number of visible items
// when tracks are filtered on author/album name, some of them are hidden
// the return value is used on label near artists/albums combobox
int MainWindow::visibleItemsCount()
{
    int count = 0;
    for( int i = 0; i < ui->matchList->count(); i++ ) {
        if( !ui->matchList->isRowHidden( i ) )
            count++;
    }

    return count;
}

// each time a song is downloaded or removed this functions
// calls an update for download button icon
void MainWindow::reloadItemsDownloadButtons()
{
    for( int i = 0; i < ui->matchList->count(); i++ ) {
        qobject_cast<MatchItem *>( ui->matchList->itemWidget( ui->matchList->item( i ) ) )->setDownloadIcon();
    }
}

// this is called when clicking batch button ("download all" button)
// this simply adds each filtered item to the queue
// in batch mode all warning are suppressed, so if, f.i., a song already exists it is skipped,
// without confirmation, like in normal mode
void MainWindow::batchDownload()
{
    m_batchDownload = true;
    m_stopBatchDownload = false;
    for( int i = 0; i < ui->matchList->count(); i++ ) {
        if( m_stopBatchDownload )
            break;
        if( !ui->matchList->item( i )->isHidden() )
            downloadPlaylistItem( (( MatchItem * )ui->matchList->itemWidget( ui->matchList->item( i ) ) )->playlistItem() );
    }
    m_batchDownload = false;
    m_stopBatchDownload = true;
}

void MainWindow::saveSessionAs()
{
//     bool ok;
//     QString fileName = QInputDialog::getText( this, tr( "New Session" ),
//                                          trUtf8( "Session name:" ), QLineEdit::Normal,
//                                          QDir::home().dirName(), &ok );
//     // remove separators, if any
//     fileName.remove( '/' );
//     fileName.remove( '\\' );
// 
//     if ( ok && !fileName.isEmpty() ) {
//         // save current session
//         saveSession();
//         // change session name
//         m_sessionFileName = fileName;
//     }
}

// save the current download list in current session file name
void MainWindow::saveSession()
{
    if( !m_saveAborted ) {
        ui->downloadList->removeFailedAborted();
    }
    
    The::sessionReaderWriter()->write( m_sessionFilePath + m_sessionFileName + QLatin1String( ".xml" ),
                                       ui->downloadList->playlistItems() );
}

// loads a session file
void MainWindow::loadSession()
{
    // disable temporarily AutoScroll
    ui->downloadList->setAutoScroll( false );

    // remove old entries
    ui->downloadList->abortAllDownloads();
    ui->downloadList->removeDownloaded();
    ui->downloadList->removeFailedAborted();

    QList<PlaylistItemPtr> items = The::sessionReaderWriter()->read( m_sessionFilePath + m_sessionFileName + QLatin1String( ".xml" ) );

    foreach( PlaylistItemPtr item, items ) {
        // Decide if show cover arts
        if( m_loadCovers && !QFile::exists( Utility::coversCachePath + item->song()->coverArtFilename() ) ) {
            m_coverManager->appendItem( item );
        }
        
        addDownloadItem( item );
    }

    ui->downloadList->setAutoScroll( true );
    ui->downloadList->reloadPlaylist();

    // FIXME
    // loadSessions();
}

// populates File menu with all sessions found on disk
// each file will have a menu entry and a qaction
void MainWindow::loadSessions()
{
    foreach( QAction *action, The::actionCollection()->getMenu( "sessionsMenu" )->actions() ) {
        The::actionCollection()->getMenu( "sessionsMenu" )->removeAction( action );
        delete action;
    }

    QDir sessionPath( m_sessionFilePath );
    QStringList sessions = sessionPath.entryList( QStringList() << "*.xml", QDir::Files, QDir::Name );

    foreach( QString session, sessions ) {
        QAction *action = new QAction( session.remove( ".xml", Qt::CaseInsensitive ), The::actionCollection()->getMenu( "sessionsMenu" ) );
        if( session == m_sessionFileName )
            action->setFont( Utility::font( QFont::Bold ) );
        The::actionCollection()->getMenu( "sessionsMenu" )->addAction( action );
        
        connect( action, SIGNAL(triggered()),
                        SLOT(loadSessionFile())
               );
    }
}

// this function is called when clicking a session entry in File menu
void MainWindow::loadSessionFile()
{
    QAction *action = ( QAction * )QObject::sender();
    QString fileName = action->text();
    if( fileName != m_sessionFileName && QFile::exists( m_sessionFilePath + fileName + QLatin1String( ".xml" ) ) ) {
        //FIXME ask if abort current downloads
        saveSession();
        ui->downloadList->abortAllDownloads();
        ui->downloadList->removeDownloaded();
        ui->downloadList->removeFailedAborted();
        m_sessionFileName = fileName;

        loadSession();
    }
}

// resets status of some widgets
void MainWindow::restoreSearch()
{
    m_searchInProgress = false;
    ui->searchButton->setVisible( true );
    ui->spinnerWidget->setVisible( false );
    m_spinner->stop();
}

// intercept palette changes to modify the palette for svg images
void MainWindow::changeEvent( QEvent *event )
{
    if( event->type() == QEvent::PaletteChange )
        The::paletteHandler()->setPalette( palette() );
}

void MainWindow::openSessionManager()
{
    QPointer<SessionManager> manager = new SessionManager( m_sessionFilePath, this );
    if( manager->exec() ) {
        if( !QFile::exists( m_sessionFilePath + m_sessionFileName + QLatin1String( ".xml" ) ) ) {
            if( The::audioEngine()->state() == Phonon::PlayingState || The::audioEngine()->state() == Phonon::PausedState )
                The::audioEngine()->stop();

            m_sessionFileName = QLatin1String( "default" );
        }

        loadSession();
    }
    
    delete manager;
}

// resize some widgets' height to that of the lineedit
void MainWindow::resizeEvent( QResizeEvent *event )
{
    ui->svgLed->setFixedSize( QSize( ui->pathLine->height(), ui->pathLine->height() ) );
    ui->searchButton->setFixedSize( QSize( ui->pathLine->height(), ui->pathLine->height() ) );
    ui->compactMenuButton->setFixedSize( QSize( ui->pathLine->height(), ui->pathLine->height() ) );
    ui->browseButton->setFixedSize( QSize( ui->pathLine->height(), ui->pathLine->height() ) );
    ui->batchDownloadButton->setFixedSize( QSize( ui->pathLine->height(), ui->pathLine->height() ) );
    ui->spinnerWidget->setFixedSize( QSize( ui->pathLine->height(), ui->pathLine->height() ) );
    QWidget::resizeEvent( event );
}

