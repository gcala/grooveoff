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
#include "PlaylistItem.h"
#include "AudioEngine.h"
#include "ActionCollection.h"
#include "PaletteHandler.h"
#include "SvgHandler.h"
#include "Playlist.h"
#include "widgets/Spinner.h"
#include "../db/Catalog.h"
#include "../db/Database.h"
#include "../db/PlaylistStore.h"
#include "../torrent/StreamServer.h"
#include "../torrent/TorrentSession.h"

#include <QFileDialog>

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
#include <QDesktopServices>

#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QMessageBox>
#include <QApplication>
#include <QNetworkInformation>
#include <QPointer>

// version include
#include <../config-version.h>

#include <utility>

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
{
    // Qt6 registers the QDataStream operators along with the metatype
    qRegisterMetaType< PlaylistItemPtr >( "PlaylistItemPtr" );
    qRegisterMetaType< TrackInfoPtr >( "TrackInfoPtr" );

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
    m_spinner->setText( QString() );
    m_spinner->setType( Spinner::Sun );

    setWindowTitle( QString::fromLatin1( "GrooveOff %1" ).arg( GROOVEOFF_VERSION ) );

    setupGuiLayout();

    if( !The::database()->isOpen() ) {
        QMessageBox::critical( this,
                               tr( "Attention" ),
                               tr( "The database could not be opened:" ) + QLatin1String( "\n\n" )
                                   + The::database()->errorString() + QLatin1String( "\n\n" )
                                   + tr( "Playlists will not be remembered." ),
                               QMessageBox::Ok );
    }

    loadCurrentPlaylist();
    wakeLastUsedTorrent();

    m_mpris = new Mpris( this );
}

MainWindow::~MainWindow()
{
    delete The::paletteHandler();
    delete The::svgHandler();

    m_spinner->stop();

    savePlaybackState();
    saveSettings();

    delete m_mpris;
}

void MainWindow::setupUi()
{
    ui->logoLabel->setPixmap( QPixmap( QLatin1String( ":/resources/grooveoff.png" ) ) );
    ui->logoLabel->setMinimumHeight( 30 );
    ui->logoLabel->setMaximumWidth( 30 );
    ui->logoLabel->setScaledContents( 30 );

    ui->appLabel->setText( tr( "GrooveOff" ) );
    ui->appLabel->setFont( Utility::font( QFont::Bold, 2 ) );
    ui->subtitleLabel->setText( tr( "Music streaming from torrents" ) );

    m_playerWidget->showMessage( tr( "Idle" ) );

    ui->compactMenuButton->setType( IconButton::Settings );
    ui->compactMenuButton->setToolTip( tr( "Main menu" ) );
    ui->compactMenuButton->setMenu( m_compactMainMenu );

    // The search line becomes the magnet field, which is the least invasive
    // change the .ui can take: same widget, same layout, another meaning. The
    // rest of these go away for good when the interface is rewritten, and
    // doing that twice would be wasted work.
    ui->label3->setText( tr( "Magnet" ) );
    ui->searchLine->setPlaceholderText( tr( "Paste a magnet link and press enter" ) );
    ui->searchLine->setClearButtonEnabled( true );
    ui->searchButton->setVisible( false );
    ui->label4->setVisible( false );
    ui->pathLine->setVisible( false );
    ui->browseButton->setVisible( false );
    ui->combosContainer->setVisible( false );
    ui->matchList->setVisible( false );

    QFontMetrics fmSystemFont( Utility::font( QFont::Bold ) );
    const int fontHeight = fmSystemFont.height();

    ui->spinnerWidget->setFixedHeight( fontHeight > 25 ? fontHeight : 25 );
    ui->spinnerWidget->setVisible( false );

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

/*!
  \brief isOnline: tells whether we have network reachability
  \return true if online, or if no backend can tell us
*/
bool MainWindow::isOnline() const
{
    const QNetworkInformation *networkInfo = QNetworkInformation::instance();

    // without a backend we cannot know: assume we are online rather than
    // locking the ui in a permanent offline state
    if( !networkInfo )
        return true;

    return networkInfo->reachability() == QNetworkInformation::Reachability::Online;
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

    connect( ac->getAction( "actionClose" ), &QAction::triggered, this, &MainWindow::close );
    connect( ac->getAction( "actionDonate" ), &QAction::triggered, this, &MainWindow::donate );
    connect( ac->getAction( "actionConfigure" ), &QAction::triggered, this, &MainWindow::configure );
    connect( ac->getAction( "actionCompact" ), &QAction::triggered, this, &MainWindow::setCompactLayout );
    connect( ac->getAction( "actionWide" ), &QAction::triggered, this, &MainWindow::setWideLayout );
    connect( ac->getAction( "miniPlayer" ), &QAction::triggered, this, &MainWindow::setMiniPlayerLayout );
    connect( ac->getAction( "actionRemoveFailed" ), &QAction::triggered, ui->downloadList, &DownloadList::removeErrored );
    connect( ac->getAction( "actionClearDownloadList" ), &QAction::triggered, ui->downloadList, &DownloadList::removeAll );
    connect( ac->getAction( "actionAbout" ), &QAction::triggered, this, &MainWindow::about );
    connect( ac->getAction( "actionQtAbout" ), &QAction::triggered, qApp, &QApplication::aboutQt );
    connect( ac->getAction( "actionOpenTorrent" ), &QAction::triggered, this, &MainWindow::openTorrentFile );

    // Nothing is downloaded any more, so there is nothing to stop. The menus
    // are rebuilt around playlists later on.
    ac->getAction( "actionStopDownloads" )->setVisible( false );

    connect( ui->searchLine, &QLineEdit::returnPressed, this, &MainWindow::addFromMagnetField );

    connect( The::torrentSession(), &TorrentSession::metadataReady, this, &MainWindow::onMetadataReady );
    connect( The::torrentSession(), &TorrentSession::statsUpdated,  this, &MainWindow::onTorrentStats );
    connect( The::torrentSession(), &TorrentSession::torrentError,  this, &MainWindow::onTorrentError );

    // The catalogue is told a resume file exists so that the next start knows
    // it can reopen this torrent without the network.
    connect( The::torrentSession(), &TorrentSession::resumeDataSaved, this,
             []( const QString &infoHash ) {
        The::catalog()->setHasResume( infoHash, true );
    } );

    // network
    if( QNetworkInformation::loadDefaultBackend() && QNetworkInformation::instance() ) {
        connect( QNetworkInformation::instance(), &QNetworkInformation::reachabilityChanged,
                 this, [this]( QNetworkInformation::Reachability reachability ) {
                     onlineStateChanged( reachability == QNetworkInformation::Reachability::Online );
                 } );
    } else {
        qWarning() << "GrooveOff ::" << "no QNetworkInformation backend available, assuming we are online";
    }

    onlineStateChanged( isOnline() );
}

void MainWindow::loadSettings()
{
    QSettings settings;

    setGeometry( settings.value( QLatin1String( "windowGeometry" ), QRect( 100, 100, 350, 600 ) ).toRect() );
}

void MainWindow::loadBootSettings()
{
    QSettings settings;

    m_guiLayout = ( GuiLayout )settings.value( QLatin1String( "guiLayout" ), Compact ).toInt();

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

    settings.setValue( QLatin1String( "timerState" ), m_playerWidget->getTimerState() );
    settings.setValue( QLatin1String( "muted" ), The::audioEngine()->isMuted() );
    settings.setValue( QLatin1String( "volume" ), The::audioEngine()->volume() );
    settings.setValue( QLatin1String( "guiLayout" ), m_guiLayout );
}

/*!
  \brief loadCurrentPlaylist: fill the list from the playlist app_state points at
  \return void
*/
void MainWindow::loadCurrentPlaylist()
{
    if( !The::database()->isOpen() )
        return;

    PlaylistStore *playlists = The::playlistStore();

    const qint64 playlistId = playlists->currentPlaylistId();
    if( playlistId <= 0 )
        return;

    // disable temporarily AutoScroll
    ui->downloadList->setAutoScroll( false );
    ui->downloadList->removeAll();

    const QList<TrackInfoPtr> &tracks = playlists->tracks( playlistId );
    for( const TrackInfoPtr &track : tracks ) {
        addPlaylistItem( PlaylistItemPtr( new PlaylistItem( track ) ) );
    }

    ui->downloadList->setAutoScroll( true );
    ui->downloadList->reloadPlaylist();

    // The track that was playing is selected but not started: pressing play
    // picks it up where it was left.
    const qint64 trackId = playlists->currentTrackId();
    if( trackId <= 0 )
        return;

    for( int i = 0; i < ui->downloadList->count(); ++i ) {
        DownloadItem *item = qobject_cast<DownloadItem *>( ui->downloadList->itemWidget( ui->downloadList->item( i ) ) );
        if( item && item->playlistItem()->track()->id() == trackId ) {
            ui->downloadList->setCurrentRow( i );
            break;
        }
    }
}

/*!
  \brief wakeLastUsedTorrent: reactivation is lazy, with one exception

  Every other row waits for somebody to press play on it. This one does not,
  because it is the one the user left playing and almost certainly the one they
  will press play on: paying its few seconds now, while the window is opening,
  is better than paying them at the first click.
*/
void MainWindow::wakeLastUsedTorrent()
{
    if( !The::torrentSession()->isRunning() )
        return;

    const qint64 trackId = The::playlistStore()->currentTrackId();
    if( trackId <= 0 )
        return;

    const TrackInfoPtr track = The::catalog()->track( trackId );
    if( !track || track->infoHash().isEmpty() )
        return;

    QString error;
    if( !The::torrentSession()->reopen( track->infoHash(), track->magnetUri(), &error ) )
        qDebug() << "GrooveOff ::" << "cannot reopen the last used torrent:" << error;
}

void MainWindow::addFromMagnetField()
{
    const QString source = ui->searchLine->text().trimmed();
    if( source.isEmpty() )
        return;

    addTorrentSource( source );
}

void MainWindow::openTorrentFile()
{
    const QString fileName = QFileDialog::getOpenFileName( this,
                                                           tr( "Open a torrent" ),
                                                           QString(),
                                                           tr( "Torrents (*.torrent)" ) );
    if( !fileName.isEmpty() )
        addTorrentSource( fileName );
}

void MainWindow::addTorrentSource( const QString &source )
{
    QString error;
    const QString infoHash = source.startsWith( QLatin1String( "magnet:" ) )
                             ? The::torrentSession()->addMagnet( source, &error )
                             : The::torrentSession()->addTorrentFile( source, &error );

    if( infoHash.isEmpty() ) {
        // No modal for this: what was typed stays in the field so that it can
        // be corrected, and the reason is on the status bar next to it.
        m_playerWidget->showMessage( error );
        return;
    }

    ui->searchLine->clear();
    m_playerWidget->showMessage( tr( "Reading the track list…" ) );
}

/*!
  \brief onMetadataReady: a torrent has told us what is inside it

  Every file is stored, audio or not: an extension nobody thought of would
  otherwise become a track that simply is not there, with no way to tell why.
  What goes into the playlist is the audio, minus whatever was excluded before.
*/
void MainWindow::onMetadataReady( const QString &infoHash, const QString &name, const QList<TrackInfoPtr> &tracks )
{
    if( !The::database()->isOpen() )
        return;

    Catalog *catalog = The::catalog();

    catalog->addTorrent( infoHash, The::torrentSession()->magnetUri( infoHash ), name );
    catalog->setName( infoHash, name );
    catalog->addTracks( tracks );
    catalog->touch( infoHash );

    const qint64 playlistId = The::playlistStore()->torrentPlaylistId( infoHash );
    if( playlistId <= 0 )
        return;

    // A torrent already known keeps the list it had: reopening it must not
    // undo an exclusion or a removal made last time.
    if( The::playlistStore()->tracks( playlistId ).isEmpty() )
        The::playlistStore()->appendTracks( playlistId, catalog->tracks( infoHash ) );

    if( The::playlistStore()->currentPlaylistId() != playlistId ) {
        The::playlistStore()->setPlaybackState( playlistId, 0, 0 );
        loadCurrentPlaylist();
    }

    refreshRowsOf( infoHash );
    m_playerWidget->showMessage( name );
}

void MainWindow::onTorrentStats( const QString &infoHash, int peers, int seeds, qint64 rate, qint64 bytesDone )
{
    Q_UNUSED( rate )

    The::catalog()->setBytesDone( infoHash, bytesDone );

    // Red with nobody to talk to, yellow with peers, green with a seed: the
    // difference the user actually feels is whether anything is coming in.
    if( peers == 0 )
        ui->svgLed->setType( IconButton::Offline );
    else
        ui->svgLed->setType( IconButton::Online );

    ui->svgLed->setToolTip( tr( "%n peer(s)", "", peers ) + QLatin1String( ", " ) + tr( "%n seed(s)", "", seeds ) );
}

void MainWindow::onTorrentError( const QString &infoHash, const QString &message )
{
    Q_UNUSED( infoHash )

    // Errors on a torrent are ordinary, not exceptional: a modal in the middle
    // of listening would be the most annoying thing this application could do.
    m_playerWidget->showMessage( message );
}

void MainWindow::refreshRowsOf( const QString &infoHash )
{
    const bool awake = The::torrentSession()->hasMetadata( infoHash );

    for( int i = 0; i < ui->downloadList->count(); ++i ) {
        DownloadItem *item = qobject_cast<DownloadItem *>( ui->downloadList->itemWidget( ui->downloadList->item( i ) ) );
        if( !item || !item->playlistItem()->track() )
            continue;

        if( item->playlistItem()->track()->infoHash() != infoHash )
            continue;

        if( item->trackState() != GrooveOff::ErrorState )
            item->setTrackState( awake ? GrooveOff::ReadyState : GrooveOff::DormantState );
    }
}

void MainWindow::savePlaybackState()
{
    if( !The::database()->isOpen() )
        return;

    const PlaylistItemPtr &current = The::audioEngine()->currentTrack();

    The::playlistStore()->setPlaybackState( The::playlistStore()->currentPlaylistId(),
                                            current && current->track() ? current->track()->id() : 0,
                                            The::audioEngine()->currentTime() );
}

void MainWindow::addPlaylistItem( const PlaylistItemPtr &playlistItem )
{
    DownloadItem *item = new DownloadItem( playlistItem, this );

    connect( item,             &DownloadItem::reloadPlaylist,
             ui->downloadList, &DownloadList::reloadPlaylist );

    connect( item,             &DownloadItem::removeMe,
             ui->downloadList, &DownloadList::removeItem );

    QListWidgetItem *wi = new QListWidgetItem;
    ui->downloadList->addItem( wi );
    ui->downloadList->setItemWidget( wi, item );
    wi->setSizeHint( QSize( Utility::coverSize + Utility::marginSize * 2,
                            Utility::coverSize + Utility::marginSize * 2
                          )
                   );
}

void MainWindow::setCompactLayout()
{
    QSettings settings;

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
    if( isOnline ) {
        ui->svgLed->setType( IconButton::Online );
        ui->svgLed->setToolTip( tr( "Online" ) );
        m_playerWidget->showMessage( tr( "Idle" ) );
    } else {
        // Tracks already in the cache still play: only reaching new peers needs
        // the network.
        ui->svgLed->setType( IconButton::Offline );
        ui->svgLed->setToolTip( tr( "Offline: only cached tracks can be played" ) );
        m_playerWidget->showMessage( tr( "Offline" ) );
    }
}

void MainWindow::donate()
{
    QDesktopServices::openUrl( QUrl( QLatin1String( "https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=WJNETV7GLTKDG&item_name=" ) + tr( "Donation to Grooveoff's author" ) ) );
}

// intercept palette changes to modify the palette for svg images
void MainWindow::changeEvent( QEvent *event )
{
    if( event->type() == QEvent::PaletteChange )
        The::paletteHandler()->setPalette( palette() );
}

// resize some widgets' height to that of the header
void MainWindow::resizeEvent( QResizeEvent *event )
{
    QFontMetrics fmSystemFont( Utility::font( QFont::Bold ) );
    const int side = qMax( fmSystemFont.height(), 24 );

    ui->svgLed->setFixedSize( QSize( side, side ) );
    ui->compactMenuButton->setFixedSize( QSize( side, side ) );
    ui->spinnerWidget->setFixedSize( QSize( side, side ) );
    QWidget::resizeEvent( event );
}
