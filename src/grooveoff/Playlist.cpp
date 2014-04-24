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

#include "Playlist.h"

namespace The {
    static Playlist* s_Playlist_instance = 0;

    Playlist* playlist()
    {
        if( !s_Playlist_instance )
            s_Playlist_instance = new Playlist();

        return s_Playlist_instance;
    }
}

Playlist::Playlist()
{
}

Playlist::~Playlist()
{
}

void Playlist::appendItem(PlaylistItemPtr item)
{
    playlist_.append(item);
    emit playlistChanged();
}

void Playlist::clear()
{
    playlist_.clear();
}

int Playlist::count() const
{
    return playlist_.count();
}

PlaylistItemPtr Playlist::item(int row)
{
    return playlist_.at(row);
}

int Playlist::row(PlaylistItemPtr item)
{
    int index = 0;
    bool found = false;
    for(; index < playlist_.count(); index++) {
        if(item == playlist_.at(index)) {
            found = true;
            break;
        }
    }

    if(!found)
        index = -1;

    return index;
}

void Playlist::removeItem(PlaylistItemPtr item)
{
    for(int i = 0; i < playlist_.count(); i++) {
        if(playlist_.at(i) == item) {
            playlist_.takeAt(i);
            break;
        }
    }
    emit playlistChanged();
}

