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

/*!
  \brief Song: this is the Song constructor.
  \param parent: The Parent Widget
*/
Song::Song ( QObject* parent ) :
    QObject(parent)
{
}

Song::Song ( const QString &title,
             const QString &album,
             const QString &artist,
             const QString &year,
             const QString &id,
             QObject* parent) :
    QObject(parent),
    title_(title),
    album_(album),
    artist_(artist),
    year_(year),
    id_(id)
{

}

/*!
  \brief ~Song: this is the Song destructor.
*/
Song::~Song()
{
}

// Song& Song::operator=(const Song* song)
// {
//     title_ = song->title();
//     album_ = song->album();
//     artist_ = song->artist();
//     year_ = song->year();
//     id_ = song->id();
//     coverName_ = song->coverName();
//     return &this;
//
// //     Song mysong;
// //     mysong.setTitle(song->title());
// //     mysong.setAlbum(song->album());
// //     mysong.setArtist(song->artist());
// //     mysong.setYear(song->year());
// //     mysong.setId(song->id());
// //     mysong.setCoverName(song->coverName());
// //     return mysong;
// }

Song::Song(const Song* song)
    : QObject()
{
    title_ = song->title();
    album_ = song->album();
    artist_ = song->artist();
    year_ = song->year();
    id_ = song->id();
    coverName_ = song->coverName();
}


#include "song.moc"
