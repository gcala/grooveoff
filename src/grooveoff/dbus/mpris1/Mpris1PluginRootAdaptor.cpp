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


#include "Mpris1PluginRootAdaptor.h"
#include "../config-version.h"

#include <QApplication>
#include <QDBusMetaType>
#include <QDBusArgument>

//register << operator
QDBusArgument &operator << ( QDBusArgument &arg, const Version &v )
{
    arg.beginStructure();
    arg << v.major;
    arg << v.minor;
    arg.endStructure();
    return arg;
}

//register >> operator
const QDBusArgument &operator >> ( const QDBusArgument &arg, Version &v )
{
    arg.beginStructure();
    arg >> v.major;
    arg >> v.minor;
    arg.endStructure();
    return arg;
}

Mpris1PluginRootAdaptor::Mpris1PluginRootAdaptor( QObject *parent )
    : QObject( parent )
{
    qDBusRegisterMetaType< Version > ();
}


Mpris1PluginRootAdaptor::~Mpris1PluginRootAdaptor()
{
}

QString Mpris1PluginRootAdaptor::Identity()
{
    QString name = QString( "GrooveOff %1" ).arg( GROOVEOFF_VERSION );
    return name;
}

Version Mpris1PluginRootAdaptor::MprisVersion()
{
    struct Version v;
    v.major = 1;
    v.minor = 0;
    return v;
}

void Mpris1PluginRootAdaptor::Quit()
{
    qApp->quit();
}
