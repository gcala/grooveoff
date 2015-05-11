/*
    GrooveOff - Offline Grooveshark.com music
    Copyright (C) 2013-2015  Giuseppe Calà <jiveaxe@gmail.com>

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

#include <QTextCodec>
#include <QLibraryInfo>
#include <QTranslator>

#define SHARE_PATH "/../share/apps/grooveoff"

int main( int argc, char** argv )
{
    App app( argc, argv );
    
#if QT_VERSION < QT_VERSION_CHECK( 5, 0, 0 )
    QTextCodec::setCodecForLocale( QTextCodec::codecForName( "UTF-8" ) );
    QTextCodec::setCodecForCStrings( QTextCodec::codecForName( "UTF-8" ) );
#endif

    QTranslator appTranslator;
    QTranslator qtTranslator;
    QStringList uiLanguages;
    // uiLanguages crashes on Windows with 4.8.0 release builds
#if ( QT_VERSION >= 0x040801 ) || ( QT_VERSION >= 0x040800 && !defined( Q_OS_WIN ) )
    uiLanguages = QLocale::system().uiLanguages();
#else
    uiLanguages << QLocale::system().name();
#endif

    const QString &appTrPath = QCoreApplication::applicationDirPath() + QLatin1String( SHARE_PATH "/translations" );

    foreach ( QString locale, uiLanguages ) {
#if ( QT_VERSION >= 0x050000 )
        locale = QLocale( locale ).name();
#else
        locale.replace( QLatin1Char( '-' ), QLatin1Char( '_' ) ); // work around QTBUG-25973
#endif
        if ( appTranslator.load( QLatin1String( "grooveoff_" ) + locale, appTrPath ) ) {
            app.installTranslator( &appTranslator );
            
            const QString &qtTrPath = QLibraryInfo::location( QLibraryInfo::TranslationsPath );
            const QString &qtTrFile = QLatin1String( "qt_" ) + locale;
            // Binary installer puts Qt tr files into creatorTrPath
            if ( qtTranslator.load( qtTrFile, qtTrPath ) || qtTranslator.load( qtTrFile, appTrPath ) ) {
                app.installTranslator( &qtTranslator );
                app.setProperty( "qtc_locale", locale );
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
    
    return app.exec();
}
