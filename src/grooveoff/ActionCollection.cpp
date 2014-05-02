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

void ActionCollection::initActions()
{
    QAction *actionClose = new QAction(QIcon::fromTheme(QLatin1String("application-exit"),
                               QIcon(QLatin1String(":/resources/application-exit.png"))),
                               trUtf8("&Exit"), this);
    actionClose->setToolTip(trUtf8("Close GrooveOff"));
    actionClose->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_Q));
    m_actionCollection[ QLatin1String( "actionClose" ) ] = actionClose;
    m_actionCollection[ QLatin1String( "actionClose" ) ]->setMenuRole( QAction::QuitRole );

    QAction *actionDonate = new QAction(QIcon::fromTheme(QLatin1String("help-donate"),
                                QIcon(QLatin1String(":/resources/help-donate.png"))),
                                trUtf8("&Donate"), this);
    actionDonate->setToolTip(trUtf8("Donate with PayPal"));
    m_actionCollection[ QLatin1String( "actionDonate" ) ] = actionDonate;

    QAction *actionManageSessions = new QAction(QIcon::fromTheme(QLatin1String("edit-rename"),
                                QIcon(QLatin1String(":/resources/edit-rename.png"))),
                                trUtf8("&Manage Sessions"), this);
    actionManageSessions->setToolTip(trUtf8("Open Session Manager"));
    m_actionCollection[ QLatin1String( "actionManageSessions" ) ] = actionManageSessions;

    QAction *actionSaveSessionAs = new QAction(QIcon::fromTheme(QLatin1String("document-save-as"),
                                QIcon(QLatin1String(":/resources/document-save-as.png"))),
                                trUtf8("&Save Session As..."), this);
    actionSaveSessionAs->setToolTip(trUtf8("Save Session As..."));
    m_actionCollection[ QLatin1String( "actionSaveSessionAs" ) ] = actionSaveSessionAs;

    QAction *actionConfigure = new QAction(trUtf8("&Configure GrooveOff..."), this);
    if(QIcon::hasThemeIcon(QLatin1String("configure")))
        actionConfigure->setIcon(QIcon::fromTheme(QLatin1String("configure")));
    else
        actionConfigure->setIcon(QIcon::fromTheme(QLatin1String("gconf-editor"),
                                  QIcon(QLatin1String(":/resources/configure.png"))));
    m_actionCollection[ QLatin1String( "actionConfigure" ) ] = actionConfigure;

    QAction *actionCompact = new QAction(QIcon::fromTheme(QLatin1String("view-split-top-bottom"),
                                 QIcon(QLatin1String(":/resources/view-split-top-bottom.png"))),
                                 trUtf8("&Compact Layout"), this);
    actionCompact->setCheckable(true);
    m_actionCollection[ QLatin1String( "actionCompact" ) ] = actionCompact;

    QAction *actionWide = new QAction(QIcon::fromTheme(QLatin1String("view-split-left-right"),
                              QIcon(QLatin1String(":/resources/view-split-left-right"))),
                              trUtf8("&Wide Layout"), this);
    actionWide->setCheckable(true);
    m_actionCollection[ QLatin1String( "actionWide" ) ] = actionWide;

    QAction *miniPlayer = new QAction(QIcon::fromTheme(QLatin1String("view-split-left-right"),
                              QIcon(QLatin1String(":/resources/view-split-left-right"))),
                              trUtf8("&Mini Player"), this);
    miniPlayer->setCheckable(true);
    m_actionCollection[ QLatin1String( "miniPlayer" ) ] = miniPlayer;

    QAction *actionNewToken = new QAction(QIcon::fromTheme(QLatin1String("emblem-new"),
                                  QIcon(QLatin1String(":/resources/emblem-new.png"))),
                                  trUtf8("&Get new token..."), this);
    m_actionCollection[ QLatin1String( "actionNewToken" ) ] = actionNewToken;

    QAction *actionStopDownloads = new QAction(QIcon::fromTheme(QLatin1String("process-stop"),
                                      QIcon(QLatin1String(":/resources/process-stop.png"))),
                                      trUtf8("&Stop all downloads"), this);
    m_actionCollection[ QLatin1String( "actionStopDownloads" ) ] = actionStopDownloads;

    QAction *actionRemoveFailed = new QAction(QIcon::fromTheme(QLatin1String("edit-delete"),
                                      QIcon(QLatin1String(":/resources/edit-delete.png"))),
                                      trUtf8("&Remove canceled/failed transfers"), this);
    m_actionCollection[ QLatin1String( "actionRemoveFailed" ) ] = actionRemoveFailed;

    QAction *actionClearDownloadList = new QAction(QIcon::fromTheme(QLatin1String("edit-clear"),
                                           QIcon(QLatin1String(":/resources/edit-clear"))),
                                           trUtf8("Clear finished"), this);
    actionClearDownloadList->setToolTip(trUtf8("Removes all finished transfers and leaves all files on disk"));
    m_actionCollection[ QLatin1String( "actionClearDownloadList" ) ] = actionClearDownloadList;

    QAction *actionAbout = new QAction(QIcon::fromTheme(QLatin1String("help-about"),
                               QIcon(QLatin1String(":/resources/help-about.png"))),
                               trUtf8("&About GrooveOff"), this);
    m_actionCollection[ QLatin1String( "actionAbout" ) ] = actionAbout;

    QAction *actionQtAbout = new QAction(QIcon::fromTheme(QLatin1String("qtlogo"),
                                 QIcon(QLatin1String(":/resources/qtlogo.png"))),
                                 trUtf8("About &Qt"), this);
    m_actionCollection[ QLatin1String( "actionQtAbout" ) ] = actionQtAbout;
    m_actionCollection[ QLatin1String( "actionQtAbout" ) ]->setMenuRole( QAction::AboutQtRole );
}

QMenuBar* ActionCollection::createMenuBar( QWidget *parent )
{
    QMenuBar* menuBar = new QMenuBar( parent );

    QMenu* sessionsMenu = new QMenu( trUtf8( "&Load Session" ), menuBar );
    m_menuCollection[ QLatin1String( "sessionsMenu" ) ] = sessionsMenu;

    QMenu* fileMenu = new QMenu( trUtf8( "&File" ), menuBar );
    fileMenu->addAction( m_actionCollection[ QLatin1String( "actionNewToken" ) ] );
    fileMenu->addSeparator();
    // FIXME
    // fileMenu->addMenu(sessionsMenu);
    // fileMenu->addSeparator();
    // fileMenu->addAction( m_actionCollection[ QLatin1String( "actionSaveSessionAs" ) ] );
    // fileMenu->addAction( m_actionCollection[ QLatin1String( "actionManageSessions" ) ] );
    // fileMenu->addSeparator();
    fileMenu->addAction( m_actionCollection[ QLatin1String( "actionDonate" ) ] );
    fileMenu->addSeparator();
    fileMenu->addAction( m_actionCollection[ QLatin1String( "actionClose" ) ] );
    m_menuCollection[ QLatin1String( "fileMenu" ) ] = fileMenu;

    QMenu* downloadsMenu = new QMenu( trUtf8( "&Downloads" ), menuBar );
    downloadsMenu->addAction( m_actionCollection[ QLatin1String( "actionStopDownloads" ) ] );
    downloadsMenu->addSeparator();
    downloadsMenu->addAction( m_actionCollection[ QLatin1String( "actionRemoveFailed" ) ] );
    downloadsMenu->addAction( m_actionCollection[ QLatin1String( "actionClearDownloadList" ) ] );
    m_menuCollection[ QLatin1String( "downloadsMenu" ) ] = downloadsMenu;

    QActionGroup *alignmentGroup = new QActionGroup(this);
    alignmentGroup->addAction( m_actionCollection[ QLatin1String( "actionCompact" ) ] );
    alignmentGroup->addAction( m_actionCollection[ QLatin1String( "actionWide" ) ] );
    alignmentGroup->addAction( m_actionCollection[ QLatin1String( "miniPlayer" ) ] );
    QMenu* viewMenu = new QMenu( trUtf8( "&View" ), menuBar );
    viewMenu->addAction( m_actionCollection[ QLatin1String( "actionCompact" ) ] );
    viewMenu->addAction( m_actionCollection[ QLatin1String( "actionWide" ) ] );
    viewMenu->addAction( m_actionCollection[ QLatin1String( "miniPlayer" ) ] );
    m_menuCollection[ QLatin1String( "viewMenu" ) ] = viewMenu;

    QMenu* settingsMenu = new QMenu( trUtf8( "&Settings" ), menuBar );
    settingsMenu->addAction( m_actionCollection[ QLatin1String( "actionConfigure" ) ] );
    m_menuCollection[ QLatin1String( "settingsMenu" ) ] = settingsMenu;

    QMenu* helpMenu = new QMenu( trUtf8( "&Help" ), menuBar );
    helpMenu->addAction( m_actionCollection[ QLatin1String( "actionAbout" ) ] );
    helpMenu->addAction( m_actionCollection[ QLatin1String( "actionQtAbout" ) ] );
    m_menuCollection[ QLatin1String( "helpMenu" ) ] = helpMenu;

    menuBar->addMenu( fileMenu );
    menuBar->addMenu( downloadsMenu );
    menuBar->addMenu( viewMenu );
    menuBar->addMenu( settingsMenu );
    menuBar->addMenu( helpMenu );

    return menuBar;
}


QMenu* ActionCollection::createCompactMenu( QWidget *parent )
{
    QMenu* compactMenu = new QMenu( tr( "Main Menu" ), parent );

    compactMenu->addAction( m_actionCollection[ QLatin1String( "actionNewToken" ) ] );
    compactMenu->addSeparator();
    compactMenu->addAction( m_actionCollection[ QLatin1String( "actionStopDownloads" ) ] );
    compactMenu->addAction( m_actionCollection[ QLatin1String( "actionRemoveFailed" ) ] );
    compactMenu->addAction( m_actionCollection[ QLatin1String( "actionClearDownloadList" ) ] );
    compactMenu->addSeparator();
    compactMenu->addAction( m_actionCollection[ QLatin1String( "actionCompact" ) ] );
    compactMenu->addAction( m_actionCollection[ QLatin1String( "actionWide" ) ] );
    compactMenu->addAction( m_actionCollection[ QLatin1String( "miniPlayer" ) ] );
    compactMenu->addSeparator();
    compactMenu->addAction( m_actionCollection[ QLatin1String( "actionConfigure" ) ] );
    compactMenu->addSeparator();
    compactMenu->addAction( m_actionCollection[ QLatin1String( "actionAbout" ) ] );
    compactMenu->addAction( m_actionCollection[ QLatin1String( "actionDonate" ) ] );
    compactMenu->addSeparator();
    compactMenu->addAction( m_actionCollection[ QLatin1String( "actionClose" ) ] );

    return compactMenu;
}


QAction* ActionCollection::getAction( const QString& name )
{
    return m_actionCollection.value( name, 0 );
}

QMenu* ActionCollection::getMenu(const QString& name)
{
    return m_menuCollection.value( name, 0 );
}

