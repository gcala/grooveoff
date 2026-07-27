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


#include "ActionCollection.h"

#include <QAction>
#include <QActionGroup>
#include <QMenuBar>
#include <QMenu>

#include <utility>

namespace The {
    static ActionCollection* s_ActionCollection_instance = 0;

    ActionCollection* actionCollection()
    {
        if( !s_ActionCollection_instance )
            s_ActionCollection_instance = new ActionCollection();

        return s_ActionCollection_instance;
    }
}

ActionCollection::ActionCollection( )
{
    initActions();
}

ActionCollection::~ActionCollection()
{
    // delete all actions before exiting
    for( QAction *action : std::as_const( m_actionCollection ) )
        delete action;
}

void ActionCollection::initActions()
{
    // The "close window" action
    QAction *actionClose = new QAction( QIcon::fromTheme( QLatin1String("application-exit" ), // theme icon
                                                          QIcon( QLatin1String( ":/resources/application-exit.png" ) ) // resource icon (fallback)
                                                        ),
                                        tr( "&Exit" ), // action text
                                        this //parent
                                       );

    QAction *actionDonate = new QAction( QIcon::fromTheme( QLatin1String( "help-donate" ),
                                                           QIcon(QLatin1String( ":/resources/help-donate.png" ) )
                                                         ),
                                         tr("&Donate"),
                                         this
                                       );    

//     QAction *actionManageSessions = new QAction(QIcon::fromTheme(QLatin1String("edit-rename"),
//                                 QIcon(QLatin1String(":/resources/edit-rename.png"))),
//                                 tr("&Manage Sessions"), this);

//     QAction *actionSaveSessionAs = new QAction(QIcon::fromTheme(QLatin1String("document-save-as"),
//                                 QIcon(QLatin1String(":/resources/document-save-as.png"))),
//                                 tr("&Save Session As..."), this);

    QAction *actionConfigure = new QAction( tr( "Configure &GrooveOff..." ), this );
    actionConfigure->setIcon( QIcon::fromTheme( QLatin1String( "configure" ),
                                                QIcon::fromTheme(QLatin1String( "gconf-editor" ),
                                                                 QIcon(QLatin1String(":/resources/configure.png" ) )
                                                                )
                                              )
                            );

    QAction *actionCompact = new QAction( QIcon::fromTheme( QLatin1String( "view-split-top-bottom" ),
                                                            QIcon(QLatin1String( ":/resources/view-split-top-bottom.png" ) )
                                                          ),
                                          tr( "&Compact Layout" ),
                                          this
                                        );
    actionCompact->setCheckable( true );

    QAction *actionWide = new QAction( QIcon::fromTheme( QLatin1String( "view-split-left-right" ),
                                                         QIcon( QLatin1String( ":/resources/view-split-left-right" ) )
                                                       ),
                                       tr( "&Wide Layout" ),
                                       this
                                     );
    actionWide->setCheckable( true );

    QAction *miniPlayer = new QAction( QIcon::fromTheme( QLatin1String( "view-split-left-right" ),
                                                         QIcon( QLatin1String( ":/resources/view-split-left-right" ) )
                                                       ),
                                       tr( "&Mini Player" ), 
                                       this
                                     );
    miniPlayer->setCheckable( true );

    QAction *actionOpenTorrent = new QAction( QIcon::fromTheme( QLatin1String( "document-open" ),
                                                                QIcon( QLatin1String( ":/resources/emblem-new.png" ) )
                                                              ),
                                              tr( "&Open torrent..." ),
                                              this
                                            );

    QAction *actionStopDownloads = new QAction( QIcon::fromTheme( QLatin1String( "process-stop" ),
                                                                  QIcon( QLatin1String( ":/resources/process-stop.png" ) )
                                                                ),
                                                tr( "&Stop all downloads" ), 
                                                this
                                              );

    QAction *actionRemoveFailed = new QAction( QIcon::fromTheme( QLatin1String( "edit-delete" ),
                                                                 QIcon( QLatin1String( ":/resources/edit-delete.png" ) )
                                                               ),
                                               tr( "&Remove canceled/failed downloads" ), 
                                               this
                                             );

    QAction *actionClearDownloadList = new QAction( QIcon::fromTheme( QLatin1String( "edit-clear" ),
                                                                      QIcon( QLatin1String( ":/resources/edit-clear" ) )
                                                                    ),
                                                    tr( "Clear &finished" ),
                                                    this
                                                  );
    
    QAction *actionNewPlaylist = new QAction( QIcon::fromTheme( QLatin1String( "list-add" ),
                                                               QIcon( QLatin1String( ":/resources/emblem-new.png" ) )
                                                             ),
                                             tr( "&New playlist..." ),
                                             this
                                           );

    QAction *actionRenamePlaylist = new QAction( QIcon::fromTheme( QLatin1String( "edit-rename" ),
                                                                   QIcon( QLatin1String( ":/resources/edit-rename.png" ) )
                                                                 ),
                                                 tr( "Re&name playlist..." ),
                                                 this
                                               );

    QAction *actionRemovePlaylist = new QAction( QIcon::fromTheme( QLatin1String( "edit-delete" ),
                                                                   QIcon( QLatin1String( ":/resources/edit-delete.png" ) )
                                                                 ),
                                                 tr( "Re&move playlist..." ),
                                                 this
                                               );

    QAction *actionEmptyCache = new QAction( QIcon::fromTheme( QLatin1String( "edit-clear-history" ),
                                                               QIcon( QLatin1String( ":/resources/edit-clear" ) )
                                                             ),
                                             tr( "&Empty the cache" ),
                                             this
                                           );

    QAction *actionAbout = new QAction( QIcon::fromTheme( QLatin1String( "help-about" ),
                                                          QIcon( QLatin1String( ":/resources/help-about.png" ) )
                                                        ),
                                        tr( "&About GrooveOff" ), 
                                        this
                                      );
    

    QAction *actionQtAbout = new QAction( QIcon::fromTheme( QLatin1String( "qtlogo" ),
                                                            QIcon( QLatin1String( ":/resources/qtlogo.png" ) )
                                                          ),
                                          tr( "About &Qt" ), 
                                          this
                                        );
    
    // append all actions to the collector
    m_actionCollection[ QLatin1String( "actionClose" ) ] = actionClose;
    m_actionCollection[ QLatin1String( "actionDonate" ) ] = actionDonate;
//     m_actionCollection[ QLatin1String( "actionManageSessions" ) ] = actionManageSessions;
//     m_actionCollection[ QLatin1String( "actionSaveSessionAs" ) ] = actionSaveSessionAs;
    m_actionCollection[ QLatin1String( "actionConfigure" ) ] = actionConfigure;
    m_actionCollection[ QLatin1String( "actionCompact" ) ] = actionCompact;
    m_actionCollection[ QLatin1String( "actionWide" ) ] = actionWide;
    m_actionCollection[ QLatin1String( "miniPlayer" ) ] = miniPlayer;
    m_actionCollection[ QLatin1String( "miniPlayer" ) ] = miniPlayer;
    m_actionCollection[ QLatin1String( "actionOpenTorrent" ) ] = actionOpenTorrent;
    m_actionCollection[ QLatin1String( "actionStopDownloads" ) ] = actionStopDownloads;
    m_actionCollection[ QLatin1String( "actionRemoveFailed" ) ] = actionRemoveFailed;
    m_actionCollection[ QLatin1String( "actionClearDownloadList" ) ] = actionClearDownloadList;
    m_actionCollection[ QLatin1String( "actionNewPlaylist" ) ] = actionNewPlaylist;
    m_actionCollection[ QLatin1String( "actionRenamePlaylist" ) ] = actionRenamePlaylist;
    m_actionCollection[ QLatin1String( "actionRemovePlaylist" ) ] = actionRemovePlaylist;
    m_actionCollection[ QLatin1String( "actionEmptyCache" ) ] = actionEmptyCache;
    m_actionCollection[ QLatin1String( "actionAbout" ) ] = actionAbout;
    m_actionCollection[ QLatin1String( "actionQtAbout" ) ] = actionQtAbout;
    
    // set action roles
    m_actionCollection[ QLatin1String( "actionClose" ) ]->setMenuRole( QAction::QuitRole );
    m_actionCollection[ QLatin1String( "actionQtAbout" ) ]->setMenuRole( QAction::AboutQtRole );
    
    // set actions tool tip
    actionClose->setToolTip( tr( "Close GrooveOff" ) );
    actionDonate->setToolTip( tr( "Donate with PayPal" ) );
//     actionManageSessions->setToolTip( tr( "Open Session Manager" ) );
//     actionSaveSessionAs->setToolTip( tr( "Save Session As..." ) );
    actionOpenTorrent->setToolTip( tr( "Add a .torrent file as a playlist" ) );
    actionClearDownloadList->setToolTip( tr( "Empties the list, leaving the catalogue alone" ) );
    actionEmptyCache->setToolTip( tr( "Deletes the downloaded data, leaving the playlists alone" ) );
    actionNewPlaylist->setToolTip( tr( "A list of your own, filled by dragging tracks into it" ) );
    
    // set actions shortcuts
    actionClose->setShortcut( QKeySequence( Qt::CTRL | Qt::Key_Q ) );
    actionOpenTorrent->setShortcut( QKeySequence( Qt::CTRL | Qt::Key_O ) );
    actionNewPlaylist->setShortcut( QKeySequence( Qt::CTRL | Qt::Key_N ) );
}

QMenuBar* ActionCollection::createMenuBar( QWidget *parent )
{
    QMenuBar* menuBar = new QMenuBar( parent );

//     QMenu* sessionsMenu = new QMenu( tr( "&Load Session" ), menuBar );
//     m_menuCollection[ QLatin1String( "sessionsMenu" ) ] = sessionsMenu;

    // The tracks of the list that is open, as one entry each: it is a way of
    // jumping to a track without the mouse, not a list of playlists. It used to
    // be called the playlist menu, back when there was only ever one list.
    QMenu* tracksMenu = new QMenu( tr( "&Tracks" ), menuBar );
    tracksMenu->setIcon( QIcon( QLatin1String( ":/resources/Playlist.png" ) ) );
    m_menuCollection[ QLatin1String( "tracksMenu" ) ] = tracksMenu;

    QMenu* playlistsMenu = new QMenu( tr( "&Playlists" ), menuBar );
    playlistsMenu->addAction( m_actionCollection[ QLatin1String( "actionNewPlaylist" ) ] );
    playlistsMenu->addSeparator();
    playlistsMenu->addAction( m_actionCollection[ QLatin1String( "actionRenamePlaylist" ) ] );
    playlistsMenu->addAction( m_actionCollection[ QLatin1String( "actionRemovePlaylist" ) ] );
    m_menuCollection[ QLatin1String( "playlistsMenu" ) ] = playlistsMenu;

    QMenu* fileMenu = new QMenu( tr( "&File" ), menuBar );
    fileMenu->addAction( m_actionCollection[ QLatin1String( "actionOpenTorrent" ) ] );
    fileMenu->addSeparator();
    // FIXME
    // fileMenu->addMenu(sessionsMenu);
    // fileMenu->addSeparator();
    // fileMenu->addAction( m_actionCollection[ QLatin1String( "actionSaveSessionAs" ) ] );
    // fileMenu->addAction( m_actionCollection[ QLatin1String( "actionManageSessions" ) ] );
    // fileMenu->addSeparator();
    
    fileMenu->addMenu(tracksMenu);
    fileMenu->addSeparator();
    
    fileMenu->addAction( m_actionCollection[ QLatin1String( "actionDonate" ) ] );
    fileMenu->addSeparator();
    fileMenu->addAction( m_actionCollection[ QLatin1String( "actionClose" ) ] );
    m_menuCollection[ QLatin1String( "fileMenu" ) ] = fileMenu;

    QMenu* downloadsMenu = new QMenu( tr( "&Downloads" ), menuBar );
    downloadsMenu->addAction( m_actionCollection[ QLatin1String( "actionStopDownloads" ) ] );
    downloadsMenu->addSeparator();
    downloadsMenu->addAction( m_actionCollection[ QLatin1String( "actionRemoveFailed" ) ] );
    downloadsMenu->addAction( m_actionCollection[ QLatin1String( "actionClearDownloadList" ) ] );
    m_menuCollection[ QLatin1String( "downloadsMenu" ) ] = downloadsMenu;

    QActionGroup *alignmentGroup = new QActionGroup(this);
    alignmentGroup->addAction( m_actionCollection[ QLatin1String( "actionCompact" ) ] );
    alignmentGroup->addAction( m_actionCollection[ QLatin1String( "actionWide" ) ] );
    alignmentGroup->addAction( m_actionCollection[ QLatin1String( "miniPlayer" ) ] );
    QMenu* viewMenu = new QMenu( tr( "&View" ), menuBar );
    viewMenu->addAction( m_actionCollection[ QLatin1String( "actionCompact" ) ] );
    viewMenu->addAction( m_actionCollection[ QLatin1String( "actionWide" ) ] );
    viewMenu->addAction( m_actionCollection[ QLatin1String( "miniPlayer" ) ] );
    m_menuCollection[ QLatin1String( "viewMenu" ) ] = viewMenu;

    QMenu* settingsMenu = new QMenu( tr( "&Settings" ), menuBar );
    settingsMenu->addAction( m_actionCollection[ QLatin1String( "actionEmptyCache" ) ] );
    settingsMenu->addSeparator();
    settingsMenu->addAction( m_actionCollection[ QLatin1String( "actionConfigure" ) ] );
    m_menuCollection[ QLatin1String( "settingsMenu" ) ] = settingsMenu;

    QMenu* helpMenu = new QMenu( tr( "&Help" ), menuBar );
    helpMenu->addAction( m_actionCollection[ QLatin1String( "actionAbout" ) ] );
    helpMenu->addAction( m_actionCollection[ QLatin1String( "actionQtAbout" ) ] );
    m_menuCollection[ QLatin1String( "helpMenu" ) ] = helpMenu;

    menuBar->addMenu( fileMenu );
    menuBar->addMenu( playlistsMenu );
    menuBar->addMenu( downloadsMenu );
    menuBar->addMenu( viewMenu );
    menuBar->addMenu( settingsMenu );
    menuBar->addMenu( helpMenu );

    return menuBar;
}

/*!
 \brief A small menu used with mini player layout
 */
QMenu* ActionCollection::createCompactMenu( QWidget *parent )
{
    QMenu* compactMenu = new QMenu( tr( "Main Menu" ), parent );

//     compactMenu->addAction( m_actionCollection[ QLatin1String( "actionNewToken" ) ] );
//     compactMenu->addSeparator();
//     compactMenu->addAction( m_actionCollection[ QLatin1String( "actionStopDownloads" ) ] );
//     compactMenu->addAction( m_actionCollection[ QLatin1String( "actionRemoveFailed" ) ] );
//     compactMenu->addAction( m_actionCollection[ QLatin1String( "actionClearDownloadList" ) ] );
//     compactMenu->addSeparator();
    compactMenu->addMenu( m_menuCollection[ QLatin1String( "tracksMenu" ) ] );
    compactMenu->addMenu( m_menuCollection[ QLatin1String( "playlistsMenu" ) ] );
    compactMenu->addSeparator();
    compactMenu->addAction( m_actionCollection[ QLatin1String( "actionCompact" ) ] );
    compactMenu->addAction( m_actionCollection[ QLatin1String( "actionWide" ) ] );
    compactMenu->addAction( m_actionCollection[ QLatin1String( "miniPlayer" ) ] );
    compactMenu->addSeparator();
    compactMenu->addAction( m_actionCollection[ QLatin1String( "actionEmptyCache" ) ] );
    compactMenu->addAction( m_actionCollection[ QLatin1String( "actionConfigure" ) ] );
    compactMenu->addSeparator();
    compactMenu->addAction( m_actionCollection[ QLatin1String( "actionAbout" ) ] );
    compactMenu->addAction( m_actionCollection[ QLatin1String( "actionDonate" ) ] );
    compactMenu->addSeparator();
    compactMenu->addAction( m_actionCollection[ QLatin1String( "actionClose" ) ] );

    return compactMenu;
}

/*!
 \brief returns action from its name
 */
QAction* ActionCollection::getAction( const QString& name )
{
    return m_actionCollection.value( name, 0 );
}

/*!
 \brief return menu from its name
 */
QMenu* ActionCollection::getMenu(const QString& name)
{
    return m_menuCollection.value( name, 0 );
}

