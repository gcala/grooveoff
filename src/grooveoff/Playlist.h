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


#ifndef PLAYLIST_H
#define PLAYLIST_H

#include "PlaylistItem.h"

#include <QObject>

class Playlist;

namespace The {
    Playlist* playlist();
}

class Playlist : public QObject
{
    Q_OBJECT
public:
    friend Playlist* The::playlist();
    ~Playlist();

    void clear();
    void appendItem( const PlaylistItemPtr &item );
    void removeItem( const PlaylistItemPtr &item );
    int count() const;
    PlaylistItemPtr item( int row ) const;
    int row( const PlaylistItemPtr &item );

Q_SIGNALS:
    void playlistChanged();

private:
    Playlist();

    QList< PlaylistItemPtr > m_playlist;
};

#endif // PLAYLIST_H
