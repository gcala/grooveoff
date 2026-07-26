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
#include "GrooveOffNamespace.h"

#include <QListWidget>
#include "GrooveOffNamespace.h"

class DownloadItem;

class DownloadList : public QListWidget
{
    Q_OBJECT
public:
    explicit DownloadList( QWidget *parent = 0 );

    QList< PlaylistItemPtr > playlistItems() const;

public Q_SLOTS:
    void reloadPlaylist();

    /** takes out the tracks that will not play however long one waits */
    void removeErrored();

    /** empties the list */
    void removeAll();

    void removeItemsFromState( GrooveOff::TrackState state );
    void removeItem(DownloadItem *item);
    
private Q_SLOTS:
    void actionTriggered();
};

#endif // DOWNLOADLIST_H
