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


#include "Mpris2PluginRootAdaptor.h"

#include <QApplication>

Mpris2PluginRootAdaptor::Mpris2PluginRootAdaptor(QObject *parent) : QDBusAbstractAdaptor(parent)
{}


Mpris2PluginRootAdaptor::~Mpris2PluginRootAdaptor()
{}

bool Mpris2PluginRootAdaptor::canQuit() const
{
    return true;
}

bool Mpris2PluginRootAdaptor::canRaise() const
{
    return false;
}

QString Mpris2PluginRootAdaptor::desktopEntry() const
{
    return QString::fromLatin1( "grooveoff" );
}

bool Mpris2PluginRootAdaptor::hasTrackList() const
{
    return false;
}
QString Mpris2PluginRootAdaptor::identity() const
{
    return QString::fromLatin1( "GrooveOff" );
}

QStringList Mpris2PluginRootAdaptor::supportedMimeTypes() const
{
    return QStringList();
}

QStringList Mpris2PluginRootAdaptor::supportedUriSchemes() const
{
    QStringList uriSchemes;
    uriSchemes << QLatin1String( "grooveoff" );
    return uriSchemes;
}

void Mpris2PluginRootAdaptor::Quit()
{
    qApp->quit();
}

void Mpris2PluginRootAdaptor::Raise(){}
