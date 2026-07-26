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


#include "TrackInfo.h"

#include <QDataStream>
#include <QFileInfo>
#include <QMetaProperty>
#include <QStringList>

namespace {

/**
 * Extensions we are willing to hand to the player. Anything else still ends up
 * in the catalogue with isAudio false, so that an unforeseen format can be
 * reached through "show every file" instead of silently disappearing.
 */
QStringList audioExtensions()
{
    static const QStringList extensions = {
        QLatin1String( "mp3" ),
        QLatin1String( "flac" ),
        QLatin1String( "ogg" ),
        QLatin1String( "oga" ),
        QLatin1String( "opus" ),
        QLatin1String( "m4a" ),
        QLatin1String( "mp4" ),
        QLatin1String( "aac" ),
        QLatin1String( "wav" ),
        QLatin1String( "wma" ),
        QLatin1String( "ape" ),
        QLatin1String( "wv" ),
        QLatin1String( "mpc" ),
        QLatin1String( "aiff" ),
        QLatin1String( "aif" )
    };

    return extensions;
}

} // namespace

TrackInfo::TrackInfo( QObject *parent )
    : QObject( parent )
    , m_id( 0 )
    , m_fileIndex( -1 )
    , m_fileSize( 0 )
    , m_isAudio( false )
    , m_excluded( false )
    , m_trackNum( 0 )
    , m_year( 0 )
    , m_duration( 0 )
{
}

TrackInfo::~TrackInfo()
{
}

qint64 TrackInfo::id() const
{
    return m_id;
}

void TrackInfo::setId( qint64 id )
{
    m_id = id;
}

QString TrackInfo::infoHash() const
{
    return m_infoHash;
}

void TrackInfo::setInfoHash( const QString &infoHash )
{
    m_infoHash = infoHash;
}

QString TrackInfo::magnetUri() const
{
    return m_magnetUri;
}

void TrackInfo::setMagnetUri( const QString &magnetUri )
{
    m_magnetUri = magnetUri;
}

int TrackInfo::fileIndex() const
{
    return m_fileIndex;
}

void TrackInfo::setFileIndex( int fileIndex )
{
    m_fileIndex = fileIndex;
}

QString TrackInfo::filePath() const
{
    return m_filePath;
}

void TrackInfo::setFilePath( const QString &filePath )
{
    m_filePath = filePath;
}

qint64 TrackInfo::fileSize() const
{
    return m_fileSize;
}

void TrackInfo::setFileSize( qint64 fileSize )
{
    m_fileSize = fileSize;
}

bool TrackInfo::isAudio() const
{
    return m_isAudio;
}

void TrackInfo::setIsAudio( bool isAudio )
{
    m_isAudio = isAudio;
}

bool TrackInfo::excluded() const
{
    return m_excluded;
}

void TrackInfo::setExcluded( bool excluded )
{
    m_excluded = excluded;
}

QString TrackInfo::title() const
{
    return m_title;
}

void TrackInfo::setTitle( const QString &title )
{
    m_title = title;
}

QString TrackInfo::artist() const
{
    return m_artist;
}

void TrackInfo::setArtist( const QString &artist )
{
    m_artist = artist;
}

QString TrackInfo::album() const
{
    return m_album;
}

void TrackInfo::setAlbum( const QString &album )
{
    m_album = album;
}

quint32 TrackInfo::trackNum() const
{
    return m_trackNum;
}

void TrackInfo::setTrackNum( quint32 trackNum )
{
    m_trackNum = trackNum;
}

quint32 TrackInfo::year() const
{
    return m_year;
}

void TrackInfo::setYear( quint32 year )
{
    m_year = year;
}

qint64 TrackInfo::duration() const
{
    return m_duration;
}

void TrackInfo::setDuration( qint64 duration )
{
    m_duration = duration;
}

QString TrackInfo::displayTitle() const
{
    if( !m_title.isEmpty() )
        return m_title;

    const QString &name = fileName();
    const int dot = name.lastIndexOf( QLatin1Char( '.' ) );

    return dot > 0 ? name.left( dot ) : name;
}

QString TrackInfo::fileName() const
{
    const int slash = m_filePath.lastIndexOf( QLatin1Char( '/' ) );

    return slash >= 0 ? m_filePath.mid( slash + 1 ) : m_filePath;
}

bool TrackInfo::isAudioFile( const QString &filePath )
{
    const QString &suffix = QFileInfo( filePath ).suffix().toLower();

    return audioExtensions().contains( suffix );
}

bool TrackInfo::operator==( const TrackInfo &other ) const
{
    return m_infoHash == other.m_infoHash && m_fileIndex == other.m_fileIndex;
}

QDataStream& operator<<( QDataStream& dataStream, const TrackInfoPtr track )
{
    for( int i = 0; i < track->metaObject()->propertyCount(); ++i ) {
        if( track->metaObject()->property( i ).isStored() )
            dataStream << track->metaObject()->property( i ).read( track.data() );
    }

    return dataStream;
}

QDataStream& operator>>( QDataStream& dataStream, TrackInfoPtr track )
{
    QVariant var;

    for( int i = 0; i < track->metaObject()->propertyCount(); ++i ) {
        if( track->metaObject()->property( i ).isStored() ) {
            dataStream >> var;
            track->metaObject()->property( i ).write( track.data(), var );
        }
    }

    return dataStream;
}
