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


#ifndef MPRIS2PLUGINROOTADAPTOR_H
#define MPRIS2PLUGINROOTADAPTOR_H

#include <QDBusAbstractAdaptor>
#include <QStringList>

class Mpris2PluginRootAdaptor : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.mpris.MediaPlayer2")

    Q_PROPERTY(bool CanQuit READ canQuit)
    bool canQuit() const;

    Q_PROPERTY(bool CanRaise READ canRaise)
    bool canRaise() const;

    Q_PROPERTY(QString DesktopEntry READ desktopEntry)
    QString desktopEntry() const;

    Q_PROPERTY(bool HasTrackList READ hasTrackList)
    bool hasTrackList() const;

    Q_PROPERTY(QString Identity READ identity)
    QString identity() const;

    Q_PROPERTY(QStringList SupportedMimeTypes READ supportedMimeTypes)
    QStringList supportedMimeTypes() const;

    Q_PROPERTY(QStringList SupportedUriSchemes READ supportedUriSchemes)
    QStringList supportedUriSchemes() const;

public:
    Mpris2PluginRootAdaptor(QObject *parent = 0);
    virtual ~Mpris2PluginRootAdaptor();

public Q_SLOTS:
     void Quit();
     void Raise();
};

#endif
