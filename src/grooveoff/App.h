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


#ifndef APP_H
#define APP_H

#include "MainWindow.h"

#include <QApplication>
#include <QPointer>

class QTranslator;

class App : public QApplication
{
    Q_OBJECT
public:
    App( int & argc, char ** argv);
    ~App();

    static App *instance() {
        return static_cast<App*>( qApp );
    }

    inline MainWindow *mainWindow() const {
        return m_mainWindow.data();
    }

    /**
     * Builds and shows the main window. Kept out of the constructor so that
     * a second instance can hand over and quit before any window is created.
     */
    void start();

    /**
     * Brings the main window to the front, on request of a second instance
     */
    void raiseWindow();

    // FRIENDS
    friend class MainWindow;

Q_SIGNALS:
    void prepareToQuit();

public Q_SLOTS:
    void quit();

private:
    void loadTranslations();

    /** starts the torrent session and the local stream server, in that order */
    void startTorrentEngine();

    /** honours emptyCacheOnExit, which is off unless somebody turned it on */
    void emptyCacheIfAsked();

    QPointer<MainWindow>    m_mainWindow;
    QTranslator            *m_appTranslator;
    QTranslator            *m_qtTranslator;
};

#define pApp static_cast<App*>(qApp)

#endif // APP_H
