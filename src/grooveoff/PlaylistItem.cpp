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

#if QT_VERSION >= QT_VERSION_CHECK( 5, 0, 0 )
#include <QStandardPaths>
#else
#include <QDesktopServices>
#endif

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
    if(path_.isEmpty()) {
        qWarning() << "GrooveOff ::" << "path is empty";
        // returning standard music path
#if QT_VERSION >= QT_VERSION_CHECK( 5, 0, 0 )
        return QStandardPaths::writableLocation(QStandardPaths::MusicLocation);
#else
        return QDesktopServices::storageLocation(QDesktopServices::MusicLocation);
#endif
    }
    return path_;
}

void PlaylistItem::setPath(const QString& path)
{
    path_ = path;
}

QString PlaylistItem::namingSchema() const
{
    if(namingSchema_.isEmpty()) {
        qWarning() << "GrooveOff ::" << "naming schema is empty";
        // returning standard naming schema
        return QLatin1String( "%artist - %title" );
    }

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

    if(fileName.isEmpty()) { // namingSchema_ is erroneously empty
        qWarning() << "naming schema is empty";
        fileName = "%artist - %title";
    }

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
