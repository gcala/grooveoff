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


#ifndef ROLES_H
#define ROLES_H

#include <QByteArray>
#include <QDataStream>
#include <QIODevice>
#include <QLatin1String>
#include <QList>
#include <QMimeData>

namespace SongRoles
{
    enum roles
    {
        Title = 100,
        Album,
        Artist,
        Year,
        Cover,
        Id,
        CoverPix,

        /**
         * The catalogue id of the track this row shows. The row carries it, and
         * not only the TrackRow inside it, because a drag leaves the widget
         * behind and takes the item's data with it.
         */
        TrackId
    };
}

namespace PlaylistRoles
{
    enum roles
    {
        Id = 200,
        Kind,
        InfoHash,   /**< empty for a custom playlist */

        /**
         * GrooveOff::SourceHealth, and only on a playlist born from a torrent.
         * A custom playlist holds tracks of several torrents, so the health of
         * the source has no single answer for it and the delegate draws
         * nothing: an average of things that are not comparable would be worse
         * than saying nothing at all.
         */
        Health,

        /** how many tracks the playlist holds, for the first line of its tooltip */
        TrackCount,

        /**
         * The playlist the music is coming from, which is not always the one
         * being looked at. Without something on the row saying so, "why is
         * this playing?" has no answer anywhere on screen.
         */
        Playing
    };
}

/**
 * @namespace GrooveOffMime
 * @short What a drag of tracks carries: their catalogue ids and nothing else.
 *
 * Ids rather than serialised tracks, and that is worth a line. A dropped track
 * has to end up pointing at the same row of the catalogue the dragged one came
 * from — that is the whole point of one table of tracks with the playlists
 * pointing into it — and a copy of the metadata travelling along would be a
 * second truth that starts drifting the moment the tags are read off the file.
 */
namespace GrooveOffMime
{
    inline const QLatin1String tracks( "application/x-grooveoff-tracks" );

    inline QByteArray encodeTracks( const QList<qint64> &trackIds )
    {
        QByteArray payload;
        QDataStream stream( &payload, QIODevice::WriteOnly );
        stream << trackIds;

        return payload;
    }

    /** empty when the drag came from somewhere that is not this application */
    inline QList<qint64> decodeTracks( const QMimeData *data )
    {
        QList<qint64> trackIds;

        if( !data || !data->hasFormat( tracks ) )
            return trackIds;

        QByteArray payload = data->data( tracks );
        QDataStream stream( &payload, QIODevice::ReadOnly );
        stream >> trackIds;

        return trackIds;
    }
}

#endif // ROLES_H
