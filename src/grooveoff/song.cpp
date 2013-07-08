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


#include "grooveoff/song.h"

Song::Song ( const QString &title,
             const QString &album,
             const QString &artist,
             const QString &year,
             const QString &id,
             const QString &coverName) :
    title_(title),
    album_(album),
    artist_(artist),
    year_(year),
    id_(id),
    coverName_(coverName)
{
}

/*!
  \brief ~Song: this is the Song destructor.
*/
Song::~Song()
{
}

void Song::requireCoverReload()
{
    emit reloadCover();
}

void Song::requireDownloadIconReload()
{
    emit reloadIcon();
}


#include "song.moc"
