/*
    GrooveOff - Offline Grooveshark.com music
    Copyright (C) 2013  Giuseppe Calà <jiveaxe@gmail.com>

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

#include "actioncollection.h"

#include <QAction>
#include <QMenuBar>
#include <QMenu>
#include <QAction>

ActionCollection* ActionCollection::s_instance = 0;
ActionCollection* ActionCollection::instance()
{
    return s_instance;
}

ActionCollection::ActionCollection( QObject* parent )
    : QObject( parent )
{
    s_instance = this;
    initActions();
}

ActionCollection::~ActionCollection()
{
    s_instance = 0;
    foreach( QString key, m_actionCollection.keys() )
        delete m_actionCollection[ key ];
}

void
ActionCollection::initActions()
{
    QAction *actionClose = new QAction(QIcon::fromTheme(QLatin1String("application-exit"),
                               QIcon(QLatin1String(":/resources/application-exit.png"))),
                               trUtf8("&Exit"), this);
    actionClose->setToolTip(trUtf8("Close GrooveOff"));
    actionClose->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_Q));
    m_actionCollection[ "actionClose" ] = actionClose;
    m_actionCollection[ "actionClose" ]->setMenuRole( QAction::QuitRole );

    QAction *actionDonate = new QAction(QIcon::fromTheme(QLatin1String("help-donate"),
                                QIcon(QLatin1String(":/resources/help-donate.png"))),
                                trUtf8("&Donate"), this);
    actionDonate->setToolTip(trUtf8("Donate with PayPal"));
    m_actionCollection[ "actionDonate" ] = actionDonate;

    QAction *actionConfigure = new QAction(trUtf8("&Configure GrooveOff..."), this);
    if(QIcon::hasThemeIcon(QLatin1String("configure")))
        actionConfigure->setIcon(QIcon::fromTheme(QLatin1String("configure")));
    else
        actionConfigure->setIcon(QIcon::fromTheme(QLatin1String("gconf-editor"),
                                  QIcon(QLatin1String(":/resources/configure.png"))));
    m_actionCollection[ "actionConfigure" ] = actionConfigure;

    QAction *actionCompact = new QAction(QIcon::fromTheme(QLatin1String("view-split-top-bottom"),
                                 QIcon(QLatin1String(":/resources/view-split-top-bottom.png"))),
                                 trUtf8("&Compact Layout"), this);
    actionCompact->setCheckable(true);
    m_actionCollection[ "actionCompact" ] = actionCompact;

    QAction *actionWide = new QAction(QIcon::fromTheme(QLatin1String("view-split-left-right"),
                              QIcon(QLatin1String(":/resources/view-split-left-right"))),
                              trUtf8("&Wide Layout"), this);
    actionWide->setCheckable(true);
    m_actionCollection[ "actionWide" ] = actionWide;

    QAction *actionNewToken = new QAction(QIcon::fromTheme(QLatin1String("emblem-new"),
                                  QIcon(QLatin1String(":/resources/emblem-new.png"))),
                                  trUtf8("&Get new token..."), this);
    m_actionCollection[ "actionNewToken" ] = actionNewToken;

    QAction *actionStopDownloads = new QAction(QIcon::fromTheme(QLatin1String("process-stop"),
                                      QIcon(QLatin1String(":/resources/process-stop.png"))),
                                      trUtf8("&Stop all downloads"), this);
    m_actionCollection[ "actionStopDownloads" ] = actionStopDownloads;

    QAction *actionRemoveFailed = new QAction(QIcon::fromTheme(QLatin1String("edit-delete"),
                                      QIcon(QLatin1String(":/resources/edit-delete.png"))),
                                      trUtf8("&Remove deleted/failed transfers"), this);
    m_actionCollection[ "actionRemoveFailed" ] = actionRemoveFailed;

    QAction *actionClearDownloadList = new QAction(QIcon::fromTheme(QLatin1String("edit-clear"),
                                           QIcon(QLatin1String(":/resources/edit-clear"))),
                                           trUtf8("Remove all finished"), this);
    actionClearDownloadList->setToolTip(trUtf8("Removes all finished transfers and leaves all files on disk"));
    m_actionCollection[ "actionClearDownloadList" ] = actionClearDownloadList;

    QAction *actionAbout = new QAction(QIcon::fromTheme(QLatin1String("help-about"),
                               QIcon(QLatin1String(":/resources/help-about.png"))),
                               trUtf8("&About GrooveOff"), this);
    m_actionCollection[ "actionAbout" ] = actionAbout;

    QAction *actionQtAbout = new QAction(QIcon::fromTheme(QLatin1String("qtlogo"),
                                 QIcon(QLatin1String(":/resources/qtlogo.png"))),
                                 trUtf8("About &Qt"), this);
    m_actionCollection[ "actionQtAbout" ] = actionQtAbout;
    m_actionCollection[ "actionQtAbout" ]->setMenuRole( QAction::AboutQtRole );

/*
    QAction *latchOn = new QAction( tr( "&Listen Along" ), this );
    latchOn->setIcon( ImageRegistry::instance()->icon( RESPATH "images/headphones.svg" ) );
    m_actionCollection[ "latchOn" ] = latchOn;
    QAction *latchOff = new QAction( tr( "Stop &Listening Along" ), this );
    latchOff->setIcon( ImageRegistry::instance()->icon( RESPATH "images/headphones-off.svg" ) );
    m_actionCollection[ "latchOff" ] = latchOff;

    QAction *realtimeFollowingAlong = new QAction( tr( "&Follow in real-time" ), this );
    realtimeFollowingAlong->setCheckable( true );
    m_actionCollection[ "realtimeFollowingAlong" ] = realtimeFollowingAlong;

    bool isPublic = TomahawkSettings::instance()->privateListeningMode() == TomahawkSettings::PublicListening;
    QAction *privacyToggle = new QAction( ( isPublic ? tr( "&Listen Privately" ) : tr( "&Listen Publicly" ) ), this );
    privacyToggle->setIcon( ImageRegistry::instance()->icon( RESPATH "images/private-listening.svg" ) );
    privacyToggle->setIconVisibleInMenu( isPublic );
    m_actionCollection[ "togglePrivacy" ] = privacyToggle;
    connect( m_actionCollection[ "togglePrivacy" ], SIGNAL( triggered() ), SLOT( togglePrivateListeningMode() ), Qt::UniqueConnection );

    m_actionCollection[ "loadPlaylist" ] =   new QAction( tr( "&Load Playlist" ), this );
    m_actionCollection[ "loadStation" ] =    new QAction( tr( "&Load Station" ), this );
    m_actionCollection[ "renamePlaylist" ] = new QAction( tr( "&Rename Playlist" ), this );
    m_actionCollection[ "renameStation" ] = new QAction( tr( "&Rename Station" ), this );
    m_actionCollection[ "copyPlaylist" ] =   new QAction( tr( "&Copy Playlist Link" ), this );
    m_actionCollection[ "playPause" ] =      new QAction( tr( "&Play" ), this );
    m_actionCollection[ "playPause" ]->setIcon( ImageRegistry::instance()->icon( RESPATH "images/play-rest.svg" ) );
    m_actionCollection[ "playPause" ]->setShortcut( Qt::Key_Space );
    m_actionCollection[ "playPause" ]->setShortcutContext( Qt::ApplicationShortcut );
    m_actionCollection[ "stop" ] =           new QAction( tr( "&Stop" ), this );
    m_actionCollection[ "previousTrack" ] =  new QAction( tr( "&Previous Track" ), this );
    m_actionCollection[ "previousTrack" ]->setIcon( ImageRegistry::instance()->icon( RESPATH "images/back-rest.svg" ) );
    m_actionCollection[ "previousTrack" ]->setShortcut( QKeySequence( "Left" ) );
    m_actionCollection[ "nextTrack" ] =      new QAction( tr( "&Next Track" ), this );
    m_actionCollection[ "nextTrack" ]->setIcon( ImageRegistry::instance()->icon( RESPATH "images/skip-rest.svg" ) );
    m_actionCollection[ "nextTrack" ]->setShortcut( QKeySequence( "Right" ) );
    m_actionCollection[ "quit" ] =           new QAction( tr( "&Quit" ), this );
    m_actionCollection[ "quit" ]->setShortcut( QKeySequence::Quit );
    m_actionCollection[ "quit" ]->setShortcutContext( Qt::ApplicationShortcut );
    m_actionCollection[ "quit" ]->setMenuRole( QAction::QuitRole );

    // connect actions to AudioEngine
    AudioEngine *ae = AudioEngine::instance();
    connect( m_actionCollection[ "playPause" ],     SIGNAL( triggered() ), ae,   SLOT( playPause() ), Qt::UniqueConnection );
    connect( m_actionCollection[ "stop" ],          SIGNAL( triggered() ), ae,   SLOT( stop() ),      Qt::UniqueConnection );
    connect( m_actionCollection[ "previousTrack" ], SIGNAL( triggered() ), ae,   SLOT( previous() ),  Qt::UniqueConnection );
    connect( m_actionCollection[ "nextTrack" ],     SIGNAL( triggered() ), ae,   SLOT( next() ),      Qt::UniqueConnection );

    // main menu actions
    m_actionCollection[ "loadXSPF" ] = new QAction( tr( "Load &XSPF..." ), this );
    m_actionCollection[ "updateCollection" ] = new QAction( tr( "U&pdate Collection" ), this );
    m_actionCollection[ "rescanCollection" ] = new QAction( tr( "Fully &Rescan Collection" ), this );
    m_actionCollection[ "showOfflineSources" ] = new QAction( tr( "Show Offline Sources" ), this );
    m_actionCollection[ "showOfflineSources" ]->setCheckable( true );
    m_actionCollection[ "preferences" ] = new QAction( tr( "&Configure Tomahawk..." ), this );
    m_actionCollection[ "preferences" ]->setIcon( ImageRegistry::instance()->icon( RESPATH "images/configure.svg" ) );
    m_actionCollection[ "preferences" ]->setMenuRole( QAction::PreferencesRole );
#ifdef Q_OS_MAC
    m_actionCollection[ "minimize" ] = new QAction( tr( "Minimize" ), this );
    m_actionCollection[ "minimize" ]->setShortcut( QKeySequence( "Ctrl+M" ) );
    m_actionCollection[ "zoom" ] = new QAction( tr( "Zoom" ), this );
    m_actionCollection[ "zoom" ]->setShortcut( QKeySequence( "Meta+Ctrl+Z" ) );
    m_actionCollection[ "fullscreen" ] = new QAction( tr( "Enter Full Screen" ), this );
    m_actionCollection[ "fullscreen" ]->setShortcut( QKeySequence( "Meta+Ctrl+F" ) );
#else
    m_actionCollection[ "toggleMenuBar" ] = new QAction( tr( "Hide Menu Bar" ), this );
    m_actionCollection[ "toggleMenuBar" ]->setShortcut( QKeySequence( "Ctrl+M" ) );
    m_actionCollection[ "toggleMenuBar" ]->setShortcutContext( Qt::ApplicationShortcut );
#endif
    m_actionCollection[ "diagnostics" ] = new QAction( tr( "Diagnostics..." ), this );
    m_actionCollection[ "diagnostics" ]->setMenuRole( QAction::ApplicationSpecificRole );
    m_actionCollection[ "aboutTomahawk" ] = new QAction( tr( "About &Tomahawk..." ), this );
    m_actionCollection[ "aboutTomahawk" ]->setIcon( ImageRegistry::instance()->icon( RESPATH "images/info.svg" ) );
    m_actionCollection[ "aboutTomahawk" ]->setMenuRole( QAction::AboutRole );
    m_actionCollection[ "legalInfo" ] = new QAction( tr( "&Legal Information..." ), this );
    m_actionCollection[ "legalInfo" ]->setMenuRole( QAction::ApplicationSpecificRole );
    m_actionCollection[ "openLogfile" ] = new QAction( tr( "&View Logfile" ), this );
    m_actionCollection[ "openLogfile" ]->setMenuRole( QAction::ApplicationSpecificRole );
    #if defined( Q_OS_MAC ) && defined( HAVE_SPARKLE ) || defined( Q_WS_WIN )
    m_actionCollection[ "checkForUpdates" ] = new QAction( tr( "Check For Updates..." ), this );
    m_actionCollection[ "checkForUpdates" ]->setMenuRole( QAction::ApplicationSpecificRole );
#endif
    m_actionCollection[ "crashNow" ] = new QAction( "Crash now...", this );
*/
}

QMenuBar*
ActionCollection::createMenuBar( QWidget *parent )
{
    QMenuBar* menuBar = new QMenuBar( parent );

    QMenu* fileMenu = new QMenu( trUtf8( "&File" ), menuBar );
    fileMenu->addAction( m_actionCollection[ "actionNewToken" ] );
    fileMenu->addSeparator();
    fileMenu->addAction( m_actionCollection[ "actionDonate" ] );
    fileMenu->addSeparator();
    fileMenu->addAction( m_actionCollection[ "actionClose" ] );

    QMenu* downloadsMenu = new QMenu( trUtf8( "&Downloads" ), menuBar );
    downloadsMenu->addAction( m_actionCollection[ "actionStopDownloads" ] );
    downloadsMenu->addSeparator();
    downloadsMenu->addAction( m_actionCollection[ "actionRemoveFailed" ] );
    downloadsMenu->addAction( m_actionCollection[ "actionClearDownloadList" ] );

    QMenu* viewMenu = new QMenu( trUtf8( "&View" ), menuBar );
    viewMenu->addAction( m_actionCollection[ "actionCompact" ] );
    viewMenu->addAction( m_actionCollection[ "actionWide" ] );


    QMenu* settingsMenu = new QMenu( trUtf8( "&Settings" ), menuBar );
    settingsMenu->addAction( m_actionCollection[ "actionConfigure" ] );

    QMenu* helpMenu = new QMenu( trUtf8( "&Help" ), menuBar );
    helpMenu->addAction( m_actionCollection[ "actionAbout" ] );
    helpMenu->addAction( m_actionCollection[ "actionQtAbout" ] );

    menuBar->addMenu( fileMenu );
    menuBar->addMenu( downloadsMenu );
    menuBar->addMenu( viewMenu );
    menuBar->addMenu( settingsMenu );
    menuBar->addMenu( helpMenu );

    return menuBar;
}


QMenu*
ActionCollection::createCompactMenu( QWidget *parent )
{
    QMenu* compactMenu = new QMenu( tr( "Main Menu" ), parent );

    compactMenu->addAction( m_actionCollection[ "actionNewToken" ] );
    compactMenu->addSeparator();
    compactMenu->addAction( m_actionCollection[ "actionStopDownloads" ] );
    compactMenu->addAction( m_actionCollection[ "actionRemoveFailed" ] );
    compactMenu->addAction( m_actionCollection[ "actionClearDownloadList" ] );
    compactMenu->addSeparator();
    compactMenu->addAction( m_actionCollection[ "actionCompact" ] );
    compactMenu->addAction( m_actionCollection[ "actionWide" ] );
    compactMenu->addSeparator();
    compactMenu->addAction( m_actionCollection[ "actionConfigure" ] );
    compactMenu->addSeparator();
    compactMenu->addAction( m_actionCollection[ "actionAbout" ] );
    compactMenu->addAction( m_actionCollection[ "actionDonate" ] );
    compactMenu->addSeparator();
    compactMenu->addAction( m_actionCollection[ "actionClose" ] );

    return compactMenu;
}


QAction*
ActionCollection::getAction( const QString& name )
{
    return m_actionCollection.value( name, 0 );
}
