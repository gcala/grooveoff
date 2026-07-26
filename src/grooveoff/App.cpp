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
#include "Utility.h"
#include "../db/Catalog.h"
#include "../db/Database.h"
#include "../db/PlaylistStore.h"
#include "../torrent/StreamServer.h"
#include "../torrent/TorrentSession.h"

#include <QDebug>
#include <QDir>
#include <QLibraryInfo>
#include <QStandardPaths>
#include <QTranslator>

#include <utility>

#define SHARE_PATH "/../share/apps/grooveoff"

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

void App::loadTranslations()
{
    QStringList uiLanguages = QLocale::system().uiLanguages(); // enabled languages

    // the path to translations
    const QString &appTrPath = QCoreApplication::applicationDirPath()
            + QLatin1String( SHARE_PATH "/translations" );

    for (const QString &uiLanguage : std::as_const(uiLanguages)) {
        const QString locale = QLocale(uiLanguage).name();
        if ( m_appTranslator->load( QLatin1String( "grooveoff_" ) + locale, appTrPath ) ) {
            installTranslator( m_appTranslator );

            // the path to qt translations
            const QString &qtTrPath = QLibraryInfo::path( QLibraryInfo::TranslationsPath );
            // translation file name
            const QString &qtTrFile = QLatin1String( "qt_" ) + locale;
            // Binary installer puts Qt tr files into creatorTrPath
            if ( m_qtTranslator->load( qtTrFile, qtTrPath ) || m_qtTranslator->load( qtTrFile, appTrPath ) ) {
                installTranslator( m_qtTranslator );
                setProperty( "qtc_locale", locale );
            }
            break;
        } else if ( locale == QLatin1String( "C" ) /* overrideLanguage == "English" */) {
            // use built-in
            break;
        } else if ( locale.startsWith( QLatin1String( "en" ) ) /* "English" is built-in */) {
            // use built-in
            break;
        }
    }
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
    The::torrentSession()->shutdown();

    delete mainWindow();
    delete The::audioEngine();
    delete The::playlist();
    delete The::actionCollection();

    delete The::streamServer();
    delete The::torrentSession();

    // Persistence goes last: the window writes what was playing on its way out.
    delete The::playlistStore();
    delete The::catalog();
    delete The::database();
}

void App::quit()
{
    emit prepareToQuit();
    QApplication::quit();
}
