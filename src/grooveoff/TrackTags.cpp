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


#include "TrackTags.h"
#include "PlaylistItem.h"

#include <QFile>
#include <QImage>

#include <taglib/fileref.h>
#include <taglib/tag.h>
#include <taglib/tfile.h>
#include <taglib/tstring.h>
#include <taglib/tvariant.h>

namespace {

QString fromTagLib( const TagLib::String &text )
{
    return QString::fromStdString( text.to8Bit( true ) ).trimmed();
}

/** the local path of a track, or an empty string when it has no place yet */
QString pathOf( const TrackInfoPtr &track )
{
    if( !track )
        return QString();

    return PlaylistItem( track ).localPath();
}

} // namespace

bool TrackTags::read( const TrackInfoPtr &track )
{
    const QString path = pathOf( track );
    if( path.isEmpty() || !QFile::exists( path ) )
        return false;

    // No audio properties asked for: reading them means scanning, and scanning
    // a file that is mostly holes costs time to produce a wrong answer.
    TagLib::FileRef file( QFile::encodeName( path ).constData(), false );
    if( file.isNull() || !file.tag() )
        return false;

    const TagLib::Tag *tag = file.tag();

    const QString title  = fromTagLib( tag->title() );
    const QString artist = fromTagLib( tag->artist() );
    const QString album  = fromTagLib( tag->album() );

    if( title.isEmpty() && artist.isEmpty() && album.isEmpty()
        && tag->track() == 0 && tag->year() == 0 ) {
        return false;
    }

    if( !title.isEmpty() )
        track->setTitle( title );
    if( !artist.isEmpty() )
        track->setArtist( artist );
    if( !album.isEmpty() )
        track->setAlbum( album );
    if( tag->track() != 0 )
        track->setTrackNum( tag->track() );
    if( tag->year() != 0 )
        track->setYear( tag->year() );

    return true;
}

QImage TrackTags::cover( const QString &localPath )
{
    if( localPath.isEmpty() || !QFile::exists( localPath ) )
        return QImage();

    TagLib::FileRef file( QFile::encodeName( localPath ).constData(), false );
    if( file.isNull() )
        return QImage();

    // The complex property api of TagLib 2 hands back pictures the same way
    // whatever the container, so there is no per format branch here: id3v2,
    // vorbis comments and mp4 atoms all answer to the same key.
    const TagLib::List<TagLib::VariantMap> pictures = file.complexProperties( "PICTURE" );
    if( pictures.isEmpty() )
        return QImage();

    const TagLib::ByteVector data = pictures.front().value( "data" ).value<TagLib::ByteVector>();
    if( data.isEmpty() )
        return QImage();

    QImage cover;
    cover.loadFromData( reinterpret_cast<const uchar *>( data.data() ), int( data.size() ) );

    return cover;
}
