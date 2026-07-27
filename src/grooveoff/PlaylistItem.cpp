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

#include <QDataStream>
#include <QDir>

PlaylistItem::PlaylistItem( const TrackInfoPtr &track )
    : m_state( GrooveOff::PlayerState::Stopped )
    , m_track( track )
{
}

PlaylistItem::PlaylistItem()
    : m_state( GrooveOff::PlayerState::Stopped )
    , m_track( TrackInfoPtr( new TrackInfo() ) )
{
}

PlaylistItem::~PlaylistItem()
{
}

bool PlaylistItem::operator==( PlaylistItem& right ) const
{
    return m_track && right.track() && *m_track == *right.track();
}

void PlaylistItem::requireRefresh()
{
    emit refreshNeeded();
}

TrackInfoPtr PlaylistItem::track() const
{
    return m_track;
}

void PlaylistItem::setTrack( const TrackInfoPtr &track )
{
    m_track = track;
}

QString PlaylistItem::localPath() const
{
    if( !m_track || m_track->infoHash().isEmpty() )
        return QString();

    return Utility::torrentsCachePath + m_track->infoHash() + QDir::separator() + m_track->filePath();
}

QString PlaylistItem::displayTitle() const
{
    return m_track ? m_track->displayTitle() : QString();
}

QString PlaylistItem::displaySubtitle() const
{
    if( !m_track )
        return QString();

    const QString &artist = m_track->artist();
    const QString &album  = m_track->album();

    if( artist.isEmpty() && album.isEmpty() )
        return QString();
    if( artist.isEmpty() )
        return album;
    if( album.isEmpty() )
        return artist;

    return artist + QLatin1String( " - " ) + album;
}

void PlaylistItem::setState( const GrooveOff::PlayerState &state )
{
    m_state = state;
    emit stateChanged( m_state );
}

QDataStream& operator<<( QDataStream& dataStream, const PlaylistItemPtr item )
{
    return dataStream << item->track();
}

QDataStream& operator>>( QDataStream& dataStream, PlaylistItemPtr item )
{
    return dataStream >> item->track();
}
