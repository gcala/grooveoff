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

#include <QtSingleApplication>

class App : public QtSingleApplication
{
    Q_OBJECT
public:
    App( int & argc, char ** argv);
    ~App();

    static App *instance() { return static_cast<App*>( qApp ); }

    inline MainWindow *mainWindow() const { return m_mainWindow.data(); }

    // FRIENDS
    friend class MainWindow; //requires access to applySettings()

Q_SIGNALS:
    void prepareToQuit();
    void settingsChanged();

public Q_SLOTS:
    void applySettings( bool firstTime = false );
    void slotConfigGrooveOff( const QString& page = QString() );
    void quit();

private:
    QWeakPointer<MainWindow>    m_mainWindow;
};

#define pApp static_cast<App*>(qApp)

#endif // APP_H
