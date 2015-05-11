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

void Playlist::appendItem( const PlaylistItemPtr &item )
{
    m_playlist.append( item );
    emit playlistChanged();
}

void Playlist::removeItem( const PlaylistItemPtr &item )
{
    for(int i = 0; i < m_playlist.count(); i++) {
        if(m_playlist.at(i) == item) {
            m_playlist.takeAt(i);
            break;
        }
    }
    emit playlistChanged();
}

void Playlist::clear()
{
    m_playlist.clear();
}

int Playlist::count() const
{
    return m_playlist.count();
}

PlaylistItemPtr Playlist::item( int row ) const
{
    return m_playlist.at(row);
}

int Playlist::row( const PlaylistItemPtr &item )
{
    int index = 0;
    bool found = false;
    for(; index < m_playlist.count(); index++) {
        if(item == m_playlist.at(index)) {
            found = true;
            break;
        }
    }

    if(!found)
        index = -1;

    return index;
}
