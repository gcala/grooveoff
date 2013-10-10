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


#ifndef SONGITEM_H
#define SONGITEM_H

#include <QObject>
#include <Phonon/MediaSource>
#include <libgrooveshark/song.h>

class SongItem : public QObject
{
    Q_OBJECT

public:
    explicit SongItem ( const GrooveShark::SongPtr &song );
    ~SongItem();

    QString path() const { return path_; }
    void setPath(const QString& path);
    GrooveShark::SongPtr info() { return song_;}

    Phonon::MediaSource source() const { return source_; }

    void requireCoverReload();
    void requireDownloadIconReload();
    void requireRemotion();
    void setPlayerState(Phonon::State);

signals:
    void reloadCover();
    void reloadIcon();
    void playMe();
    void removeMe();
    void stateChanged(Phonon::State);

public slots:

private:
    GrooveShark::SongPtr song_;
    Phonon::MediaSource source_;
    QString path_;
};

typedef QSharedPointer<SongItem> SongItemPtr;
Q_DECLARE_METATYPE( SongItemPtr )

#endif // SONGITEM_H
