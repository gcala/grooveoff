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


#ifndef PLAYLIST_H
#define PLAYLIST_H

#include <QListWidget>
#include "phononnamespace.h"

class DownloadItem;

class Playlist : public QListWidget
{
    Q_OBJECT
public:
    explicit Playlist(QWidget *parent = 0);

public slots:
    void reloadPlaylist();
    void cambioStato(Phonon::State newState, QString source);
    void removeFailedDeletedAborted();
    void removeDownloaded();
};

#endif // PLAYLIST_H
