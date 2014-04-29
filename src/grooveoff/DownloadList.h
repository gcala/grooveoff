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


#ifndef DOWNLOADLIST_H
#define DOWNLOADLIST_H

#include "PlaylistItem.h"

#include <QListWidget>
#include <phonon/phononnamespace.h>

class DownloadItem;

class DownloadList : public QListWidget
{
    Q_OBJECT
public:
    explicit DownloadList(QWidget *parent = 0);

    QList<PlaylistItemPtr> playlistItems() const;

public Q_SLOTS:
    void reloadPlaylist();
    void removeFailedAborted();
    void removeDownloaded();
    void abortAllDownloads();
    void removeItem(DownloadItem *item);
};

#endif // DOWNLOADLIST_H
