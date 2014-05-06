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


#ifndef ACTIONCOLLECTION_H
#define ACTIONCOLLECTION_H

#include <QObject>
#include <QHash>

class QMenuBar;
class QMenu;
class QAction;

class ActionCollection;

namespace The {
    ActionCollection* actionCollection();
}

class ActionCollection : public QObject
{
    Q_OBJECT
public:
    friend ActionCollection* The::actionCollection();
    
    ~ActionCollection();

    void initActions();

    QMenuBar *createMenuBar( QWidget *parent );
    QMenu    *createCompactMenu( QWidget *parent );
    QAction  *getAction( const QString& name );
    QMenu    *getMenu( const QString& name );

private:
    ActionCollection();

    QHash< QString, QAction* > m_actionCollection;
    QHash< QString, QMenu*   > m_menuCollection;
};

#endif // ACTIONCOLLECTION_H
