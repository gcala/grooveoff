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
#include <phonon/mediasource.h>
#include <../libgrooveshark/song.h>

class PlaylistItem : public QObject
{
    Q_OBJECT
    Q_PROPERTY( QString path READ path WRITE setPath )
    Q_PROPERTY( QString namingSchema READ namingSchema WRITE setNamingSchema )
    Q_PROPERTY( GrooveShark::SongPtr song READ song WRITE setSong )

public:
    explicit PlaylistItem ( const GrooveShark::SongPtr &song );
    PlaylistItem();
    ~PlaylistItem();

    QString path() const;
    void setPath(const QString& path);
    GrooveShark::SongPtr song();
    void setSong(GrooveShark::SongPtr song);
    QString namingSchema() const;
    void setNamingSchema(const QString& schema);

    QString fileName() const;
    void requireCoverReload();
    bool isPlaying() { return m_state == Phonon::PlayingState; }
    void setState(Phonon::State state);

    bool operator==(PlaylistItem &) const;

Q_SIGNALS:
    void reloadCover();
    void reloadIcon();
    void stateChanged(Phonon::State);

public Q_SLOTS:

private:
    Phonon::State m_state;
    GrooveShark::SongPtr m_song;
    QString m_path;
    QString m_namingSchema;
};

typedef QSharedPointer<PlaylistItem> PlaylistItemPtr;

QDataStream& operator<<( QDataStream& dataStream, const PlaylistItemPtr item );
QDataStream& operator>>( QDataStream& dataStream, PlaylistItemPtr item ); // deprecated: throw( UserException )

Q_DECLARE_METATYPE( PlaylistItemPtr )

#endif // PLAYLISTITEM_H
