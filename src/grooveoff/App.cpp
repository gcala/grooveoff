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


#include "App.h"
#include "AudioEngine.h"
#include "Playlist.h"
#include "ActionCollection.h"
#include "Settings.h"
#include "Utility.h"
#include "../db/Catalog.h"
#include "../db/Database.h"
#include "../db/PlaylistStore.h"
#include "../torrent/CacheManager.h"
#include "../torrent/StreamServer.h"
#include "../torrent/TorrentSession.h"

#include <QDebug>
#include <QDir>
#include <QLibraryInfo>
#include <QSettings>
#include <QStandardPaths>
#include <QTranslator>

App::App( int & argc, char ** argv ):
    QApplication( argc, argv )
    , m_appTranslator( new QTranslator( this ) )
    , m_qtTranslator( new QTranslator( this ) )
{
    // set application info: it has to happen before anything else, the single
    // instance socket name is derived from it
    QCoreApplication::setOrganizationName( QLatin1String( "gcala" ) );
    QCoreApplication::setOrganizationDomain( QLatin1String( "gcala.blogger.com" ) );
    setApplicationName( QLatin1String( "grooveoff" ) );

    loadTranslations();

    // application icon
    setWindowIcon( QIcon( QLatin1String( ":/resources/grooveoff.png" ) ) );
}

/*!
  \brief loadTranslations: the interface in the language of the system

  The catalogues are compiled into the binary under :/i18n by
  qt_add_translations, so there is no path to build and nothing to find. What
  this replaces walked uiLanguages by hand against a directory derived from
  applicationDirPath, which answered nothing at all when the application was run
  from anywhere but its install prefix.

  Nothing is done when there is no catalogue for the language, and that is not
  an error: the strings in the source are English, so English is what shows.
*/
void App::loadTranslations()
{
    const QLocale locale;

    if( m_appTranslator->load( locale, QLatin1String( "grooveoff" ), QLatin1String( "_" ),
                               QLatin1String( ":/i18n" ) ) ) {
        installTranslator( m_appTranslator );
        qDebug() << "GrooveOff ::" << "interface language" << m_appTranslator->language();
    } else {
        // Worth saying out loud: "the interface is in English" is the one report
        // that has no other symptom to go on.
        qDebug() << "GrooveOff ::" << "no translation for" << locale.name() << "- using the source strings";
    }

    // Qt's own strings — the buttons of the file dialog, the standard messages
    // — come from where Qt installed them, and are missing rather than wrong
    // when the system has no translations package.
    if( m_qtTranslator->load( locale, QLatin1String( "qt" ), QLatin1String( "_" ),
                              QLibraryInfo::path( QLibraryInfo::TranslationsPath ) ) )
        installTranslator( m_qtTranslator );
}

void App::start()
{
    startTorrentEngine();

    // Creating MainWindow
    m_mainWindow = new MainWindow();

    //DON'T DELETE THIS NEXT LINE or the app crashes when you click the X (unless we reimplement closeEvent)
    //Reason: in ~App we have to call the deleteBrowsers method or else we run afoul of refcount foobar in KHTMLPart
    //But if you click the X (not Action->Quit) it automatically kills MainWindow because KMainWindow sets this
    //for us as default (bad KMainWindow)
    mainWindow()->setAttribute( Qt::WA_DeleteOnClose, false );

    mainWindow()->show();
}

/*!
  \brief raiseWindow: bring the window to the front, on behalf of a second instance
*/
void App::raiseWindow()
{
    if( !m_mainWindow )
        return;

    mainWindow()->show();
    mainWindow()->raise();
    mainWindow()->activateWindow();
}

/*!
  \brief startTorrentEngine: brings up the session and the local stream server

  Both are needed before the window opens: the list is built from the database,
  but the row of the track that was playing has to know whether its torrent is
  awake, and the player is handed a url from the server rather than a path.
*/
void App::startTorrentEngine()
{
    const QString &cacheRoot = QStandardPaths::writableLocation( QStandardPaths::CacheLocation );

    if( !The::torrentSession()->start( cacheRoot ) ) {
        qDebug() << "GrooveOff ::" << "the torrent session did not start; nothing will play";
        return;
    }

    // One layout, one owner: the session derives the cache paths from the root
    // and everything else asks it, rather than building the same strings again.
    Utility::torrentsCachePath = The::torrentSession()->torrentsPath() + QDir::separator();
    Utility::resumeCachePath   = The::torrentSession()->resumePath() + QDir::separator();

    if( !The::streamServer()->start() )
        qDebug() << "GrooveOff ::" << "the stream server did not start; nothing will play";
}

App::~App()
{
    // The engine goes first, while the window is still there to hear it: the
    // shutdown saves the resume data of every torrent, and it is the window
    // that writes what came back into the catalogue.
    The::streamServer()->stop();

    emptyCacheIfAsked();

    The::torrentSession()->shutdown();

    delete mainWindow();
    delete The::audioEngine();
    delete The::playlist();
    delete The::actionCollection();

    delete The::cacheManager();
    delete The::streamServer();
    delete The::torrentSession();

    // Persistence goes last: the window writes what was playing on its way out.
    delete The::playlistStore();
    delete The::catalog();
    delete The::database();
}

/*!
  \brief emptyCacheIfAsked: throws the cache away on the way out, if asked to

  Off by default: a cache that survives is the difference between pressing play
  and hearing something, and pressing play and waiting for a swarm. The setting
  exists for metered connections and shared machines, where the trade goes the
  other way.

  It runs before the session is shut down, so that libtorrent deletes the files
  of the torrents it holds itself, and after the stream server has stopped, so
  that nothing is reading them. Nothing is playing at this point either, which
  is why the torrent being listened to — normally spared — is spared from
  nothing here.
*/
void App::emptyCacheIfAsked()
{
    QSettings settings;
    if( !settings.value( Settings::emptyCacheOnExit, Settings::emptyCacheOnExitDefault ).toBool() )
        return;

    The::cacheManager()->setPlayingTorrent( QString() );
    The::cacheManager()->emptyCache();
}

void App::quit()
{
    emit prepareToQuit();
    QApplication::quit();
}
