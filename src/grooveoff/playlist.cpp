/*
 * GrooveOff - Offline Grooveshark.com music
 * Copyright (C) 2013  Giuseppe Calà <Giuseppe.Cala-1973@poste.it>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "playlist.h"

bool Playlist::instanceFlag = false;
Playlist* Playlist::playlist = NULL;

Playlist* Playlist::instance()
{
    if(! instanceFlag)
    {
        playlist = new Playlist();
        instanceFlag = true;
        return playlist;
    }
    else
    {
        return playlist;
    }
}

Playlist::Playlist()
{
}

Playlist::~Playlist()
{
    instanceFlag = false;
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
}

