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

#include <QTextCodec>
#include <QLibraryInfo>
#include <QTranslator>

#define SHARE_PATH "/../share/apps/grooveoff"

App::App( int & argc, char ** argv ):
    QtSingleApplication( argc, argv )
{

    if(isRunning())
        return;

    // set application info
    QCoreApplication::setOrganizationName( QLatin1String( "gcala" ) );
    QCoreApplication::setOrganizationDomain( QLatin1String( "gcala.blogger.com" ) );
    setApplicationName( QLatin1String( "grooveoff" ) );

#if QT_VERSION < QT_VERSION_CHECK( 5, 0, 0 )
    // Set the codec to UTF-8
    QTextCodec::setCodecForLocale( QTextCodec::codecForName( "UTF-8" ) );
    QTextCodec::setCodecForCStrings( QTextCodec::codecForName( "UTF-8" ) );
#endif

    QTranslator appTranslator; // application translator
    QTranslator qtTranslator;  // qt translator
    QStringList uiLanguages;   // enabled languages
    // uiLanguages crashes on Windows with 4.8.0 release builds
#if ( QT_VERSION >= 0x040801 ) || ( QT_VERSION >= 0x040800 && !defined( Q_OS_WIN ) )
    uiLanguages = QLocale::system().uiLanguages();
#else
    uiLanguages << QLocale::system().name();
#endif

    // the path to translations
    const QString &appTrPath = QCoreApplication::applicationDirPath()
            + QLatin1String( SHARE_PATH "/translations" );

    foreach (QString locale, uiLanguages) {
#if (QT_VERSION >= 0x050000)
        locale = QLocale(locale).name();
#else
        locale.replace( QLatin1Char( '-' ), QLatin1Char( '_' ) ); // work around QTBUG-25973
#endif
        if ( appTranslator.load( QLatin1String( "grooveoff_" ) + locale, appTrPath ) ) {
            installTranslator(&appTranslator);
            
            // the path to qt translations
            const QString &qtTrPath = QLibraryInfo::location( QLibraryInfo::TranslationsPath );
            // translation file name
            const QString &qtTrFile = QLatin1String( "qt_" ) + locale;
            // Binary installer puts Qt tr files into creatorTrPath
            if (qtTranslator.load( qtTrFile, qtTrPath ) || qtTranslator.load( qtTrFile, appTrPath ) ) {
                installTranslator( &qtTranslator );
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

    // application icon
    setWindowIcon( QIcon( QLatin1String( ":/resources/grooveoff.png" ) ) );

    // Creating MainWindow
    m_mainWindow = new MainWindow();

    //DON'T DELETE THIS NEXT LINE or the app crashes when you click the X (unless we reimplement closeEvent)
    //Reason: in ~App we have to call the deleteBrowsers method or else we run afoul of refcount foobar in KHTMLPart
    //But if you click the X (not Action->Quit) it automatically kills MainWindow because KMainWindow sets this
    //for us as default (bad KMainWindow)
    mainWindow()->setAttribute( Qt::WA_DeleteOnClose, false );

    mainWindow()->show();
}

App::~App()
{
    delete mainWindow();
    delete The::audioEngine();
    delete The::playlist();
    delete The::actionCollection();
}

void App::quit()
{
    emit prepareToQuit();
    QApplication::quit();
}
