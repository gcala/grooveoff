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


#ifndef MPRIS1PLUGINROOTADAPTOR_H
#define MPRIS1PLUGINROOTADAPTOR_H

#include <QObject>
#include <QVariantMap>

struct Version
{
    quint16 major;
    quint16 minor;
};

Q_DECLARE_METATYPE( Version );

class Mpris1PluginRootAdaptor : public QObject
{
    Q_OBJECT
    Q_CLASSINFO( "D-Bus Interface", "org.freedesktop.MediaPlayer" )

public:
    Mpris1PluginRootAdaptor( QObject *parent = 0 );

    ~Mpris1PluginRootAdaptor();

public slots:
    QString Identity();
    Version MprisVersion();
    void Quit();
};

#endif
