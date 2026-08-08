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

/*!
  \brief hasHead: whether there is anything at the head of the file to read

  The rule everywhere here is that a file is asked what it is only once its
  head is in the cache. This is what enforces it, and it is not a nicety: a
  sparse file whose first bytes have not arrived reads back as zeros, and
  TagLib does not take that for an answer. The mpeg parser goes looking for a
  sync word and scans forward through the whole file to find one that is not
  there — a second, measured, for ten megabytes of holes. Adding an album of
  thirteen tracks built thirteen rows and froze the window for thirteen
  seconds, with nothing on screen and nothing in the log to say why.

  Four bytes are enough to know. Every format this reads begins with a
  signature — "ID3", "fLaC", "OggS", the mpeg sync itself — so four zeros mean
  the head has not arrived, whatever the rest of the file may already hold.

  A flac hid this for a long time by being honest: its magic is at offset zero,
  so it gives up at once. It is the mp3 that goes looking.
*/
bool headOf( const QString &path )
{
    QFile file( path );
    if( !file.open( QIODevice::ReadOnly ) )
        return false;

    const QByteArray head = file.read( 4 );

    return head.size() == 4 && head != QByteArray( 4, '\0' );
}

} // namespace

bool TrackTags::hasHead( const QString &localPath )
{
    return headOf( localPath );
}

bool TrackTags::read( const TrackInfoPtr &track )
{
    const QString path = pathOf( track );
    if( path.isEmpty() || !QFile::exists( path ) || !headOf( path ) )
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

bool TrackTags::pictureIsWhole( const QByteArray &data )
{
    if( data.size() < 4 )
        return false;

    const auto byte = [&data]( int at ) { return static_cast<unsigned char>( data.at( at ) ); };

    if( byte( 0 ) == 0xFF && byte( 1 ) == 0xD8 ) {
        return byte( data.size() - 2 ) == 0xFF && byte( data.size() - 1 ) == 0xD9;
    }

    if( data.size() >= 8 && byte( 0 ) == 0x89 && byte( 1 ) == 0x50
        && byte( 2 ) == 0x4E && byte( 3 ) == 0x47 ) {
        return data.contains( QByteArrayLiteral( "IEND" ) );
    }

    return true;
}

QImage TrackTags::cover( const QString &localPath )
{
    if( localPath.isEmpty() || !QFile::exists( localPath ) || !headOf( localPath ) )
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
    if( data.isEmpty()
        || !pictureIsWhole( QByteArray::fromRawData( data.data(), int( data.size() ) ) ) ) {
        return QImage();
    }

    QImage cover;
    cover.loadFromData( reinterpret_cast<const uchar *>( data.data() ), int( data.size() ) );

    return cover;
}
