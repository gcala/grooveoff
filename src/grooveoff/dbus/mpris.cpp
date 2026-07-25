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


#include "mpris2/Mpris2PluginRootAdaptor.h"
#include "mpris2/Mpris2PluginPlayerAdaptor.h"
#include "mpris.h"

#include <QDBusConnection>

Mpris::Mpris(QObject *parent) : QObject( parent )
{
    QDBusConnection connection = QDBusConnection::sessionBus();

    new Mpris2PluginRootAdaptor( this );
    new Mpris2PluginPlayerAdaptor( this );
    connection.registerObject( QLatin1String( "/org/mpris/MediaPlayer2" ), this );
    connection.registerService( QLatin1String( "org.mpris.MediaPlayer2.grooveoff" ) );
}

Mpris::~Mpris()
{
    QDBusConnection::sessionBus().unregisterService( QLatin1String( "org.mpris.MediaPlayer2.grooveoff" ) );
}
