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
#include "TrackSelector.h"
#include "TrackTags.h"
#include "Settings.h"
#include "widgets/PlaylistList.h"
#include "widgets/PlaylistDelegate.h"
#include "../torrent/CacheManager.h"
#include "../torrent/DownloadQueue.h"
#include "../torrent/StreamServer.h"
#include "../torrent/TorrentSession.h"

#include <QFileDialog>
#include <QTimer>

#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QAction>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QInputDialog>
#include <QLocale>
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

    loadPlaylists();
    loadCurrentPlaylist();
    wakeLastUsedTorrent();

    m_mpris = new Mpris( this );
}

MainWindow::~MainWindow()
{
    // First of all, and not as a precaution: the child widgets are destroyed
    // before this object's own QObject destructor runs, so a signal arriving in
    // between finds handlers that reach into a ui that is already half gone.
    // PlayerWidget stops the player as it goes, which is exactly such a signal.
    disconnect( The::audioEngine(),  nullptr, this, nullptr );
    disconnect( The::torrentSession(), nullptr, this, nullptr );
    disconnect( The::streamServer(),   nullptr, this, nullptr );

    delete The::paletteHandler();
    delete The::svgHandler();

    m_spinner->stop();

    // The playback state is not written here: App asks for it before it stops
    // the player, because the position comes from a player that is still
    // running. By the time this destructor runs it would be a zero.
    saveSettings();

    delete m_mpris;
}

void MainWindow::setupUi()
{
    // Scaled once, to a square, rather than stretched to whatever the label
    // ends up being. It used to be told to fill its label and to keep its width
    // under thirty, and it looked right only because the led beside it was
    // fixed at thirty and held the whole row to that height. Take the led away
    // and the row grows to the labels, the logo grows with it, and a round icon
    // comes out an oval — a widget's shape depending on a neighbour that has
    // nothing to do with it.
    const int logoSide = 30;
    ui->logoLabel->setFixedSize( logoSide, logoSide );
    ui->logoLabel->setAlignment( Qt::AlignCenter );
    ui->logoLabel->setPixmap( QPixmap( QLatin1String( ":/resources/grooveoff.png" ) )
                              .scaled( logoSide, logoSide,
                                       Qt::KeepAspectRatio, Qt::SmoothTransformation ) );

    ui->appLabel->setText( tr( "GrooveOff" ) );
    ui->appLabel->setFont( Utility::font( QFont::Bold, 2 ) );
    ui->subtitleLabel->setText( tr( "Music streaming from torrents" ) );

    m_playerWidget->showMessage( tr( "Idle" ) );

    ui->compactMenuButton->setType( IconButton::Settings );
    ui->compactMenuButton->setToolTip( tr( "Main menu" ) );
    ui->compactMenuButton->setMenu( m_compactMainMenu );

    // The search line is the magnet field: same widget, same layout, another
    // meaning. Everything else the search era needed — the result list, the
    // artist and album filters, the destination folder — is gone from the .ui.
    ui->label3->setText( tr( "Magnet" ) );
    ui->searchLine->setPlaceholderText( tr( "Paste a magnet link and press enter" ) );
    ui->searchLine->setClearButtonEnabled( true );
    ui->searchButton->setVisible( false );

    // No pane may go to zero, by dragging or otherwise. A collapsed track list
    // looks exactly like a playlist that did not load, and there is nothing on
    // screen to tell whoever is looking which of the two it is.
    ui->splitter->setChildrenCollapsible( false );
    ui->splitter->setStretchFactor( 0, 0 );
    ui->splitter->setStretchFactor( 1, 1 );

    QFontMetrics fmSystemFont( Utility::font( QFont::Bold ) );
    const int fontHeight = fmSystemFont.height();

    ui->spinnerWidget->setFixedHeight( fontHeight > 25 ? fontHeight : 25 );
    ui->spinnerWidget->setVisible( false );

    ui->splitter->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Expanding );

    m_playerWidget->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Fixed );

    ui->playlistList->setItemDelegate( new PlaylistDelegate( this ) );
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
    connect( ac->getAction( "actionEmptyCache" ), &QAction::triggered, this, &MainWindow::emptyCache );
    connect( ac->getAction( "actionNewPlaylist" ), &QAction::triggered, this, &MainWindow::newPlaylist );
    connect( ac->getAction( "actionRenamePlaylist" ), &QAction::triggered, this, &MainWindow::renamePlaylist );
    connect( ac->getAction( "actionRemovePlaylist" ), &QAction::triggered, this, &MainWindow::removePlaylist );

    // currentItemChanged rather than itemClicked, so that the keyboard opens a
    // playlist too. Selecting a row from the code blocks the signal, or every
    // rebuild of the sidebar would reopen the list that is already open.
    connect( ui->playlistList, &QListWidget::currentItemChanged, this,
             [this]( QListWidgetItem *current, QListWidgetItem * ) { openPlaylist( current ); } );

    connect( ui->playlistList, &QWidget::customContextMenuRequested, this, &MainWindow::showPlaylistMenu );
    connect( ui->playlistList, &PlaylistList::tracksDropped, this, &MainWindow::addTracksToPlaylist );

    connect( ui->downloadList, &DownloadList::reorderRequested, this, &MainWindow::reorderTracks );

    // Nothing is downloaded any more, so there is nothing to stop. The menus
    // are rebuilt around playlists later on.
    ac->getAction( "actionStopDownloads" )->setVisible( false );

    connect( ui->searchLine, &QLineEdit::returnPressed, this, &MainWindow::addFromMagnetField );

    connect( The::torrentSession(), &TorrentSession::metadataReady, this, &MainWindow::onMetadataReady );
    connect( The::torrentSession(), &TorrentSession::statsUpdated,  this, &MainWindow::onTorrentStats );

    connect( The::torrentSession(), &TorrentSession::torrentError,  this, &MainWindow::onTorrentError );
    connect( The::torrentSession(), &TorrentSession::storageError,  this, &MainWindow::onStorageError );

    // The catalogue is told a resume file exists so that the next start knows
    // it can reopen this torrent without the network.
    connect( The::torrentSession(), &TorrentSession::resumeDataSaved, this,
             []( const QString &infoHash ) {
        The::catalog()->setHasResume( infoHash, true );
    } );

    // A torrent whose bytes were thrown away is not an error and nothing is
    // missing: its rows go back to Dormant and are fetched again at the next
    // play, which is what a cache is for.
    connect( The::cacheManager(), &CacheManager::evicted, this, &MainWindow::refreshRowsOf );

    // The head of a file is where its tags are, so this is the first moment at
    // which asking the file what it is called gets an answer.
    connect( The::streamServer(), &StreamServer::trackPlayable, this, &MainWindow::readTagsOf );

    // And again when a track starts, because a track whose head was already in
    // the cache never had to wait and so was never announced as playable.
    connect( The::audioEngine(), &AudioEngine::sourceChanged, this, [this] {
        const PlaylistItemPtr current = The::audioEngine()->currentTrack();
        if( !current || !current->track() )
            return;

        readTagsOf( current->track()->infoHash(), current->track()->fileIndex() );

        // And the download queue moves its cursor here: what comes after this
        // track is what will be wanted next.
        The::downloadQueue()->setPlaying( current->track()->infoHash(),
                                          current->track()->fileIndex() );
    } );

    // The row of the track being listened to follows the player: it is the one
    // place where waiting has to be visible, because it is the one the user is
    // looking at when nothing happens.
    connect( The::audioEngine(), &AudioEngine::stateChanged, this,
             [this]( GrooveOff::PlayerState state ) {
        const PlaylistItemPtr current = The::audioEngine()->currentTrack();
        if( !current )
            return;

        // The listening has moved on, so whatever the previous row was showing
        // was about a moment that has passed. Left alone it stays there for
        // good — a track that stumbled on its last frames kept saying it was
        // unavailable long after it had finished playing, with the next one
        // already going.
        if( m_playingTrack && m_playingTrack != current->track() ) {
            if( DownloadItem *previous = rowFor( m_playingTrack ) ) {
                previous->setTrackState( The::torrentSession()->hasMetadata( m_playingTrack->infoHash() )
                                         ? GrooveOff::ReadyState
                                         : GrooveOff::DormantState );
            }
        }

        m_playingTrack = current->track();

        DownloadItem *item = rowFor( current->track() );
        if( !item )
            return;

        switch( state ) {
            case GrooveOff::PlayerState::Buffering:
                item->setTrackState( GrooveOff::BufferingState );
                m_stallTimer->start();
                break;
            case GrooveOff::PlayerState::Error:
                item->setTrackState( GrooveOff::ErrorState );
                m_stallTimer->stop();
                break;
            default:
                item->setTrackState( GrooveOff::ReadyState );
                m_stallTimer->stop();
        }
    } );

    // Buffering that goes on long enough stops being buffering and becomes
    // waiting: the row says so rather than spinning for ever with nothing to
    // show. It is not an error — a swarm can wake up — so nothing is given up.
    m_stallTimer = new QTimer( this );
    m_stallTimer->setInterval( 20000 );
    m_stallTimer->setSingleShot( true );

    connect( m_stallTimer, &QTimer::timeout, this, [this] {
        const PlaylistItemPtr current = The::audioEngine()->currentTrack();
        if( !current )
            return;

        DownloadItem *item = rowFor( current->track() );
        if( item && item->trackState() == GrooveOff::BufferingState )
            item->setTrackState( GrooveOff::StalledState );
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

    restoreSplitterSizes();

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
/*!
  \brief restoreSplitterSizes: the saved split, but only when it still fits

  A list saved when the splitter had a different number of panes is not a
  shorter version of this one. Applied as it is, the panes it says nothing
  about end up at zero — and since what gets saved on the way out is whatever
  is on screen, a track list that once ended up with no height keeps no height
  for ever after. That is not a hypothetical: the sidebar arriving beside a
  list that had been alone in the splitter did exactly this.
*/
void MainWindow::restoreSplitterSizes()
{
    QSettings settings;
    const QList<int> sizes = settings.value( QLatin1String( "splitterSizes" ) ).value< QList<int> >();

    if( sizes.count() == ui->splitter->count() && !sizes.contains( 0 ) ) {
        ui->splitter->setSizes( sizes );
        return;
    }

    // Enough for the sidebar to show a handful of names, and the rest to the
    // tracks. A fixed number rather than a fraction because this runs before
    // the splitter has been laid out, so its own size is not worth asking for;
    // the stretch factors then give every later pixel to the track list.
    ui->splitter->setSizes( { 200, qMax( ui->splitter->height() - 200, 200 ) } );
}

void MainWindow::loadCurrentPlaylist()
{
    if( !The::database()->isOpen() )
        return;

    PlaylistStore *playlists = The::playlistStore();

    const qint64 playlistId = playlists->currentPlaylistId();

    // disable temporarily AutoScroll
    ui->downloadList->setAutoScroll( false );
    ui->downloadList->removeAll();

    // No playlist open is a state to be shown, not a reason to do nothing.
    // Returning here before emptying the list is what left the tracks of the
    // last playlist on screen after it was removed: the sidebar was empty, the
    // rows were still there, and they went only at the next start — which reads
    // as a removal that did not work rather than one that did.
    if( playlistId <= 0 ) {
        ui->downloadList->setAutoScroll( true );
        ui->downloadList->reloadPlaylist();
        m_playerWidget->showMessage( tr( "Idle" ) );
        return;
    }

    const QList<TrackInfoPtr> &tracks = playlists->tracks( playlistId );
    for( const TrackInfoPtr &track : tracks ) {
        addPlaylistItem( PlaylistItemPtr( new PlaylistItem( track ) ) );
    }

    ui->downloadList->setAutoScroll( true );
    ui->downloadList->reloadPlaylist();

    // The track that was playing is selected but not started: pressing play
    // picks it up where it was left.
    QSettings settings;
    if( !settings.value( Settings::restorePlayback, Settings::restorePlaybackDefault ).toBool() )
        return;

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

    QSettings settings;

    const qint64 trackId = The::playlistStore()->currentTrackId();
    const TrackInfoPtr track = trackId > 0 ? The::catalog()->track( trackId ) : TrackInfoPtr();

    // Nothing is being brought back, so there is nothing worth announcing to a
    // tracker before the user has asked for anything.
    if( track && !track->infoHash().isEmpty()
        && settings.value( Settings::restorePlayback, Settings::restorePlaybackDefault ).toBool() ) {
        QString error;
        if( !The::torrentSession()->reopen( track->infoHash(), track->magnetUri(), &error ) )
            qDebug() << "GrooveOff ::" << "cannot reopen the last used torrent:" << error;
    }

    if( !The::downloadQueue()->isEnabled() )
        return;

    // Fetching ahead cannot happen in a torrent the session is not holding, so
    // reactivation stops being lazy for the torrents that still have music to
    // fetch. Not for the others: one whose chosen tracks are already in the
    // cache has nothing to do on the network, and waking it would announce it
    // to a tracker to accomplish precisely nothing. So the announcements stay
    // proportionate to the work left, and once everything has been fetched a
    // start up goes back to being as quiet as it was before.
    wakeUnfinishedTorrents();
}

void MainWindow::wakeUnfinishedTorrents()
{
    const QList<PlaylistStore::PlaylistInfo> playlists = The::playlistStore()->visiblePlaylists();

    for( const PlaylistStore::PlaylistInfo &playlist : playlists ) {
        if( playlist.kind == PlaylistStore::CustomPlaylist || playlist.infoHash.isEmpty() )
            continue;

        if( The::torrentSession()->isActive( playlist.infoHash ) )
            continue;

        const QList<TrackInfoPtr> tracks = The::playlistStore()->tracks( playlist.id );
        if( tracks.isEmpty() )
            continue;

        qint64 wanted = 0;
        for( const TrackInfoPtr &track : tracks )
            wanted += track->fileSize();

        // Rough on purpose, and rough in the safe direction: bytes_done counts
        // every piece the torrent holds, including the ones shared with files
        // nobody chose, so a torrent that has enough bytes to cover the chosen
        // tracks is probably done with them. Being wrong here costs one torrent
        // waking up for nothing; being wrong the other way would leave a
        // playlist that never finishes fetching.
        if( The::catalog()->bytesDone( playlist.infoHash ) >= wanted )
            continue;

        QString error;
        if( !The::torrentSession()->reopen( playlist.infoHash, tracks.first()->magnetUri(), &error ) )
            qDebug() << "GrooveOff ::" << "cannot wake" << playlist.infoHash << "to fetch ahead:" << error;
    }
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
    m_awaitingSelection.insert( infoHash );
    m_playerWidget->showMessage( tr( "Reading the track list…" ) );

    // A torrent whose file list is already in hand posts no alert, so the
    // selector has to be opened here instead of waiting for one.
    if( The::torrentSession()->hasMetadata( infoHash ) && m_awaitingSelection.remove( infoHash ) )
        QTimer::singleShot( 0, this, [this, infoHash] { chooseTracksOf( infoHash ); } );
}

/*!
  \brief onMetadataReady: a torrent has told us what is inside it

  Every file is stored, audio or not: an extension nobody thought of would
  otherwise become a track that simply is not there, with no way to tell why.
  What goes into the playlist is then up to the user, through the selector.
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

    // A torrent that was hidden and has just been added again is back in the
    // sidebar, and one added for the first time appears there now.
    loadPlaylists();
    refreshRowsOf( infoHash );

    if( !m_awaitingSelection.remove( infoHash ) ) {
        // A torrent woken up to play something, not one just added: it keeps
        // the playlist it had and does not steal the view.
        m_playerWidget->showMessage( name );
        return;
    }

    // Out of the alert handler before opening a modal: the dialog spins its own
    // event loop, and doing that in the middle of draining libtorrent's alerts
    // is asking for trouble that only shows up under load.
    QTimer::singleShot( 0, this, [this, infoHash] { chooseTracksOf( infoHash ); } );
}

void MainWindow::chooseTracksOf( const QString &infoHash )
{
    Catalog *catalog = The::catalog();
    PlaylistStore *playlists = The::playlistStore();

    const qint64 playlistId = playlists->torrentPlaylistId( infoHash );
    if( playlistId <= 0 )
        return;

    // Everything, excluded and non audio included: hiding them here is what
    // makes a missing track impossible to explain.
    const QList<TrackInfoPtr> all = catalog->tracks( infoHash, false, true );

    QList<qint64> chosen;
    for( const TrackInfoPtr &track : playlists->tracks( playlistId ) )
        chosen << track->id();

    TrackSelector selector( catalog->name( infoHash ), all, chosen, this );

    if( selector.exec() != QDialog::Accepted ) {
        // The torrent stays in the catalogue — it may be chosen from later —
        // but with nothing queued there is no reason to keep announcing it.
        if( playlists->tracks( playlistId ).isEmpty() )
            The::torrentSession()->remove( infoHash, false );

        m_playerWidget->showMessage( catalog->name( infoHash ) );
        return;
    }

    // Ticking a file that had been refused is how a refusal is taken back.
    for( const TrackInfoPtr &track : selector.readmittedTracks() )
        catalog->setExcluded( track->id(), false );

    playlists->setTracks( playlistId, selector.selectedTracks() );
    playlists->setPlaybackState( playlistId, 0, 0 );

    loadPlaylists();
    loadCurrentPlaylist();
    refreshRowsOf( infoHash );

    // Whatever happens, the message stops saying the track list is being read:
    // it was read, and leaving that sentence there is the interface claiming to
    // be busy with something it finished several clicks ago.
    m_playerWidget->showMessage( catalog->name( infoHash ) );
}

/*!
  \brief excludeTrack: the refusal that sticks

  Taking a row out of the playlist and refusing a file are two different
  things, and conflating them is the mistake that has the user pressing select
  all and watching the same three tracks come back for ever.
*/
void MainWindow::excludeTrack( DownloadItem *item )
{
    if( !item || !item->playlistItem()->track() )
        return;

    const TrackInfoPtr track = item->playlistItem()->track();

    The::catalog()->setExcluded( track->id(), true );

    The::audioEngine()->removingTrack( item->playlistItem() );
    The::playlist()->removeItem( item->playlistItem() );
    ui->downloadList->removeItem( item );

    savePlaylistOrder();
}

void MainWindow::savePlaylistOrder()
{
    if( !The::database()->isOpen() )
        return;

    const qint64 playlistId = The::playlistStore()->currentPlaylistId();
    if( playlistId <= 0 )
        return;

    QList<TrackInfoPtr> tracks;
    for( int i = 0; i < ui->downloadList->count(); ++i ) {
        DownloadItem *item = qobject_cast<DownloadItem *>( ui->downloadList->itemWidget( ui->downloadList->item( i ) ) );
        if( item && item->playlistItem()->track() )
            tracks << item->playlistItem()->track();
    }

    The::playlistStore()->setTracks( playlistId, tracks );
}

void MainWindow::onTorrentStats( const QString &infoHash, int peers, int seeds, qint64 rate, qint64 bytesDone )
{
    Q_UNUSED( rate )

    The::catalog()->setBytesDone( infoHash, bytesDone );

    const PlaylistItemPtr current = The::audioEngine()->currentTrack();
    if( !current || !current->track() || current->track()->infoHash() != infoHash )
        return;

    // Bytes are still coming in, so the wait is buffering rather than a stall,
    // and the clock that would call it one goes back to the start. A slow swarm
    // is not a silent one, and a row saying it is waiting while its own
    // progress bar fills would be the interface contradicting itself.
    if( infoHash != m_lastStatsHash )
        m_lastBytesDone = -1;

    if( bytesDone > m_lastBytesDone && m_stallTimer->isActive() )
        m_stallTimer->start();

    m_lastStatsHash = infoHash;
    m_lastBytesDone = bytesDone;

    DownloadItem *item = rowFor( current->track() );
    if( !item )
        return;

    item->setPeerInfo( peers, seeds );
    item->setBuffered( The::torrentSession()->bufferedBytes( infoHash, current->track()->fileIndex() ),
                       current->track()->fileSize() );
}

/*!
  \brief readTagsOf: takes what the file says about itself, once it can

  Tags live at the head of a file, so this is called when that head has
  arrived and not before: asking a hole what it is called gets no answer, and
  the row would keep the file name for ever.
*/
void MainWindow::readTagsOf( const QString &infoHash, int fileIndex )
{
    const TrackInfoPtr track = The::catalog()->track( infoHash, fileIndex );
    if( !track )
        return;

    if( !TrackTags::read( track ) )
        return;

    The::catalog()->updateTags( track );

    DownloadItem *item = rowFor( track );
    if( !item )
        return;

    // The row holds its own copy of the track, so the tags are copied onto it
    // rather than the row being rebuilt: a rebuild would lose the state it is
    // showing at the very moment it starts playing.
    const TrackInfoPtr shown = item->playlistItem()->track();
    shown->setTitle( track->title() );
    shown->setArtist( track->artist() );
    shown->setAlbum( track->album() );
    shown->setTrackNum( track->trackNum() );
    shown->setYear( track->year() );

    // Announced on the track rather than set on the row: the player bar is
    // looking at the same object and needs the same news, and a signal is the
    // only way of saying it once to everybody who is showing it. Setting it on
    // the row is what left the bar with the file name — track number and all —
    // beside a row that had the title.
    item->playlistItem()->requireRefresh();
}

/*!
  \brief emptyCache: the bytes go, the playlists stay

  Worth a confirmation even though nothing is lost for good, because emptying a
  cache sounds irreversible; the question says what it actually costs, which is
  fetching those tracks again the next time they are played.
*/
void MainWindow::emptyCache()
{
    const qint64 used = The::cacheManager()->usage();

    if( used <= 0 ) {
        statusBar()->showMessage( tr( "The cache is already empty." ), 4000 );
        return;
    }

    const int answer = QMessageBox::question( this,
                                              tr( "Empty the cache" ),
                                              tr( "Free %1 of downloaded data?" )
                                                  .arg( QLocale().formattedDataSize( used ) )
                                                  + QLatin1String( "\n\n" )
                                                  + tr( "Your playlists are not touched: the tracks will "
                                                        "be fetched again the next time you play them." ),
                                              QMessageBox::Yes | QMessageBox::No,
                                              QMessageBox::No );

    if( answer != QMessageBox::Yes )
        return;

    The::cacheManager()->emptyCache();

    statusBar()->showMessage( tr( "%1 freed." ).arg( QLocale().formattedDataSize( used - The::cacheManager()->usage() ) ),
                              4000 );
}

DownloadItem* MainWindow::rowFor( const TrackInfoPtr &track ) const
{
    if( !track )
        return nullptr;

    for( int i = 0; i < ui->downloadList->count(); ++i ) {
        DownloadItem *item = qobject_cast<DownloadItem *>( ui->downloadList->itemWidget( ui->downloadList->item( i ) ) );
        if( !item || !item->playlistItem()->track() )
            continue;

        if( item->playlistItem()->track()->infoHash() == track->infoHash()
            && item->playlistItem()->track()->fileIndex() == track->fileIndex() ) {
            return item;
        }
    }

    return nullptr;
}

void MainWindow::onTorrentError( const QString &infoHash, const QString &message )
{
    // Errors on a torrent are ordinary, not exceptional: a modal in the middle
    // of listening would be the most annoying thing this application could do.
    m_playerWidget->showMessage( message );

    // A torrent that never produced a file list has no selector to open, and
    // waiting for one would leave the row hanging.
    m_awaitingSelection.remove( infoHash );

    for( int i = 0; i < ui->downloadList->count(); ++i ) {
        DownloadItem *item = qobject_cast<DownloadItem *>( ui->downloadList->itemWidget( ui->downloadList->item( i ) ) );
        if( item && item->playlistItem()->track()
            && item->playlistItem()->track()->infoHash() == infoHash ) {
            item->setTrackState( GrooveOff::ErrorState );
        }
    }
}

/*!
  \brief onStorageError: the one interruption worth making

  Everything else goes to the status bar, because a network that comes and
  goes is the normal weather of a torrent. A cache that cannot be written to
  is different: nothing will play until somebody does something about it, so
  saying it quietly would only mean saying it to nobody.
*/
void MainWindow::onStorageError( const QString &infoHash, const QString &message )
{
    Q_UNUSED( infoHash )

    QMessageBox::critical( this,
                           tr( "Attention" ),
                           tr( "The cache could not be written to:" ) + QLatin1String( "\n\n" )
                               + message + QLatin1String( "\n\n" )
                               + tr( "Nothing will play until there is room for it." ),
                           QMessageBox::Ok );
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

/*!
  \brief loadPlaylists: the sidebar, of both kinds at once

  A list born from a torrent and one built by hand are the same thing here — an
  ordered list of tracks — so they share a table, a query and a row. What tells
  them apart is the icon and, when it matters, what removing them means.
*/
void MainWindow::loadPlaylists()
{
    if( !The::database()->isOpen() )
        return;

    const qint64 open = The::playlistStore()->currentPlaylistId();

    ui->playlistList->clear();

    The::downloadQueue()->forgetAll();

    const QList<PlaylistStore::PlaylistInfo> playlists = The::playlistStore()->visiblePlaylists();
    for( const PlaylistStore::PlaylistInfo &playlist : playlists ) {
        ui->playlistList->addPlaylist( playlist );

        // The queue is told about every torrent playlist and not only the open
        // one: what is being fetched ahead has nothing to do with what is being
        // looked at, and a playlist nobody has clicked on this week is still
        // one whose tracks somebody chose.
        if( playlist.kind == PlaylistStore::CustomPlaylist )
            continue;

        QList<int> fileIndexes;
        const QList<TrackInfoPtr> tracks = The::playlistStore()->tracks( playlist.id );
        for( const TrackInfoPtr &track : tracks )
            fileIndexes << track->fileIndex();

        The::downloadQueue()->setSelection( playlist.infoHash, fileIndexes );
    }

    ui->playlistList->selectPlaylist( open );

    // Playlists but nothing open — a database whose playback row went missing,
    // or a first start right after one was made — would show a full sidebar
    // beside an empty list, which reads as something having failed to load.
    if( open <= 0 && ui->playlistList->count() > 0 )
        openPlaylist( ui->playlistList->item( 0 ) );
}

void MainWindow::openPlaylist( QListWidgetItem *row )
{
    if( !row || !The::database()->isOpen() )
        return;

    const qint64 playlistId = row->data( PlaylistRoles::Id ).toLongLong();
    if( playlistId <= 0 || playlistId == The::playlistStore()->currentPlaylistId() )
        return;

    // Where the listening had got to belongs to the playlist being left, and it
    // has to be written before app_state points somewhere else.
    savePlaybackState();

    The::playlistStore()->setPlaybackState( playlistId, 0, 0 );

    loadCurrentPlaylist();
}

void MainWindow::newPlaylist()
{
    if( !The::database()->isOpen() )
        return;

    bool accepted = false;
    const QString name = QInputDialog::getText( this,
                                                tr( "New playlist" ),
                                                tr( "Name:" ),
                                                QLineEdit::Normal,
                                                tr( "New playlist" ),
                                                &accepted ).trimmed();

    if( !accepted || name.isEmpty() )
        return;

    const qint64 playlistId = The::playlistStore()->createCustomPlaylist( name );
    if( playlistId <= 0 )
        return;

    savePlaybackState();
    The::playlistStore()->setPlaybackState( playlistId, 0, 0 );

    loadPlaylists();
    loadCurrentPlaylist();

    m_playerWidget->showMessage( tr( "Drag tracks into «%1» to fill it." ).arg( name ) );
}

void MainWindow::renamePlaylist()
{
    const qint64 playlistId = ui->playlistList->currentPlaylistId();
    if( playlistId <= 0 )
        return;

    const PlaylistStore::PlaylistInfo playlist = The::playlistStore()->playlist( playlistId );
    if( !playlist.isValid() )
        return;

    bool accepted = false;
    const QString name = QInputDialog::getText( this,
                                                tr( "Rename playlist" ),
                                                tr( "Name:" ),
                                                QLineEdit::Normal,
                                                playlist.name,
                                                &accepted ).trimmed();

    if( !accepted || name.isEmpty() || name == playlist.name )
        return;

    The::playlistStore()->rename( playlistId, name );
    loadPlaylists();
}

/*!
  \brief removePlaylist: the only operation here that can destroy the user's work

  A custom playlist is a list and nothing else: dropping it leaves every track
  in the catalogue. A torrent playlist is the torrent, so dropping it is a
  question about the torrent — and the answer depends on whether tracks of it
  have been put into lists made by hand, because those are not ours to throw
  away without asking.
*/
void MainWindow::removePlaylist()
{
    const qint64 playlistId = ui->playlistList->currentPlaylistId();
    if( playlistId <= 0 || !The::database()->isOpen() )
        return;

    const PlaylistStore::PlaylistInfo playlist = The::playlistStore()->playlist( playlistId );
    if( !playlist.isValid() )
        return;

    if( playlist.kind == PlaylistStore::CustomPlaylist ) {
        const int answer = QMessageBox::question( this,
                                                  tr( "Remove playlist" ),
                                                  tr( "Remove the playlist «%1»?" ).arg( playlist.name )
                                                      + QLatin1String( "\n\n" )
                                                      + tr( "The tracks themselves stay where they are: "
                                                            "this only removes the list." ),
                                                  QMessageBox::Yes | QMessageBox::No,
                                                  QMessageBox::No );

        if( answer != QMessageBox::Yes )
            return;

        The::playlistStore()->removeCustomPlaylist( playlistId );

        loadPlaylists();
        loadCurrentPlaylist();
        return;
    }

    const QString infoHash = playlist.infoHash;
    const QList<Catalog::PlaylistReference> references = The::catalog()->customPlaylistReferences( infoHash );

    if( references.isEmpty() ) {
        const int answer = QMessageBox::question( this,
                                                  tr( "Remove torrent" ),
                                                  tr( "Remove «%1»?" ).arg( playlist.name )
                                                      + QLatin1String( "\n\n" )
                                                      + tr( "Its tracks and its downloaded data go with it. "
                                                            "Adding the magnet again brings it back." ),
                                                  QMessageBox::Yes | QMessageBox::No,
                                                  QMessageBox::No );

        if( answer != QMessageBox::Yes )
            return;

        removeTorrentEverywhere( infoHash );

        loadPlaylists();
        loadCurrentPlaylist();
        return;
    }

    // Somebody put tracks of this torrent into lists of their own. Saying how
    // many and which is the whole point of this dialog: without the numbers
    // there is no way to choose between the two ways that are not Cancel.
    int trackCount = 0;
    QStringList names;
    for( const Catalog::PlaylistReference &reference : references ) {
        trackCount += reference.trackCount;
        names << reference.name;
    }

    QMessageBox box( this );
    box.setIcon( QMessageBox::Warning );
    box.setWindowTitle( tr( "Remove torrent" ) );
    box.setText( tr( "Remove «%1»?" ).arg( playlist.name ) );
    box.setInformativeText( tr( "%n track(s) of this torrent are used in your own playlists: %1.",
                                "", trackCount ).arg( names.join( QLatin1String( ", " ) ) ) );

    QPushButton *removeAll = box.addButton( tr( "Remove everything" ), QMessageBox::DestructiveRole );
    QPushButton *keepTracks = box.addButton( tr( "Keep those tracks" ), QMessageBox::AcceptRole );
    box.addButton( QMessageBox::Cancel );
    box.setDefaultButton( keepTracks );

    box.exec();

    if( box.clickedButton() == removeAll ) {
        removeTorrentEverywhere( infoHash );
    } else if( box.clickedButton() == keepTracks ) {
        // Hidden, not removed: the torrent leaves the sidebar, its tracks stay
        // where they were put and go on playing, and its data follow the
        // ordinary rules of the cache. Adding the same magnet again brings it
        // back with the selection and the exclusions it had.
        The::catalog()->hideTorrent( infoHash );
    } else {
        return;
    }

    loadPlaylists();
    loadCurrentPlaylist();
}

void MainWindow::removeTorrentEverywhere( const QString &infoHash )
{
    // Removing from the catalogue, which is not evicting from the cache: this
    // one takes the rows with it, on purpose, because it is what was asked for.
    const PlaylistItemPtr current = The::audioEngine()->currentTrack();
    if( current && current->track() && current->track()->infoHash() == infoHash ) {
        The::audioEngine()->stop();
        The::cacheManager()->setPlayingTorrent( QString() );
    }

    The::torrentSession()->remove( infoHash, true );
    The::catalog()->removeTorrent( infoHash );
}

void MainWindow::showPlaylistMenu( const QPoint &position )
{
    const qint64 playlistId = ui->playlistList->playlistAt( position );

    // The menu acts on the row it was opened over, so that row becomes the
    // current one first: otherwise a right click somewhere and a Remove would
    // take away whatever happened to be selected before.
    if( playlistId > 0 )
        ui->playlistList->selectPlaylist( playlistId );

    ActionCollection *ac = The::actionCollection();

    QMenu menu( this );
    menu.addAction( ac->getAction( "actionNewPlaylist" ) );

    if( playlistId > 0 ) {
        menu.addSeparator();
        menu.addAction( ac->getAction( "actionRenamePlaylist" ) );
        menu.addAction( ac->getAction( "actionRemovePlaylist" ) );
    }

    menu.exec( ui->playlistList->mapToGlobal( position ) );
}

void MainWindow::addTracksToPlaylist( qint64 playlistId, const QList<qint64> &trackIds )
{
    if( playlistId <= 0 || trackIds.isEmpty() || !The::database()->isOpen() )
        return;

    const PlaylistStore::PlaylistInfo playlist = The::playlistStore()->playlist( playlistId );
    if( !playlist.isValid() || playlist.kind != PlaylistStore::CustomPlaylist )
        return;

    QList<TrackInfoPtr> tracks;
    for( qint64 trackId : trackIds ) {
        const TrackInfoPtr track = The::catalog()->track( trackId );
        if( track )
            tracks << track;
    }

    if( tracks.isEmpty() || !The::playlistStore()->appendTracks( playlistId, tracks ) )
        return;

    loadPlaylists();

    if( playlistId == The::playlistStore()->currentPlaylistId() )
        loadCurrentPlaylist();

    m_playerWidget->showMessage( tr( "%n track(s) added to «%1».", "", tracks.count() )
                                 .arg( playlist.name ) );
}

void MainWindow::addTrackToPlaylist( DownloadItem *item, qint64 playlistId )
{
    if( !item || !item->playlistItem()->track() )
        return;

    addTracksToPlaylist( playlistId, { item->playlistItem()->track()->id() } );
}

/*!
  \brief reorderTracks: a drag inside the list is an order, not a move

  The rows are not moved by the view, and that is the point: each carries a
  widget put there with setItemWidget, which a view that removes and reinserts
  rows knows nothing about. The new order is written and the list rebuilt from
  it, through the same path that builds it in the first place.
*/
void MainWindow::reorderTracks( const QList<int> &rows, int targetRow )
{
    const qint64 playlistId = The::playlistStore()->currentPlaylistId();
    if( playlistId <= 0 || rows.isEmpty() )
        return;

    QList<TrackInfoPtr> tracks;
    for( int i = 0; i < ui->downloadList->count(); ++i ) {
        DownloadItem *item = qobject_cast<DownloadItem *>( ui->downloadList->itemWidget( ui->downloadList->item( i ) ) );
        if( !item || !item->playlistItem()->track() )
            return;

        tracks << item->playlistItem()->track();
    }

    const QList<int> order = Utility::reorderedIndexes( tracks.count(), rows, targetRow );
    if( order.count() != tracks.count() )
        return;

    QList<TrackInfoPtr> reordered;
    for( int index : order )
        reordered << tracks.at( index );

    The::playlistStore()->setTracks( playlistId, reordered );

    loadCurrentPlaylist();
}

bool MainWindow::openPlaylistIsCustom() const
{
    const PlaylistStore::PlaylistInfo playlist =
        The::playlistStore()->playlist( The::playlistStore()->currentPlaylistId() );

    return playlist.isValid() && playlist.kind == PlaylistStore::CustomPlaylist;
}

void MainWindow::savePlaybackState()
{
    if( !The::database()->isOpen() )
        return;

    const PlaylistItemPtr &current = The::audioEngine()->currentTrack();

    // Nothing playing is not the same as nothing to remember, and writing a
    // zero here would be the second: the track of the last session comes back
    // selected and paused, so closing the window without pressing play would
    // throw away the very thing that had just been restored. One restart later
    // there would be nothing left to come back to.
    //
    // What does forget the track is opening another playlist, which says so
    // itself by writing the new playlist with no track in it.
    if( !current || !current->track() )
        return;

    The::playlistStore()->setPlaybackState( The::playlistStore()->currentPlaylistId(),
                                            current->track()->id(),
                                            The::audioEngine()->currentTime() );
}

void MainWindow::addPlaylistItem( const PlaylistItemPtr &playlistItem )
{
    DownloadItem *item = new DownloadItem( playlistItem, this );

    connect( item,             &DownloadItem::reloadPlaylist,
             ui->downloadList, &DownloadList::reloadPlaylist );

    connect( item,             &DownloadItem::removeMe,
             ui->downloadList, &DownloadList::removeItem );

    // After the list has taken the row out, not before: what is written is
    // what is on screen.
    connect( item, &DownloadItem::removeMe, this, &MainWindow::savePlaylistOrder );

    connect( item, &DownloadItem::excludeRequested, this, &MainWindow::excludeTrack );

    connect( item, &DownloadItem::addToPlaylistRequested, this, &MainWindow::addTrackToPlaylist );

    item->setInCustomPlaylist( openPlaylistIsCustom() );

    QListWidgetItem *wi = new QListWidgetItem;

    // The row carries the id as well as the widget: a drag leaves the widget
    // behind and takes the item's data with it.
    if( playlistItem->track() )
        wi->setData( SongRoles::TrackId, playlistItem->track()->id() );

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
        restoreSplitterSizes();

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
        restoreSplitterSizes();

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
    The::downloadQueue()->reloadSettings();
    The::downloadQueue()->refresh();
}

void MainWindow::onlineStateChanged( bool isOnline )
{
    if( isOnline ) {
        m_playerWidget->showMessage( tr( "Idle" ) );
    } else {
        // Tracks already in the cache still play: only reaching new peers needs
        // the network.
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

    ui->compactMenuButton->setFixedSize( QSize( side, side ) );
    ui->spinnerWidget->setFixedSize( QSize( side, side ) );
    QWidget::resizeEvent( event );
}
