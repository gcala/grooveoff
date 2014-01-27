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


#include "PlaylistItem.h"
#include "Utility.h"

#include <QDir>
#include <QMetaProperty>

using namespace GrooveShark;

PlaylistItem::PlaylistItem ( const SongPtr &song ) :
    song_(song)
{
    state_ = Phonon::StoppedState;
}

PlaylistItem::PlaylistItem()
{
    song_ = SongPtr(new Song());
}


/*!
\brief ~Song: this is the Song destructor.
*/
PlaylistItem::~PlaylistItem()
{
}

bool PlaylistItem::operator==(PlaylistItem& right) const
{
    return song_->songID() == right.song()->songID();
}

void PlaylistItem::requireCoverReload()
{
    emit reloadCover();
}

QString PlaylistItem::path() const
{
    return path_;
}

void PlaylistItem::setPath(const QString& path)
{
    path_ = path;
}

QString PlaylistItem::namingSchema() const
{
    return namingSchema_;
}

void PlaylistItem::setNamingSchema(const QString& schema)
{
    namingSchema_ = schema;
}

SongPtr PlaylistItem::song()
{
    return song_;
}

void PlaylistItem::setSong(SongPtr song)
{
    song_ = song;
}

QString PlaylistItem::fileName() const
{
    QString fileName = namingSchema_;
    fileName.replace(QLatin1String("%title"), song_->songName(), Qt::CaseInsensitive);
    fileName.replace(QLatin1String("%artist"), song_->artistName(), Qt::CaseInsensitive);
    fileName.replace(QLatin1String("%album"), song_->albumName(), Qt::CaseInsensitive);
    fileName.replace(QLatin1String("%track"), QString::number(song_->trackNum()), Qt::CaseInsensitive);

    return fileName + ".mp3";
}

void PlaylistItem::setState(Phonon::State state)
{
    state_ = state;
    emit stateChanged(state_);
}
