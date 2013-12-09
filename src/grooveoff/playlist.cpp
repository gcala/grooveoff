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

#include "playlist_p.h"

PlaylistPrivate::PlaylistPrivate(Playlist* qq, QObject* parent) : QObject ( parent ), q(qq)
{
}

PlaylistPrivate::~PlaylistPrivate()
{
}

Playlist::Playlist(QObject* parent)
    : QObject ( parent ), d(new PlaylistPrivate(this))
{

}

Playlist::~Playlist()
{
    delete d;

}

#include "playlist_p.moc"
#include "playlist.moc"
