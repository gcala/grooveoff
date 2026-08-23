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
#include "TrackRow.h"
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

    // Before the sidebar is built, because the rows of the magnets still being
    // looked for are laid on top of it from the map this fills.
    resumeMagnetSearches();

    loadPlaylists();
    loadCurrentPlaylist();
    restoreWhatWasPlaying();
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

    // Before the settings, and while the database is still open: App tears the
    // singletons down after this window, persistence last.
    rememberPendingMagnets();

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
    connect( ac->getAction( "actionAbout" ), &QAction::triggered, this, &MainWindow::about );
    connect( ac->getAction( "actionOpenTorrent" ), &QAction::triggered, this, &MainWindow::openTorrentFile );
    connect( ac->getAction( "actionEmptyCache" ), &QAction::triggered, this, &MainWindow::emptyCache );
    connect( ac->getAction( "actionNewPlaylist" ), &QAction::triggered, this, &MainWindow::newPlaylist );
    connect( ac->getAction( "actionRenamePlaylist" ), &QAction::triggered, this, &MainWindow::renamePlaylist );
    connect( ac->getAction( "actionChooseTracks" ), &QAction::triggered, this, &MainWindow::chooseTracksAgain );
    connect( ac->getAction( "actionEmptyPlaylist" ), &QAction::triggered, this, &MainWindow::emptyPlaylist );
    connect( ac->getAction( "actionRemovePlaylist" ), &QAction::triggered, this, &MainWindow::removePlaylist );

    // One connection for a submenu whose entries are made and unmade at every
    // change of row: which list was picked is in the action's own data, so
    // nothing here has to survive the rebuild.
    connect( ac->getMenu( "addToPlaylistMenu" ), &QMenu::triggered, this,
             &MainWindow::addShownTorrentToPlaylist );

    // currentItemChanged rather than itemClicked, so that the keyboard opens a
    // playlist too. Selecting a row from the code blocks the signal, or every
    // rebuild of the sidebar would reopen the list that is already open.
    connect( ui->playlistList, &QListWidget::currentItemChanged, this,
             [this]( QListWidgetItem *current, QListWidgetItem * ) {
        openPlaylist( current );
        updatePlaylistActions();
    } );

    connect( ui->playlistList, &QWidget::customContextMenuRequested, this, &MainWindow::showPlaylistMenu );
    connect( ui->playlistList, &PlaylistList::tracksDropped, this, &MainWindow::addTracksToPlaylist );

    connect( ui->trackList, &TrackList::reorderRequested, this, &MainWindow::reorderTracks );

    connect( ui->searchLine, &QLineEdit::returnPressed, this, &MainWindow::addFromMagnetField );

    // Which playlist the music is coming from is decided when a track is asked
    // for and not when it starts: between the two there is the wait for the
    // head of the file, and somebody who presses play and goes looking through
    // another playlist while it buffers would otherwise have moved on by the
    // time anybody asked where the track came from.
    connect( The::audioEngine(), &AudioEngine::trackRequested, this,
             [this]( const PlaylistItemPtr &track, bool chosen ) {
        // The queue advancing, or the track left over from last time being
        // resumed: neither is somebody choosing, so the mark stays where it is
        // rather than walking onto whatever playlist happens to be on screen.
        //
        // This used to ask whether the track was already in the queue, which is
        // the same answer most of the time and the wrong one exactly when it
        // matters: a track clicked in a list made by hand while that very track
        // was playing from its torrent's playlist is in the queue, and removing
        // it afterwards carried on with the next track of the other playlist.
        if( !chosen )
            return;

        if( !track || !track->track() || !rowFor( track->track() ) || shownPlaylistId() <= 0 )
            return;

        m_playingPlaylistId = shownPlaylistId();
        rebuildPlayQueue();
        updatePlayingMark();
    } );

    // And the mark comes off by itself when the music ends. Nobody chooses that
    // moment — the last track of the playlist simply finishes — so there is no
    // gesture to hang it on, only the state the player ends up in.
    connect( The::audioEngine(), &AudioEngine::stateChanged, this,
             [this]( GrooveOff::PlayerState ) { updatePlayingMark(); } );

    connect( The::audioEngine(), &AudioEngine::nothingToPlay, this, &MainWindow::playShownPlaylist );

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

    // And when the queue is allowed to fetch again — room in the cache, or the
    // setting switched on — the torrents it would fetch from have to be in the
    // session to hear about it. Nothing else would put them there: the sidebar
    // is not rebuilt by a cache dropping under its ceiling.
    connect( The::downloadQueue(), &DownloadQueue::fetchingResumed, this,
             &MainWindow::wakeUnfinishedTorrents );

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
            if( TrackRow *previous = rowFor( m_playingTrack ) ) {
                previous->setTrackState( The::torrentSession()->hasMetadata( m_playingTrack->infoHash() )
                                         ? GrooveOff::ReadyState
                                         : GrooveOff::DormantState );
            }
        }

        m_playingTrack = current->track();

        TrackRow *item = rowFor( current->track() );
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

        TrackRow *item = rowFor( current->track() );
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

qint64 MainWindow::shownPlaylistId() const
{
    return ui->playlistList->currentPlaylistId();
}

/*!
  \brief rebuildPlayQueue: what the engine will walk when this track ends

  Not what is on screen. Somebody who starts a playlist and then goes looking
  through the others for something to add to it wants the first one to go on,
  track after track, including the ones added while it played — which is the
  whole reason this is a list of its own.
*/
/*!
  \brief playShownPlaylist: what pressing play means when nothing is playing

  The playlist being looked at, from its first track. The engine asks rather
  than decides, because the whole point of the split between the playlist shown
  and the one playing is that it does not know about the first — and here the
  two become the same one, which is right: choosing is exactly what this is.

  Nothing else has to be told. The track is not in the queue, so playItem()
  goes through trackRequested() like any other choice, and that is where the
  playing playlist, the queue and the mark are all decided.

  A list with no rows leaves the button doing nothing, and there is nothing
  else it could do: an empty playlist has no first track.
*/
void MainWindow::playShownPlaylist()
{
    if( ui->trackList->count() == 0 )
        return;

    if( TrackRow *first = qobject_cast<TrackRow *>( ui->trackList->itemWidget( ui->trackList->item( 0 ) ) ) )
        The::audioEngine()->playItem( first->playlistItem() );
}

/*!
  \brief restoreWhatWasPlaying: the queue to walk and the point to carry on from

  Selecting the row was all that came back, and it was not enough for either
  half of carrying on. The queue is built from the playlist playback is in, and
  at start up nothing was playing, so it was empty: the play button of the
  player bar — and MPRIS with it — had nothing to walk and did nothing at all,
  in silence. The position, meanwhile, was written at every close and read by
  nobody, so whatever did start, started from its beginning.

  Both answers were in app_state already. This runs from the constructor and
  nowhere else: the row is selected in loadCurrentPlaylist(), which runs at
  every playlist opened, and doing this there would make looking at a list that
  happens to hold the remembered track an act of choosing it.

  Nothing is marked as playing on the sidebar, and nothing should be: this is
  what will play if somebody presses play, which is not the same as playing.
*/
void MainWindow::restoreWhatWasPlaying()
{
    QSettings settings;
    if( !settings.value( Settings::restorePlayback, Settings::restorePlaybackDefault ).toBool() )
        return;

    PlaylistStore *playlists = The::playlistStore();

    const qint64 playlistId = playlists->currentPlaylistId();
    const qint64 trackId    = playlists->currentTrackId();

    if( playlistId <= 0 || trackId <= 0 )
        return;

    m_playingPlaylistId = playlistId;
    rebuildPlayQueue();

    // The item out of the queue rather than a fresh one over the same track:
    // it is the one the row on screen is showing, and the engine says what it
    // is doing by setting state on the item it was handed. A copy here is a
    // row that sits still while the music comes out of it.
    for( int i = 0; i < The::playlist()->count(); ++i ) {
        const PlaylistItemPtr item = The::playlist()->item( i );

        if( item && item->track() && item->track()->id() == trackId ) {
            The::audioEngine()->setResumePoint( item, playlists->currentPositionMs() );

            // And the fetching starts from where the listening stopped rather
            // than from the top of the list. The cursor is otherwise moved only
            // by a track actually starting, which after a restart is a moment
            // that may never come: a collection left at its hundred and fiftieth
            // track would have been refilled from the first, and the cursor
            // would have jumped to the right place at the one moment it is no
            // longer needed — the play that asks for that track directly.
            //
            // It takes top priority within its torrent, which is the same trade
            // wakeLastUsedTorrent() makes one line further down: this is the
            // track somebody is going to press play on.
            The::downloadQueue()->setPlaying( item->track()->infoHash(),
                                              item->track()->fileIndex() );
            return;
        }
    }
}

/*!
  \brief updatePlayingMark: whether the sidebar says where the music comes from

  The mark answers "why is this playing, when I am looking at something else?",
  so it belongs to a playlist exactly while something is coming out of it. Which
  playlist that is stays known after the music stops — a finished playlist is
  still the one to come back to at the next start — so the mark cannot be read
  off m_playingPlaylistId alone, and left to it the mark stayed on a playlist
  that had played itself out.

  The rule is the one a track row already follows, deliberately: Stopped puts
  the play button back, and everything else keeps the mark, because paused,
  buffering, or stopped on an error the player still holds that track and
  pressing play goes on with it. Two marks about the same thing that disagreed
  would be worse than either.

  Called at every change of state and not only when play is pressed: reaching
  the end of a playlist is a moment nobody chose, and it is the moment this
  exists for.
*/
void MainWindow::updatePlayingMark()
{
    const bool coming = The::audioEngine()->state() != GrooveOff::PlayerState::Stopped;

    ui->playlistList->setPlayingPlaylist( coming ? m_playingPlaylistId : 0 );
}

void MainWindow::rebuildPlayQueue()
{
    The::playlist()->clear();

    if( m_playingPlaylistId <= 0 || !The::database()->isOpen() )
        return;

    const PlaylistItemPtr current = The::audioEngine()->currentTrack();
    const QList<TrackInfoPtr> tracks = The::playlistStore()->tracks( m_playingPlaylistId );

    for( const TrackInfoPtr &track : tracks ) {
        // The engine's own item for the track being played, first of all: next()
        // finds where it is by identity, and the same track can have a row in
        // another playlist on screen — two playlists holding one track is the
        // point of having playlists — so asking the rows first would put a
        // stranger in the queue where the engine expected itself.
        if( current && current->track()
            && current->track()->infoHash() == track->infoHash()
            && current->track()->fileIndex() == track->fileIndex() ) {
            The::playlist()->appendItem( current );
            continue;
        }

        // Then the row on screen when there is one, so that it goes on showing
        // what the engine is doing; a fresh item otherwise, for a playlist
        // nobody is looking at.
        if( TrackRow *row = rowFor( track ) ) {
            The::playlist()->appendItem( row->playlistItem() );
            continue;
        }

        The::playlist()->appendItem( PlaylistItemPtr( new PlaylistItem( track ) ) );
    }
}

void MainWindow::loadCurrentPlaylist()
{
    if( !The::database()->isOpen() )
        return;

    PlaylistStore *playlists = The::playlistStore();

    // The playlist on screen, from the sidebar. It used to come from app_state,
    // which now holds the one being played instead — and asking that one which
    // list to draw is how looking at a second playlist drew the first.
    const qint64 playlistId = shownPlaylistId();
    m_shownPlaylistId = playlistId;

    // disable temporarily AutoScroll
    ui->trackList->setAutoScroll( false );
    ui->trackList->releaseRows();

    // No playlist open is a state to be shown, not a reason to do nothing.
    // Returning here before emptying the list is what left the tracks of the
    // last playlist on screen after it was removed: the sidebar was empty, the
    // rows were still there, and they went only at the next start — which reads
    // as a removal that did not work rather than one that did.
    if( playlistId <= 0 ) {
        ui->trackList->setAutoScroll( true );
        ui->trackList->reloadPlaylist();
        m_playerWidget->showMessage( tr( "Idle" ) );
        return;
    }

    // The track being played keeps the item the engine is holding, and does not
    // get a fresh one. A row shows that it is playing by listening to its own
    // PlaylistItem, and the engine announces on the one it was given: two
    // objects for one track means a row that sits still while the music comes
    // out of it. Rebuilding the list is frequent — adding a track, coming back
    // from another playlist, reordering — so a copy here is a copy for good.
    const PlaylistItemPtr playing = The::audioEngine()->currentTrack();

    const QList<TrackInfoPtr> &tracks = playlists->tracks( playlistId );
    for( const TrackInfoPtr &track : tracks ) {
        if( playing && playing->track()
            && playing->track()->infoHash() == track->infoHash()
            && playing->track()->fileIndex() == track->fileIndex() ) {
            addPlaylistItem( playing );
            continue;
        }

        addPlaylistItem( PlaylistItemPtr( new PlaylistItem( track ) ) );
    }

    ui->trackList->setAutoScroll( true );
    ui->trackList->reloadPlaylist();

    // The rows are new objects, so the queue is built again over them: a row
    // that is the track being played has to be the engine's own item, or the
    // list will look idle while the music comes out of it.
    rebuildPlayQueue();

    // Once here as well, and not only with the statistics: a torrent whose
    // files are already in the cache is asleep, so no statistics arrive for it
    // and nothing would ever ask. Its rows would show file names until
    // somebody pressed play on each of them — which is the behaviour this is
    // meant to be the end of.
    //
    // Behind the list rather than in front of it: this opens every file of the
    // playlist to read its tags, which at start up is a quarter of a second
    // spent before there is anything on screen. A moment later the rows say the
    // same thing and the window is already there — and switching playlists
    // stops paying it in the middle of the switch.
    QTimer::singleShot( 0, this, [this] { readArrivedTags(); } );

    // The track that was playing is selected but not started: pressing play
    // picks it up where it was left.
    QSettings settings;
    if( !settings.value( Settings::restorePlayback, Settings::restorePlaybackDefault ).toBool() )
        return;

    const qint64 trackId = playlists->currentTrackId();
    if( trackId <= 0 )
        return;

    for( int i = 0; i < ui->trackList->count(); ++i ) {
        TrackRow *item = qobject_cast<TrackRow *>( ui->trackList->itemWidget( ui->trackList->item( i ) ) );
        if( item && item->playlistItem()->track()->id() == trackId ) {
            ui->trackList->setCurrentRow( i );
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
    //
    // And nothing worth announcing either when the track is whole in the cache:
    // then pressing play reads it off the disk and the torrent is never needed,
    // so waking it here would be an announcement made in advance of a use that
    // is not going to come.
    if( track && !track->infoHash().isEmpty()
        && settings.value( Settings::restorePlayback, Settings::restorePlaybackDefault ).toBool()
        && !The::torrentSession()->cachedFile( track->infoHash(), track->fileIndex() ).complete ) {
        QString error;
        if( !The::torrentSession()->reopen( track->infoHash(), track->magnetUri(), &error ) )
            qDebug() << "GrooveOff ::" << "cannot reopen the last used torrent:" << error;
    }

    // And the ones with music still to fetch, which asks for itself whether
    // there is anything to be gained by waking them. The question used to be
    // asked here instead, and being asked here was the same as not being asked
    // at all: loadPlaylists() runs three lines before this in the constructor
    // and calls the same function, so with the fetching switched off every
    // unfinished torrent had already announced itself by the time the check was
    // reached.
    wakeUnfinishedTorrents();
}

void MainWindow::wakeUnfinishedTorrents()
{
    // Waking a torrent is an announcement — to a tracker, to the dht, to
    // whatever peers answer — and the only thing it buys here is the fetching
    // ahead. So the two questions that decide whether a byte would be asked for
    // are asked first, and they are asked here rather than at the three call
    // sites: a guard living in one of them is a guard the other two walk past.
    //
    // Switched off, this is the setting doing what it promises. Near the
    // ceiling, DownloadQueue hands out SkipFile to everything nobody is
    // listening to, so a torrent woken now would announce itself and then ask
    // for nothing at all.
    if( !The::downloadQueue()->isEnabled() || !The::cacheManager()->hasRoomForPrefetch() )
        return;

    // The way back in is DownloadQueue::fetchingResumed, connected in
    // setupConnections(): both of those answers change on their own, with no
    // gesture behind them, and a torrent asleep when they changed would
    // otherwise stay asleep until something happened to rebuild the sidebar.
    //
    // The opposite edge is deliberately not handled. A torrent already awake
    // when the room runs out goes on announcing while fetching nothing, and
    // putting it back to sleep means taking it out of the session — with the
    // torrent being listened to, the one being brought up and the one the
    // stream server is reading from all to be spared. That is a real risk to
    // the music in exchange for an announcement that has already been made.
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

        // Rough on purpose: bytes_done counts every piece the torrent holds,
        // including the ones shared with files nobody chose, so a torrent with
        // enough bytes to cover the chosen tracks is probably done with them.
        //
        // Which way it is rough depends on the selection, and that is worth
        // knowing before trusting it. With everything chosen it cannot overstate
        // anything — the count cannot pass the sum of the files — so equality
        // means complete. With a partial selection it can: twenty tracks of two
        // hundred, nineteen of them fetched plus the pieces they share with
        // their neighbours, and the count reaches the total with a track still
        // missing. That torrent is not woken and its last track waits to be
        // asked for. The exact answer exists — cachedFile() reads the piece map
        // — but it costs one read per track at every rebuild of the sidebar,
        // which is the cheap operation multiplied by the number of tracks that
        // has frozen this window twice already.
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

    // Pasting again a magnet already being looked for leaves the entry alone.
    // Replacing it would put the question back on a magnet that has been asked
    // about and answered, which is the one thing asked() exists to prevent.
    if( !m_awaitingSelection.contains( infoHash ) ) {
        PendingAdd pending;
        pending.name      = Utility::magnetDisplayName( source );
        pending.magnetUri = The::torrentSession()->magnetUri( infoHash );
        m_awaitingSelection.insert( infoHash, pending );
    }

    const PendingAdd pending = m_awaitingSelection.value( infoHash );

    // A torrent whose file list is already in hand posts no alert, so the
    // selector has to be opened here instead of waiting for one. That is every
    // .torrent file and every magnet of a torrent already awake, and none of
    // them ever waits for anything.
    if( The::torrentSession()->hasMetadata( infoHash ) ) {
        m_awaitingSelection.remove( infoHash );
        m_playerWidget->showMessage( tr( "Reading the track list…" ) );
        QTimer::singleShot( 0, this, [this, infoHash] { chooseTracksOf( infoHash, true ); } );
        return;
    }

    // The wait that follows is up to a minute long, and it goes in the sidebar
    // rather than in the bar of the player: a message there costs three seconds
    // and then gives the bar back to whoever is listening, which is the right
    // shape for acknowledging a gesture and the wrong one for a state. The row
    // stays for as long as the looking does.
    //
    // Unless the sidebar already has a row for this torrent — the same magnet
    // pasted twice, or one the catalogue knows and nobody has woken. Then there
    // is already something on screen standing for it.
    if( !ui->playlistList->hasRow( infoHash ) )
        ui->playlistList->addPending( infoHash, pendingName( infoHash ) );

    m_playerWidget->showMessage( pending.name.isEmpty()
                                 ? tr( "Reading the track list…" )
                                 : tr( "Looking for the track list of «%1»…" ).arg( pending.name ) );
}

/*!
  \brief pendingName: what to call a torrent that has not said its name yet

  The magnet's own dn field, which is there nearly always, and something
  neutral when it is not. Never the info hash: forty hexadecimal digits are an
  identifier for us and noise for somebody listening to music.
*/
QString MainWindow::pendingName( const QString &infoHash ) const
{
    const QString name = m_awaitingSelection.value( infoHash ).name;

    return name.isEmpty() ? tr( "New torrent" ) : name;
}

/*!
  \brief addPendingRows: puts the provisional rows back after a rebuild

  loadPlaylists() empties the sidebar and fills it from a query, and a magnet
  still being looked for is in no query — it is in memory and nowhere else. So
  the rows have to be laid back on top, and they go last: the fallback that
  opens the first row when nothing is open must keep landing on a real playlist.
*/
void MainWindow::addPendingRows()
{
    for( auto it = m_awaitingSelection.constBegin(); it != m_awaitingSelection.constEnd(); ++it ) {
        if( ui->playlistList->hasRow( it.key() ) )
            continue;

        ui->playlistList->addPending( it.key(), pendingName( it.key() ) );
        ui->playlistList->setPendingSearching( it.key(), it.value().searching );
    }
}

void MainWindow::resumeMagnetSearches()
{
    if( !The::database()->isOpen() )
        return;

    const QList<Catalog::PendingMagnet> magnets = The::catalog()->magnetsWithoutMetadata();
    for( const Catalog::PendingMagnet &magnet : magnets ) {
        PendingAdd pending;
        pending.name      = magnet.name;
        pending.magnetUri = magnet.magnetUri;

        // Looking again from scratch, so the led blinks again — a new attempt
        // is genuinely under way and saying otherwise would be showing the
        // result of the last one. What does not come back is the question.
        pending.asked = true;

        m_awaitingSelection.insert( magnet.infoHash, pending );

        QString error;
        if( !The::torrentSession()->reopen( magnet.infoHash, magnet.magnetUri, &error ) )
            qDebug() << "GrooveOff ::" << "could not look for" << magnet.name << "again:" << error;
    }
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

    // Read and dropped before the sidebar is rebuilt, and both halves of that
    // matter. The provisional row is put back from this very map, so an entry
    // left in it would have the row reappear beside the real one and never go
    // away again; and the answer is needed further down, after the rebuild.
    const bool wasAdded = m_awaitingSelection.contains( infoHash );
    if( wasAdded ) {
        // The question of the timeout, if it is still on screen, is about a
        // torrent that has just answered. Left there, its «Remove» would throw
        // away a torrent whose tracks are being chosen in the selector.
        if( QMessageBox *question = m_awaitingSelection.value( infoHash ).question )
            question->close();

        m_awaitingSelection.remove( infoHash );
    }

    Catalog *catalog = The::catalog();

    catalog->addTorrent( infoHash, The::torrentSession()->magnetUri( infoHash ), name );
    catalog->setName( infoHash, name );
    catalog->addTracks( tracks );
    catalog->touch( infoHash );

    // A torrent that was hidden and has just been added again is back in the
    // sidebar, and one added for the first time appears there now.
    loadPlaylists();
    refreshRowsOf( infoHash );

    if( !wasAdded ) {
        // A torrent woken up rather than added: it keeps the playlist it had
        // and does not steal the view.
        //
        // It does not steal the bar either, unless there is nothing in it. A
        // message covers the player for three seconds, and nobody asked for
        // this one — the torrent was woken by the start up, by the fetching
        // ahead or by a play, and a name landing on top of the track being
        // started answers a question nobody put. At start up it was worse than
        // noise: the bar is filled with the restored track a quarter of a
        // second in, and every torrent woken behind it pushed those three
        // seconds further out, so the whole thing opened blank for as long as
        // there were torrents to wake.
        if( !The::audioEngine()->currentTrack() )
            m_playerWidget->showMessage( name );
        return;
    }

    // Out of the alert handler before opening a modal: the dialog spins its own
    // event loop, and doing that in the middle of draining libtorrent's alerts
    // is asking for trouble that only shows up under load.
    QTimer::singleShot( 0, this, [this, infoHash] { chooseTracksOf( infoHash, true ); } );
}

/*!
  \brief chooseTracksAgain: the selector, on a torrent that is already here

  Two things it is for: putting back a track taken out of the playlist by
  mistake, and taking more of a torrent once its album turns out to be worth
  it. Neither needs the network — what the selector shows comes from the
  catalogue, so a torrent fast asleep answers as well as one in the session.
*/
void MainWindow::chooseTracksAgain()
{
    const qint64 playlistId = ui->playlistList->currentPlaylistId();
    if( playlistId <= 0 || !The::database()->isOpen() )
        return;

    const PlaylistStore::PlaylistInfo playlist = The::playlistStore()->playlist( playlistId );
    if( playlist.kind != PlaylistStore::TorrentPlaylist || playlist.infoHash.isEmpty() )
        return;

    chooseTracksOf( playlist.infoHash );
}

void MainWindow::chooseTracksOf( const QString &infoHash, bool justAdded )
{
    Catalog *catalog = The::catalog();
    PlaylistStore *playlists = The::playlistStore();

    const qint64 playlistId = playlists->torrentPlaylistId( infoHash );
    if( playlistId <= 0 )
        return;

    // Every file, non audio included: hiding them here is what makes a missing
    // track impossible to explain.
    const QList<TrackInfoPtr> all = catalog->tracks( infoHash, false );

    QList<qint64> chosen;
    for( const TrackInfoPtr &track : playlists->tracks( playlistId ) )
        chosen << track->id();

    TrackSelector selector( catalog->name( infoHash ), all, chosen, this );

    if( selector.exec() != QDialog::Accepted ) {
        // Nothing was taken, so nothing should be showing. The playlist of a
        // torrent is created with the torrent — that is where the selection
        // lives — so cancelling used to leave an empty row in the sidebar,
        // which reads as a torrent that failed rather than one that was
        // declined.
        //
        // Hidden and not removed, which is the distinction this program
        // already makes elsewhere: the file list, the exclusions and whatever
        // had been chosen before all stay, and pasting the same magnet again
        // brings the torrent back with them instead of asking the swarm for
        // its metadata a second time. With nothing queued there is also
        // nothing worth announcing, so it leaves the session as well.
        // And only for the selector that follows an add. Reopening it on a
        // torrent that is already in the sidebar and then changing one's mind
        // must leave it exactly where it was — even when it is empty, because
        // emptying it was somebody's decision too.
        if( justAdded && playlists->tracks( playlistId ).isEmpty() ) {
            The::torrentSession()->remove( infoHash, false );
            playlists->removeTorrentPlaylist( infoHash );

            loadPlaylists();
            loadCurrentPlaylist();

            m_playerWidget->showMessage( tr( "Nothing chosen from «%1»: paste the magnet again to pick tracks." )
                                         .arg( catalog->name( infoHash ) ) );
            return;
        }

        m_playerWidget->showMessage( catalog->name( infoHash ) );
        return;
    }

    // Unticking is the same act as the bin, so it costs the same: what comes
    // out of the playlist comes out of every list and its bytes go with it.
    // The alternative — leaving the bytes because the file was unticked rather
    // than binned — would make the same outcome depend on which of two doors
    // somebody walked through.
    const QList<TrackInfoPtr> previous = playlists->tracks( playlistId );
    const QList<TrackInfoPtr> selected = selector.selectedTracks();

    QSet<qint64> keptIds;
    for( const TrackInfoPtr &track : selected )
        keptIds.insert( track->id() );

    QList<TrackInfoPtr> removed;
    for( const TrackInfoPtr &track : previous ) {
        if( !keptIds.contains( track->id() ) )
            removed << track;
    }

    // One question for the lot of them, and Cancel abandons the whole
    // confirmation: it was one gesture, so half of it is not an outcome
    // anybody asked for.
    if( !removed.isEmpty() && !confirmDiscard( removed ) )
        return;

    // What was already in the playlist keeps the order it was put in, and what
    // has just been ticked is dropped in where its place in the album says.
    // Handing setTracks() the selector's own order instead would quietly
    // rearrange a playlist somebody had dragged into shape, and would send a
    // track taken out by mistake to the end of the list rather than back where
    // it came from — which is one of the two things this dialog is for.
    const QList<TrackInfoPtr> ordered = Utility::mergedSelection( previous, selected );

    playlists->setTracks( playlistId, ordered );
    playlists->setPlaybackState( playlistId, 0, 0 );

    // The rest of what unticking means. setTracks() has already taken them out
    // of this playlist, so what is left is the other lists and the bytes.
    if( !removed.isEmpty() ) {
        QList<PlaylistItemPtr> playing;
        for( const TrackInfoPtr &track : removed ) {
            if( TrackRow *row = rowFor( track ) )
                playing << row->playlistItem();
        }

        detachFromPlayer( playing );
        discardTracks( removed );
    }

    loadPlaylists();
    loadCurrentPlaylist();
    refreshRowsOf( infoHash );

    // Whatever happens, the message stops saying the track list is being read:
    // it was read, and leaving that sentence there is the interface claiming to
    // be busy with something it finished several clicks ago.
    m_playerWidget->showMessage( catalog->name( infoHash ) );
}

/*!
  \brief rowsForGesture: what one removal acts on

  There used to be two entries here, «Remove from the playlist» and «Exclude
  this track», with different consequences the next time round. They are one
  now: what a removal means is decided by the kind of list open, not by which
  of two words was clicked, and nothing is remembered about a refusal — a track
  taken out is a track not chosen, which is where every file starts.
*/
QList<TrackRow *> MainWindow::rowsForGesture( TrackRow *row ) const
{
    QList<TrackRow *> rows;
    if( !row )
        return rows;

    // The menu is raised by the row under the cursor and knows nothing about
    // what is highlighted, so the selection only counts when that row is in it.
    // A right click somewhere else means that row and not the ten elsewhere.
    const QList<QListWidgetItem *> selected = ui->trackList->selectedItems();

    bool inSelection = false;
    QList<TrackRow *> fromSelection;
    for( QListWidgetItem *item : selected ) {
        TrackRow *each = qobject_cast<TrackRow *>( ui->trackList->itemWidget( item ) );
        if( !each )
            continue;

        fromSelection << each;
        if( each == row )
            inSelection = true;
    }

    if( inSelection && fromSelection.count() > 1 )
        return fromSelection;

    rows << row;
    return rows;
}

bool MainWindow::confirmDiscard( const QList<TrackInfoPtr> &tracks )
{
    QList<qint64> ids;
    for( const TrackInfoPtr &track : tracks ) {
        if( track && track->id() > 0 )
            ids << track->id();
    }

    const QList<Catalog::PlaylistReference> holding = The::catalog()->customPlaylistsHolding( ids );
    if( holding.isEmpty() )
        return true;

    QStringList names;
    for( const Catalog::PlaylistReference &reference : holding )
        names << Utility::playlistName( reference.name, reference.pinned );

    QMessageBox box( this );
    box.setIcon( QMessageBox::Warning );
    box.setWindowTitle( tr( "Remove tracks" ) );
    box.setText( tr( "%n track(s) of this selection are in your own playlists: %1.", "", ids.count() )
                 .arg( names.join( QLatin1String( ", " ) ) ) );
    box.setInformativeText( tr( "Removing them takes them out of those too." ) );

    QPushButton *remove = box.addButton( tr( "Remove" ), QMessageBox::DestructiveRole );
    box.addButton( QMessageBox::Cancel );
    box.setDefaultButton( QMessageBox::Cancel );

    box.exec();

    return box.clickedButton() == remove;
}

void MainWindow::detachFromPlayer( const QList<PlaylistItemPtr> &items )
{
    const PlaylistItemPtr current = The::audioEngine()->currentTrack();
    PlaylistItemPtr playing;

    // The one being listened to goes last, and the order is the whole point:
    // removingTrack() stops the player and advances, and it advances through
    // the queue as it stands at that moment. Taking the others out first is
    // what stops it landing on a track that is going in the same gesture.
    for( const PlaylistItemPtr &item : items ) {
        if( !item )
            continue;

        if( item == current ) {
            playing = item;
            continue;
        }

        The::audioEngine()->removingTrack( item );
        The::playlist()->removeItem( item );
    }

    if( playing ) {
        The::audioEngine()->removingTrack( playing );
        The::playlist()->removeItem( playing );
    }
}

void MainWindow::discardTracks( const QList<TrackInfoPtr> &tracks )
{
    QList<qint64> ids;
    QHash<QString, QList<int>> byTorrent;

    for( const TrackInfoPtr &track : tracks ) {
        if( !track || track->id() <= 0 )
            continue;

        ids << track->id();
        byTorrent[track->infoHash()] << track->fileIndex();
    }

    if( ids.isEmpty() )
        return;

    // The rows first, the bytes after. A failure in between leaves bytes no
    // playlist points at, and those are still reachable — the torrent row is
    // untouched, so an eviction or «Empty the cache» still finds them. The
    // other order would leave a playlist pointing at music that has gone.
    The::playlistStore()->removeTracksEverywhere( ids );

    // What was playing may have been one of these, and app_state must not come
    // back at start up to a track that is in no list and has no bytes.
    if( ids.contains( The::playlistStore()->currentTrackId() ) )
        The::playlistStore()->setPlaybackState( m_playingPlaylistId, 0, 0 );

    for( auto torrent = byTorrent.constBegin(); torrent != byTorrent.constEnd(); ++torrent )
        The::cacheManager()->discardFiles( torrent.key(), torrent.value() );
}

void MainWindow::offerToRemoveEmptied( const QString &infoHash, const QString &name )
{
    const int answer = QMessageBox::question( this,
                                              tr( "Remove torrent" ),
                                              tr( "Nothing is left of «%1». Remove it?" ).arg( name ),
                                              QMessageBox::Yes | QMessageBox::No,
                                              QMessageBox::No );

    if( answer != QMessageBox::Yes )
        return;

    removeTorrentEverywhere( infoHash );

    loadPlaylists();
    loadCurrentPlaylist();
}

void MainWindow::removeTracksRequested( TrackRow *row )
{
    if( !row || !The::database()->isOpen() )
        return;

    const qint64 playlistId = shownPlaylistId();
    const PlaylistStore::PlaylistInfo playlist = The::playlistStore()->playlist( playlistId );
    if( !playlist.isValid() )
        return;

    QList<PlaylistItemPtr> items;
    QList<TrackInfoPtr>    tracks;
    QList<qint64>          ids;

    for( TrackRow *each : rowsForGesture( row ) ) {
        const PlaylistItemPtr item = each->playlistItem();
        if( !item || !item->track() )
            continue;

        items  << item;
        tracks << item->track();
        ids    << item->track()->id();
    }

    if( tracks.isEmpty() )
        return;

    const bool fromTorrent = playlist.kind == PlaylistStore::TorrentPlaylist;

    // Asked before anything is touched, so that Cancel really means nothing
    // happened rather than nothing more happened.
    if( fromTorrent && !confirmDiscard( tracks ) )
        return;

    detachFromPlayer( items );

    if( fromTorrent )
        discardTracks( tracks );
    else
        The::playlistStore()->removeTracks( playlistId, ids );

    loadPlaylists();
    loadCurrentPlaylist();

    // And a torrent playlist left with nothing in it is a question worth
    // asking: what is on the disk of that torrent is now nothing anybody chose.
    if( fromTorrent && The::playlistStore()->playlist( playlistId ).trackCount == 0 )
        offerToRemoveEmptied( playlist.infoHash, playlist.name );
}

void MainWindow::onTorrentStats( const QString &infoHash, int peers, int seeds, qint64 rate, qint64 bytesDone )
{
    Q_UNUSED( rate )

    The::catalog()->setBytesDone( infoHash, bytesDone );

    // Whatever has landed since the last second gets asked what it is.
    readArrivedTags( infoHash );

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

    TrackRow *item = rowFor( current->track() );
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
/*!
  \brief readArrivedTags: asks every row of this torrent whose file has landed

  Reading the tags used to hang off the track being prepared for the player,
  which was right when a file only ever arrived because somebody had pressed
  play on it. Now the whole playlist is fetched whether or not anybody presses
  anything, so a row would sit there showing a file name — track number, dashes
  and extension — beside a file that has had its title in it for a minute.

  Asking costs four bytes per row: TrackTags::hasHead() looks at the signature
  and nothing else, and a file that has not arrived reads back as zeros. So
  this runs with the statistics, once a second per torrent, and each row is
  asked exactly once — when the answer is yes.
*/
void MainWindow::readArrivedTags( const QString &infoHash )
{
    for( int i = 0; i < ui->trackList->count(); ++i ) {
        TrackRow *item = qobject_cast<TrackRow *>( ui->trackList->itemWidget( ui->trackList->item( i ) ) );
        if( !item || !item->playlistItem()->track() )
            continue;

        const TrackInfoPtr &track = item->playlistItem()->track();
        if( !infoHash.isEmpty() && track->infoHash() != infoHash )
            continue;

        const QString key = track->infoHash() + QLatin1Char( '/' ) + QString::number( track->fileIndex() );

        if( !TrackTags::hasHead( item->playlistItem()->localPath() ) )
            continue;

        if( !m_tagsRead.contains( key ) ) {
            // The words are in the first kilobytes, so once the head is there
            // the answer will not change: the row is marked whatever came back,
            // and a file with no tags is a file with no tags.
            m_tagsRead.insert( key );
            readTagsOf( track->infoHash(), track->fileIndex() );
        }

        // The picture is the one answer that goes on changing. A cover is tens
        // of kilobytes where the tags are two, so read as early as them it
        // comes back cut in half — and a half decoded jpeg is not null, so the
        // row used to remember it and never ask again. It stayed broken until
        // something unrelated rebuilt the list, which is exactly what somebody
        // switching playlist and coming back was doing by hand.
        if( m_coversRead.contains( key ) || item->hasCover() )
            continue;

        item->refreshCover();

        // Settled when there is one, or when the whole file has arrived and
        // there is none — which is also what stops this asking for ever.
        if( item->hasCover()
            || The::torrentSession()->bufferedBytes( track->infoHash(), track->fileIndex() )
               >= track->fileSize() ) {
            m_coversRead.insert( key );
        }
    }
}

void MainWindow::readTagsOf( const QString &infoHash, int fileIndex )
{
    const TrackInfoPtr track = The::catalog()->track( infoHash, fileIndex );
    if( !track )
        return;

    if( !TrackTags::read( track ) )
        return;

    The::catalog()->updateTags( track );

    TrackRow *item = rowFor( track );
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

TrackRow* MainWindow::rowFor( const TrackInfoPtr &track ) const
{
    if( !track )
        return nullptr;

    for( int i = 0; i < ui->trackList->count(); ++i ) {
        TrackRow *item = qobject_cast<TrackRow *>( ui->trackList->itemWidget( ui->trackList->item( i ) ) );
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
    // A magnet whose track list has not come in a minute is a case of its own,
    // and the entry stays in the map: the timeout is a report and not a giving
    // up — the torrent is still in the session and still announcing, so the
    // metadata may yet arrive, and when they do the selector must open. It used
    // to be dropped right here, which is why a magnet answering at the
    // ninetieth second left an empty row nobody had asked for.
    if( m_awaitingSelection.contains( infoHash ) ) {
        PendingAdd &pending = m_awaitingSelection[ infoHash ];
        pending.searching = false;

        // The blinking was "I am looking"; stopping it on the red is "I looked
        // and nobody answered". Nothing is said in the bar: the row says it,
        // and it goes on saying it.
        ui->playlistList->setPendingSearching( infoHash, false );

        // Asked once and never again. A magnet somebody chose to keep looking
        // for comes back at every start and times out at every start, and a
        // question that reappears every time is one that was not listened to.
        if( !pending.asked ) {
            pending.asked = true;
            askAboutSlowMagnet( infoHash );
        }

        return;
    }

    // Errors on a torrent are ordinary, not exceptional: a modal in the middle
    // of listening would be the most annoying thing this application could do.
    m_playerWidget->showMessage( message );

    for( int i = 0; i < ui->trackList->count(); ++i ) {
        TrackRow *item = qobject_cast<TrackRow *>( ui->trackList->itemWidget( ui->trackList->item( i ) ) );
        if( item && item->playlistItem()->track()
            && item->playlistItem()->track()->infoHash() == infoHash ) {
            item->setTrackState( GrooveOff::ErrorState );
        }
    }
}

/*!
  \brief askAboutSlowMagnet: the one question a failed add is worth

  This is the exception to the rule two functions up, and it has a reason: an
  error arriving on its own is weather, while this is the outcome of something
  somebody did deliberately a minute ago and is waiting on. An add that goes
  well opens the selector; one that does not deserves an answer of the same
  weight.

  Opened rather than executed, which is the whole difference between a question
  and an interruption. The event loop keeps running: the music goes on, the
  window answers clicks, another magnet can be pasted, and two of these that
  come due together stand side by side instead of one on top of the other. And
  because the row keeps the state whatever happens here, closing the window
  without choosing loses nothing.
*/
void MainWindow::askAboutSlowMagnet( const QString &infoHash )
{
    PendingAdd &pending = m_awaitingSelection[ infoHash ];
    if( pending.question )
        return;

    const QString name = pendingName( infoHash );
    const bool anybodyThere = The::torrentSession()->trackerPeers( infoHash ) > 0;

    QMessageBox *box = new QMessageBox( this );
    box->setAttribute( Qt::WA_DeleteOnClose );
    box->setIcon( QMessageBox::Warning );
    box->setWindowTitle( tr( "No answer" ) );

    box->setModal( false );

    // Two sentences and not one, because one of them would be false half the
    // time: a torrent whose tracker knows peers is not one nobody is sharing,
    // it is one that has not sent its track list yet. Saying the wrong one
    // sends whoever reads it looking in the wrong place.
    box->setText( anybodyThere
                  ? tr( "«%1» has not sent its track list." ).arg( name )
                  : tr( "Nobody is sharing «%1» at the moment." ).arg( name ) );

    box->setInformativeText( tr( "It can be left in the sidebar and looked for again at every "
                                 "start: if a source turns up, the track list will arrive on "
                                 "its own." ) );

    QPushButton *remove = box->addButton( tr( "Remove" ), QMessageBox::DestructiveRole );
    QPushButton *keep   = box->addButton( tr( "Keep looking" ), QMessageBox::AcceptRole );
    box->setDefaultButton( keep );

    connect( box, &QDialog::finished, this, [this, infoHash, box, remove] {
        if( box->clickedButton() == remove ) {
            stopLookingFor( infoHash );
            return;
        }

        // Going on looking is a decision, and a decision that only lasted until
        // the program was closed would be the button promising more than it
        // does. So the magnet goes into the catalogue — a row and no playlist,
        // because there is still nothing to make a playlist of.
        //
        // Closing the window without choosing counts as this too: the looking
        // carries on either way, and it would be strange for the way out that
        // decides nothing to be the destructive one.
        keepLookingFor( infoHash );
    } );

    pending.question = box;

    // show() and not open(), which is the whole difference and is invisible to
    // anything driving the program from outside. open() shows a dialog
    // *window-modal* whatever it was set to — it sets the modality itself — and
    // that blocks every click on the window behind while letting D-Bus through,
    // so a check made over MPRIS reports an interface that answers where a
    // person would find it frozen. Worse, it makes the application refuse to
    // quit: closing the main window is blocked by its own modal child and
    // closeAllWindows() gives up, so somebody who left the question open could
    // not shut the program down at all.
    box->show();
}

/*!
  \brief keepLookingFor: makes the waiting outlast the program

  The row of a magnet being looked for lives in memory, which is right while
  nothing has been decided: pasting one and closing the window a moment later
  should leave nothing behind. Choosing to go on looking is the moment
  something is decided, so that is where it is written down.

  A row in torrent and no playlist. There is nothing to make a playlist of —
  the track list is exactly what has not arrived — and the absence is then what
  says, at the next start, which magnets to go looking for again.
*/
void MainWindow::keepLookingFor( const QString &infoHash )
{
    if( !The::database()->isOpen() || !m_awaitingSelection.contains( infoHash ) )
        return;

    const PendingAdd pending = m_awaitingSelection.value( infoHash );

    The::catalog()->rememberMagnet( infoHash, pending.magnetUri, pending.name );
}

/*!
  \brief rememberPendingMagnets: closing early is not giving up

  Writing down the decision and not the paste is right while the program is
  open — pasting a magnet and closing the window a moment later left nothing
  behind, which is what "nothing was decided" should look like. Except that
  closing the window *is* somebody doing something, and reading it as "throw it
  away" is the wrong way round: a magnet without sources is exactly the one
  worth keeping until a source turns up, and whoever pasted it did so on
  purpose.

  So the only way a magnet is forgotten is somebody saying so, with Remove or
  with «Stop looking». This covers the two ways out that used to lose one: the
  first minute, and the question still on screen when the program goes.
*/
void MainWindow::rememberPendingMagnets()
{
    if( !The::database()->isOpen() )
        return;

    for( auto it = m_awaitingSelection.constBegin(); it != m_awaitingSelection.constEnd(); ++it )
        The::catalog()->rememberMagnet( it.key(), it.value().magnetUri, it.value().name );
}

/*!
  \brief stopLookingFor: gives up on a magnet

  Out of the session, out of the catalogue if it ever got there, and out of the
  map. There is no playlist and there are no tracks, so the DELETE has nothing
  to cascade into: giving up on a magnet cannot take anybody's music with it.
*/
void MainWindow::stopLookingFor( const QString &infoHash )
{
    The::torrentSession()->remove( infoHash, true );

    if( The::database()->isOpen() )
        The::catalog()->removeTorrent( infoHash );

    m_awaitingSelection.remove( infoHash );

    loadPlaylists();
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

    for( int i = 0; i < ui->trackList->count(); ++i ) {
        TrackRow *item = qobject_cast<TrackRow *>( ui->trackList->itemWidget( ui->trackList->item( i ) ) );
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

    // The row that is selected now, and at start up — when there is no sidebar
    // to be selected in yet — the one app_state remembers, which is the
    // playlist that was playing. Read before the list is emptied, or it is the
    // emptied list that answers.
    const qint64 open = shownPlaylistId() > 0 ? shownPlaylistId()
                                              : The::playlistStore()->currentPlaylistId();

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

    // The magnets still being looked for are in no query: they live in memory
    // and the clear() above has just taken their rows away.
    addPendingRows();

    // Whatever has just changed may have given a sleeping torrent something to
    // fetch — a track chosen from the selector reopened on it, most of all —
    // and a torrent the session is not holding fetches nothing however much
    // its playlist asks for. Cheap: it wakes only the ones with work left.
    wakeUnfinishedTorrents();

    // Tracks may have been added to the playlist being played while somebody
    // was looking at another one, and they belong in the queue too.
    rebuildPlayQueue();
    updatePlayingMark();

    ui->playlistList->selectPlaylist( open );

    // Playlists but nothing open — a database whose playback row went missing,
    // a first start right after one was made, or the playlist that was open
    // having just been removed — would show a full sidebar beside an empty
    // list, which reads as something having failed to load.
    if( open <= 0 && ui->playlistList->count() > 0 ) {
        QListWidgetItem *first = ui->playlistList->item( 0 );
        openPlaylist( first );

        // And the row is brought forward as well. Opening one without selecting
        // it leaves the sidebar with nothing highlighted beside a track list
        // that is showing something, and the click that would fix it does
        // nothing at all: openPlaylist() returns early on the playlist that is
        // already open, so the row stays unselected however often it is
        // pressed. It looks exactly like a playlist that refuses to open.
        ui->playlistList->selectPlaylist( first->data( PlaylistRoles::Id ).toLongLong() );
    }

    // The rebuild may have left the sidebar pointing at a different kind of
    // playlist, or at nothing — a removal does exactly that — and the menu has
    // to follow. Asked here and not only on the change of selection, because
    // selecting the row that was already current emits nothing.
    updatePlaylistActions();
}

void MainWindow::openPlaylist( QListWidgetItem *row )
{
    if( !row || !The::database()->isOpen() )
        return;

    const qint64 playlistId = row->data( PlaylistRoles::Id ).toLongLong();

    // A magnet still being looked for has no tracks to show and no playlist to
    // point at. Returning here and leaving the list alone would put the
    // highlight on one row while the tracks of another were on screen, which
    // reads as a playlist that failed to open — so the list is emptied and the
    // reason said out loud. releaseRows() lets the rows go without telling the
    // engine, so a track playing out of the playlist just left goes on playing.
    if( playlistId <= 0 ) {
        const QString infoHash = row->data( PlaylistRoles::InfoHash ).toString();
        if( infoHash.isEmpty() )
            return;

        ui->trackList->releaseRows();
        m_shownPlaylistId = 0;
        m_playerWidget->showMessage( tr( "Still looking for the tracks of «%1»…" )
                                     .arg( pendingName( infoHash ) ) );
        return;
    }

    if( playlistId == m_shownPlaylistId )
        return;

    // The row becomes the current one here rather than being assumed to be:
    // what is on screen is now read from the sidebar, so a caller that opens a
    // playlist without touching the selection — the fallback below, a menu, a
    // test — would otherwise draw one playlist while the sidebar named another.
    ui->playlistList->selectPlaylist( playlistId );

    // What app_state holds is the playlist being played, not the one being
    // looked at, so opening another one says nothing to it: browsing is not an
    // act of stopping. The position of what is playing is written all the same,
    // since it goes on playing and this is a good moment to remember it.
    savePlaybackState();

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
/*!
  \brief emptyPlaylist: takes every track out and leaves the list where it is

  What the playlist that is always there is for: fill it, listen to it, empty
  it, fill it again, without inventing a name each time. It is offered on every
  playlist made by hand, because two lists that are the same thing should not
  answer differently to the same gesture.

  No question is asked. Nothing is lost: the tracks stay in the catalogue and
  in whatever else holds them, and putting them back is a drag away.
*/
void MainWindow::emptyPlaylist()
{
    const qint64 playlistId = ui->playlistList->currentPlaylistId();
    if( playlistId <= 0 || !The::database()->isOpen() )
        return;

    const PlaylistStore::PlaylistInfo playlist = The::playlistStore()->playlist( playlistId );
    if( playlist.kind != PlaylistStore::CustomPlaylist || playlist.trackCount == 0 )
        return;

    if( !The::playlistStore()->clearPlaylist( playlistId ) )
        return;

    loadPlaylists();
    loadCurrentPlaylist();
}

void MainWindow::removePlaylist()
{
    // A magnet still being looked for comes through the same entry, because
    // from the sidebar it is a row like any other and getting rid of it is the
    // same gesture. What it means underneath is different — there is no
    // playlist, no track and no byte on disk, only the looking — so it takes
    // its own way out. It asks the same question, though: what is being removed
    // is the row somebody is pointing at, and a question that changed its
    // wording with what happens to sit behind the row would be making a
    // distinction the person asking has no reason to care about.
    const QString pendingHash = ui->playlistList->currentPendingHash();
    if( !pendingHash.isEmpty() ) {
        const int answer = QMessageBox::question( this,
                                                  tr( "Remove torrent" ),
                                                  tr( "Remove «%1»?" ).arg( pendingName( pendingHash ) ),
                                                  QMessageBox::Yes | QMessageBox::No,
                                                  QMessageBox::No );

        if( answer == QMessageBox::Yes )
            stopLookingFor( pendingHash );

        return;
    }

    const qint64 playlistId = ui->playlistList->currentPlaylistId();
    if( playlistId <= 0 || !The::database()->isOpen() )
        return;

    const PlaylistStore::PlaylistInfo playlist = The::playlistStore()->playlist( playlistId );
    if( !playlist.isValid() || playlist.pinned )
        return;

    if( playlist.kind == PlaylistStore::CustomPlaylist ) {
        const int answer = QMessageBox::question( this,
                                                  tr( "Remove playlist" ),
                                                  tr( "Remove «%1»?" ).arg( playlist.name ),
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
                                                  tr( "Remove «%1»?" ).arg( playlist.name ),
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
        names << Utility::playlistName( reference.name, reference.pinned );
    }

    QMessageBox box( this );
    box.setIcon( QMessageBox::Warning );
    box.setWindowTitle( tr( "Remove torrent" ) );
    box.setText( tr( "Remove «%1»?" ).arg( playlist.name ) );
    box.setInformativeText( tr( "%n track(s) of this torrent are used in your own playlists: %1.",
                                "", trackCount ).arg( names.join( QLatin1String( ", " ) ) ) );

    QPushButton *removeAll = box.addButton( tr( "Remove everything" ), QMessageBox::DestructiveRole );
    QPushButton *keepUsed  = box.addButton( tr( "Remove only what nothing else uses" ),
                                            QMessageBox::AcceptRole );
    box.addButton( QMessageBox::Cancel );
    box.setDefaultButton( keepUsed );

    box.exec();

    if( box.clickedButton() == removeAll ) {
        removeTorrentEverywhere( infoHash );
    } else if( box.clickedButton() == keepUsed ) {
        // Nothing is hidden any more: the row stays in the sidebar holding what
        // survived, so what is left and why is on the screen rather than in a
        // column. The entry does not always remove the playlist, and the
        // wording of the button is what says so.
        const QList<qint64> kept = The::catalog()->tracksInCustomPlaylists( infoHash );

        QList<TrackInfoPtr> remaining;
        QList<TrackInfoPtr> going;

        // Filtered out of the list as it stands, not rebuilt from the kept ids:
        // a playlist dragged into shape would otherwise come back in file order.
        for( const TrackInfoPtr &track : The::playlistStore()->tracks( playlistId ) ) {
            if( kept.contains( track->id() ) )
                remaining << track;
            else
                going << track;
        }

        QList<PlaylistItemPtr> playing;
        for( const TrackInfoPtr &track : std::as_const( going ) ) {
            if( TrackRow *row = rowFor( track ) )
                playing << row->playlistItem();
        }

        detachFromPlayer( playing );

        The::playlistStore()->setTracks( playlistId, remaining );
        discardTracks( going );
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
    //
    // Everything of this torrent the player is holding goes, and stopping is
    // not enough: a stopped player still holds its track, so the bar went on
    // showing the name, the album and the cover of music that had just been
    // deleted, and play answered with a url the server now refuses. It is the
    // whole queue and not only the track being listened to, or the one that
    // follows would be another track of the torrent being removed.
    QList<PlaylistItemPtr> going;
    for( int i = 0; i < The::playlist()->count(); ++i ) {
        const PlaylistItemPtr item = The::playlist()->item( i );
        if( item && item->track() && item->track()->infoHash() == infoHash )
            going << item;
    }

    const PlaylistItemPtr current = The::audioEngine()->currentTrack();
    if( current && current->track() && current->track()->infoHash() == infoHash ) {
        if( !going.contains( current ) )
            going << current;

        The::cacheManager()->setPlayingTorrent( QString() );
    }

    detachFromPlayer( going );

    The::downloadQueue()->forget( infoHash );

    The::torrentSession()->remove( infoHash, true );
    The::catalog()->removeTorrent( infoHash );
}

void MainWindow::updatePlaylistActions()
{
    ActionCollection *ac = The::actionCollection();

    const qint64 playlistId = The::database()->isOpen() ? ui->playlistList->currentPlaylistId() : 0;
    const PlaylistStore::PlaylistInfo playlist = playlistId > 0 ? The::playlistStore()->playlist( playlistId )
                                                                : PlaylistStore::PlaylistInfo();

    // The one that is always there has no name of its own to change and no
    // removal to offer. Leaving the entries in and refusing afterwards would be
    // the interface offering something it will not do.
    const bool named = playlist.isValid() && !playlist.pinned;

    // A magnet still being looked for answers none of the questions below — no
    // name of its own to change, no tracks to empty, no files to choose between
    // — but it does need the way out, and it is the same one: from the sidebar
    // it is a row, and removing a row is one command however different the
    // things behind them are.
    const bool pending = !ui->playlistList->currentPendingHash().isEmpty();

    ac->getAction( "actionRenamePlaylist" )->setVisible( named );
    ac->getAction( "actionRemovePlaylist" )->setVisible( named || pending );

    // Only a torrent has files to choose between; a list made by hand is filled
    // by dragging into it, and emptying is what that one is offered instead.
    ac->getAction( "actionChooseTracks" )->setVisible( playlist.kind == PlaylistStore::TorrentPlaylist
                                                       && playlist.isValid() );
    ac->getAction( "actionEmptyPlaylist" )->setVisible( playlist.kind == PlaylistStore::CustomPlaylist
                                                        && playlist.isValid() );

    fillAddToPlaylistMenu( playlist );
}

/*!
  \brief fillAddToPlaylistMenu: which lists a whole torrent can be poured into

  Three torrents of Vivaldi and one list to hear them one after the other: that
  is the gesture, and doing it a track at a time is the same gesture repeated
  forty times. What goes in is the torrent's playlist and not the torrent —
  what was left unticked was left out on purpose, and what was excluded is a
  refusal that is meant to survive, so taking every file of the torrent would
  undo both.

  The entries are rebuilt at every change of row rather than kept up to date,
  because a list made by hand can appear or be renamed at any moment and a menu
  that is right only until then is worse than one built late.
*/
void MainWindow::fillAddToPlaylistMenu( const PlaylistStore::PlaylistInfo &playlist )
{
    QMenu *menu = The::actionCollection()->getMenu( "addToPlaylistMenu" );
    if( !menu )
        return;

    menu->clear();

    // Only a torrent playlist, and only one with something in it: on an empty
    // one there is nothing to pour and no wording that would make the entry
    // mean anything.
    const bool applies = playlist.isValid() && playlist.kind == PlaylistStore::TorrentPlaylist
                         && playlist.trackCount > 0;

    menu->menuAction()->setVisible( applies );
    if( !applies )
        return;

    for( const PlaylistStore::PlaylistInfo &target : The::playlistStore()->visiblePlaylists() ) {
        if( target.kind != PlaylistStore::CustomPlaylist )
            continue;

        QAction *entry = menu->addAction( Utility::playlistName( target ) );
        entry->setData( target.id );
    }

    // Greyed out rather than hidden, which is the opposite of what the entries
    // above do, and for the opposite reason: there is a now — «New playlist…»
    // is one line up — so the entry says what is missing instead of vanishing.
    menu->setEnabled( !menu->isEmpty() );
    menu->setTitle( menu->isEmpty() ? tr( "Add all tracks to (no list of your own yet)" )
                                    : tr( "Add all tracks to" ) );
}

void MainWindow::showPlaylistMenu( const QPoint &position )
{
    const qint64 playlistId = ui->playlistList->playlistAt( position );
    const QString pendingHash = ui->playlistList->pendingAt( position );

    // The menu acts on the row it was opened over, so that row becomes the
    // current one first: otherwise a right click somewhere and a Remove would
    // take away whatever happened to be selected before.
    if( playlistId > 0 )
        ui->playlistList->selectPlaylist( playlistId );
    else if( !pendingHash.isEmpty() )
        ui->playlistList->selectPending( pendingHash );

    // And selecting it is not only about what the entries act on: which of them
    // apply is decided from the current row, so this has to come after. Asked
    // here as well as on the change of selection, because a right click on the
    // row that is already current changes nothing and emits nothing.
    updatePlaylistActions();

    ActionCollection *ac = The::actionCollection();

    QMenu menu( this );
    menu.addAction( ac->getAction( "actionNewPlaylist" ) );

    if( playlistId > 0 || !pendingHash.isEmpty() ) {
        menu.addSeparator();
        menu.addAction( ac->getAction( "actionRenamePlaylist" ) );
        menu.addAction( ac->getAction( "actionChooseTracks" ) );
        menu.addMenu( ac->getMenu( "addToPlaylistMenu" ) );
        menu.addAction( ac->getAction( "actionEmptyPlaylist" ) );
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

    // A list holds a track at most once and the writer drops the repeats, so
    // what is dropped has to be dropped here too or the count reported would be
    // the count asked for rather than the count that landed. It shows with a
    // whole torrent poured into a list that already has half of it.
    QList<TrackInfoPtr> tracks;
    for( qint64 trackId : trackIds ) {
        if( The::playlistStore()->contains( playlistId, trackId ) )
            continue;

        const TrackInfoPtr track = The::catalog()->track( trackId );
        if( track )
            tracks << track;
    }

    const QString name = Utility::playlistName( playlist );

    if( tracks.isEmpty() ) {
        m_playerWidget->showMessage( tr( "Already in «%1»." ).arg( name ) );
        return;
    }

    if( !The::playlistStore()->appendTracks( playlistId, tracks ) )
        return;

    loadPlaylists();

    if( playlistId == shownPlaylistId() )
        loadCurrentPlaylist();

    m_playerWidget->showMessage( tr( "%n track(s) added to «%1».", "", tracks.count() )
                                 .arg( name ) );
}

QList<TrackInfoPtr> MainWindow::tracksForGesture( TrackRow *row ) const
{
    QList<TrackInfoPtr> tracks;

    for( TrackRow *each : rowsForGesture( row ) ) {
        if( each->playlistItem() && each->playlistItem()->track() )
            tracks << each->playlistItem()->track();
    }

    return tracks;
}

void MainWindow::addTrackToPlaylist( TrackRow *item, qint64 playlistId )
{
    QList<qint64> ids;
    for( const TrackInfoPtr &track : tracksForGesture( item ) )
        ids << track->id();

    addTracksToPlaylist( playlistId, ids );
}

void MainWindow::addShownTorrentToPlaylist( QAction *chosen )
{
    if( !chosen || !The::database()->isOpen() )
        return;

    const qint64 target = chosen->data().toLongLong();
    const qint64 source = ui->playlistList->currentPlaylistId();
    if( target <= 0 || source <= 0 )
        return;

    // In the order of the torrent's own playlist, which is the order on screen
    // and the order the tracks were dragged into. Three albums poured in turn
    // arrive as three albums and not as a shuffle.
    QList<qint64> trackIds;
    for( const TrackInfoPtr &track : The::playlistStore()->tracks( source ) ) {
        if( track && track->id() > 0 )
            trackIds << track->id();
    }

    // Queued, and not out of caution about the work: adding rebuilds the
    // sidebar, and rebuilding refills this very submenu, which begins by
    // deleting the action whose signal we are standing inside. Doing that while
    // the menu is still unwinding is how a gesture that worked a hundred times
    // crashes on the hundred and first.
    QMetaObject::invokeMethod( this, [this, target, trackIds] {
        addTracksToPlaylist( target, trackIds );
    }, Qt::QueuedConnection );
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
    const qint64 playlistId = shownPlaylistId();
    if( playlistId <= 0 || rows.isEmpty() )
        return;

    QList<TrackInfoPtr> tracks;
    for( int i = 0; i < ui->trackList->count(); ++i ) {
        TrackRow *item = qobject_cast<TrackRow *>( ui->trackList->itemWidget( ui->trackList->item( i ) ) );
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
        The::playlistStore()->playlist( shownPlaylistId() );

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

    // The engine goes on holding the track it was given after the rows behind
    // it are deleted — a torrent removed while one of its tracks was playing is
    // exactly that — so this can name a track that no longer exists. What to do
    // about it belongs to the writer and is done there.
    // The playlist being played and not the one on screen: what this row is
    // for is coming back to what was playing, and coming back to a list that
    // happened to be open when the window closed would name a playlist the
    // remembered track may not even be in.
    The::playlistStore()->setPlaybackState( m_playingPlaylistId > 0 ? m_playingPlaylistId
                                                                    : shownPlaylistId(),
                                            current->track()->id(),
                                            The::audioEngine()->currentTime() );
}

void MainWindow::addPlaylistItem( const PlaylistItemPtr &playlistItem )
{
    TrackRow *item = new TrackRow( playlistItem, this );

    connect( item,             &TrackRow::reloadPlaylist,
             ui->trackList, &TrackList::reloadPlaylist );

    // The row asks and the window acts, then the list is rebuilt from what was
    // written — the same road every other change to a playlist takes. Taking
    // the row out here and writing the rows that are left, which is what used
    // to happen, cannot serve a gesture that removes a track from lists nobody
    // is looking at.
    connect( item, &TrackRow::removeRequested, this, &MainWindow::removeTracksRequested );

    connect( item, &TrackRow::addToPlaylistRequested, this, &MainWindow::addTrackToPlaylist );

    QListWidgetItem *wi = new QListWidgetItem;

    // The row carries the id as well as the widget: a drag leaves the widget
    // behind and takes the item's data with it.
    if( playlistItem->track() )
        wi->setData( SongRoles::TrackId, playlistItem->track()->id() );

    ui->trackList->addItem( wi );
    ui->trackList->setItemWidget( wi, item );
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
    QDesktopServices::openUrl( QUrl( Utility::donateUrl ) );
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
