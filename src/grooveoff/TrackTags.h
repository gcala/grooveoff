/*
    GrooveOff - Music streaming from torrents
    Copyright (C) 2026  Giuseppe Calà <jiveaxe@gmail.com>

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


#ifndef TRACKTAGS_H
#define TRACKTAGS_H

#include "TrackInfo.h"

class QImage;
class QString;

/**
  * @namespace TrackTags
  * @short Reads what a half downloaded file is willing to say about itself.
  *
  * Tags live at the head of a file, so they are readable as soon as the first
  * pieces are in — which is also the only moment at which reading is honest,
  * since before that the sparse file is a hole and TagLib would find nothing.
  *
  * The duration is the exception and it is left alone on purpose: a variable
  * bitrate file without a Xing header only gives its length to a full scan,
  * and scanning a file made mostly of holes returns a number that is wrong
  * rather than missing. The player reports the real one as soon as it opens
  * the stream.
  */
namespace TrackTags {

    /**
     * Fills title, artist, album, track number and year of @p track from the
     * file in the cache. Returns @c true when anything was actually read, so
     * that the caller knows whether it is worth writing to the catalogue.
     */
    bool read( const TrackInfoPtr &track );

    /** the picture frame of the file, null when it has none or cannot be read */
    QImage cover( const QString &localPath );

}

#endif // TRACKTAGS_H
