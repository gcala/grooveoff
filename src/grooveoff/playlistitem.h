/*
GrooveOff - Offline Grooveshark.com music
Copyright (C) 2013 Giuseppe Calà <jiveaxe@gmail.com>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program. If not, see <http://www.gnu.org/licenses/>.
*/


#ifndef PLAYLISTITEM_H
#define PLAYLISTITEM_H

#include <QObject>
#include <Phonon/MediaSource>
#include <../libgrooveshark/song.h>

class PlaylistItem : public QObject
{
    Q_OBJECT

public:
    explicit PlaylistItem ( const GrooveShark::SongPtr &song );
    ~PlaylistItem();

    QString path() const { return path_; }
    void setPath(const QString& path);
    QString fileName() const;
    GrooveShark::SongPtr info() { return song_;}

    Phonon::MediaSource source() const { return source_; }

    void requireCoverReload();
    void requireDownloadIconReload();
    void setPlayerState(Phonon::State);
    bool isPlaying() { return isPlaying_; }
    void setPlaying(bool ok) {isPlaying_ = ok; }

signals:
    void reloadCover();
    void reloadIcon();
    void stateChanged(Phonon::State);

public slots:

private:
    GrooveShark::SongPtr song_;
    Phonon::MediaSource source_;
    QString path_;
    bool isPlaying_;
};

typedef QSharedPointer<PlaylistItem> PlaylistItemPtr;
Q_DECLARE_METATYPE( PlaylistItemPtr )

#endif // PLAYLISTITEM_H
