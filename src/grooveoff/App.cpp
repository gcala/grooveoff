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

#include <QTextCodec>
#include <QLibraryInfo>
#include <QTranslator>

App::App( int & argc, char ** argv):
    QtSingleApplication(argc,argv)
{

    if(isRunning())
        return;

    QCoreApplication::setOrganizationName(QLatin1String("gcala"));
    QCoreApplication::setOrganizationDomain(QLatin1String("gcala.blogger.com"));
    setApplicationName(QLatin1String("grooveoff"));

#if QT_VERSION < QT_VERSION_CHECK( 5, 0, 0 )
    QTextCodec::setCodecForLocale(QTextCodec::codecForName("UTF-8"));
    QTextCodec::setCodecForCStrings(QTextCodec::codecForName("UTF-8"));
#endif

    QTranslator qtTranslator;
    qtTranslator.load(QLatin1String("qt_") + QLocale::system().name(),
                      QLibraryInfo::location(QLibraryInfo::TranslationsPath));
    installTranslator(&qtTranslator);

    QLatin1String localeSuffix("/translations");
    QString localeName(QLatin1String("grooveoff_") + QLocale::system().name());

    QTranslator appSystemTranslator;
    appSystemTranslator.load(localeName, QLatin1String("/usr/share/grooveoff") + localeSuffix);
    installTranslator(&appSystemTranslator);

    setWindowIcon(QIcon(QLatin1String(":/resources/grooveoff.png")));

    // Creating MainWindow
    m_mainWindow = new MainWindow();

    // Creating DBus handlers


    //DON'T DELETE THIS NEXT LINE or the app crashes when you click the X (unless we reimplement closeEvent)
    //Reason: in ~App we have to call the deleteBrowsers method or else we run afoul of refcount foobar in KHTMLPart
    //But if you click the X (not Action->Quit) it automatically kills MainWindow because KMainWindow sets this
    //for us as default (bad KMainWindow)
    mainWindow()->setAttribute( Qt::WA_DeleteOnClose, false );

    // applySettings

    //The::engineController()->setVolume( AmarokConfig::masterVolume() );
    //The::engineController()->setMuted( AmarokConfig::muteState() );

    m_mainWindow.data()->show();
}

App::~App()
{
    delete mainWindow();
}

void App::applySettings(bool firstTime)
{

}

void App::quit()
{
    emit prepareToQuit();
    QApplication::quit();
}

void App::slotConfigGrooveOff(const QString& page)
{

}

