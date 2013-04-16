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

#include <QtGui/QApplication>
#include <QTranslator>
#include <QLibraryInfo>
#include <QLocale>
#include <QTextCodec>
#include "grooveoff/mainwindow.h"

int main(int argc, char** argv)
{
    QApplication app(argc, argv);
    QCoreApplication::setOrganizationName(QLatin1String("gcala"));
    QCoreApplication::setOrganizationDomain(QLatin1String("gcala.blogger.com"));
    app.setApplicationName(QLatin1String("grooveoff"));

    QTextCodec::setCodecForLocale(QTextCodec::codecForName("UTF-8"));
    QTextCodec::setCodecForCStrings(QTextCodec::codecForName("UTF-8"));

    QTranslator qtTranslator;
    qtTranslator.load(QLatin1String("qt_") + QLocale::system().name(),
                      QLibraryInfo::location(QLibraryInfo::TranslationsPath));
    app.installTranslator(&qtTranslator);

    QLatin1String localeSuffix("/translations");
    QString localeName(QLatin1String("grooveoff_") + QLocale::system().name());

    QTranslator appSystemTranslator;
    appSystemTranslator.load(localeName, QLatin1String("/usr/share/grooveoff") + localeSuffix);
    app.installTranslator(&appSystemTranslator);

    app.setWindowIcon(QIcon(QLatin1String(":/resources/grooveoff.png")));

    MainWindow w;
    w.show();
    return app.exec();
}
