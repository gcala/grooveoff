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

#include "GrooveOffNamespace.h"
#include "TrackInfo.h"

#include <QObject>

/**
  * @class PlaylistItem
  * @short A track as it appears in a list, with its playback state.
  *
  * Unlike TrackInfo it is not serialised by reflection: a QSharedPointer
  * cannot be written to a QDataStream, which is what forced the old code to
  * special case the song property. The stream operators below hand the work
  * to TrackInfo instead, whose properties are all plain values.
  */
class PlaylistItem : public QObject
{
    Q_OBJECT

public:
    explicit PlaylistItem( const TrackInfoPtr &track );
    PlaylistItem();
    ~PlaylistItem();

    TrackInfoPtr track() const;
    void setTrack( const TrackInfoPtr &track );

    /**
     * Where the file lives in the torrent cache. Until the stream server
     * exists this is also what the player is given, which is why a fully
     * cached track can already be played.
     */
    QString localPath() const;

    /** what to show in a list row */
    QString displayTitle() const;
    QString displaySubtitle() const;

    /**
     * The file has said what it is — tags, picture, the lot — and whoever is
     * showing this track has to draw it again. One signal rather than one per
     * field: the row and the player bar are looking at the same object and both
     * of them would otherwise learn half of it.
     */
    void requireRefresh();

    void setState( const GrooveOff::PlayerState &state );

    inline bool isPlaying() {
        return m_state == GrooveOff::PlayerState::Playing;
    }

    inline GrooveOff::PlayerState state() {
        return m_state;
    }

    bool operator==( PlaylistItem & ) const;

Q_SIGNALS:
    void refreshNeeded();
    void stateChanged( GrooveOff::PlayerState );

private:
    GrooveOff::PlayerState m_state;
    TrackInfoPtr           m_track;
};

typedef QSharedPointer<PlaylistItem> PlaylistItemPtr;

QDataStream& operator<<( QDataStream& dataStream, const PlaylistItemPtr item );
QDataStream& operator>>( QDataStream& dataStream, PlaylistItemPtr item );

Q_DECLARE_METATYPE( PlaylistItemPtr )

#endif // PLAYLISTITEM_H
