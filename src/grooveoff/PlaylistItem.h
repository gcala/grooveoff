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


#ifndef PLAYLISTITEM_H
#define PLAYLISTITEM_H

#include <QObject>
#include <Phonon/MediaSource>
#include <../libgrooveshark/song.h>

class PlaylistItem : public QObject
{
    Q_OBJECT
    Q_PROPERTY( QString path READ path WRITE setPath )
    Q_PROPERTY( GrooveShark::SongPtr song READ song WRITE setSong )

public:
    explicit PlaylistItem ( const GrooveShark::SongPtr &song );
    PlaylistItem();
    ~PlaylistItem();

    QString path() const;
    void setPath(const QString& path);
    QString fileName() const;
    GrooveShark::SongPtr song();
    void setSong(GrooveShark::SongPtr song);

    void requireCoverReload();
    void requireDownloadIconReload();
    bool isPlaying() { return state_ == Phonon::PlayingState; }
    void setState(Phonon::State state);

    bool operator==(PlaylistItem &) const;

signals:
    void reloadCover();
    void reloadIcon();
    void stateChanged(Phonon::State);

public slots:

private:
    Phonon::State state_;
    GrooveShark::SongPtr song_;
    QString path_;
};

typedef QSharedPointer<PlaylistItem> PlaylistItemPtr;

Q_DECLARE_METATYPE( PlaylistItemPtr )

#endif // PLAYLISTITEM_H
