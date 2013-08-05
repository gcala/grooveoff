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
#include <QDebug>

#include "grooveoff/songobject.h"

#include <QDir>

SongObject::SongObject (  const GrooveShark::SongPtr &song ) :
    song_(song)
{
//     qDebug() << "CREO" << id_;
}

/*!
  \brief ~Song: this is the Song destructor.
*/
SongObject::~SongObject()
{
//     qDebug() << "DISTRUGGO" << id_;
}

void SongObject::requireCoverReload()
{
    emit reloadCover();
}

void SongObject::requireDownloadIconReload()
{
    emit reloadIcon();
}

void SongObject::requireRemotion()
{
    emit removeMe();
}

void SongObject::setPath(const QString& path)
{
    path_ = path;
    QString fileName = song_->songName() + " - " + song_->artistName();
    QString completeName = path_ + QDir::separator() + fileName.replace('/','-');
    source_ =  Phonon::MediaSource(completeName + ".mp3");
}

void SongObject::setPlayerState(Phonon::State state)
{
    emit stateChanged(state);
}


#include "songobject.moc"
