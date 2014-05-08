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

#include <QDir>
#include <QMetaProperty>

#if QT_VERSION >= QT_VERSION_CHECK( 5, 0, 0 )
#include <QStandardPaths>
#else
#include <QDesktopServices>
#endif

using namespace GrooveShark;

PlaylistItem::PlaylistItem ( const SongPtr &song ) :
    m_song(song)
{
    m_state = Phonon::StoppedState;
}

PlaylistItem::PlaylistItem()
{
    m_song = SongPtr( new Song() );
}


/*!
\brief ~Song: this is the Song destructor.
*/
PlaylistItem::~PlaylistItem()
{
}

bool PlaylistItem::operator==( PlaylistItem& right ) const
{
    return m_song->songID() == right.song()->songID();
}

void PlaylistItem::requireCoverReload()
{
    emit reloadCover();
}

QString PlaylistItem::path() const
{
    if(m_path.isEmpty()) {
        qWarning() << "GrooveOff ::" << "path is empty";
        // returning standard music path
#if QT_VERSION >= QT_VERSION_CHECK( 5, 0, 0 )
        return QStandardPaths::writableLocation( QStandardPaths::MusicLocation );
#else
        return QDesktopServices::storageLocation( QDesktopServices::MusicLocation );
#endif
    }
    return m_path;
}

void PlaylistItem::setPath( const QString& path )
{
    m_path = path;
}

QString PlaylistItem::namingSchema() const
{
    if( m_namingSchema.isEmpty() ) {
        qWarning() << "GrooveOff ::" << "naming schema is empty. Returning default.";
        return QLatin1String( "%artist/%album/%track - %title" );
    }

    return m_namingSchema;
}

void PlaylistItem::setNamingSchema( const QString& schema )
{
    m_namingSchema = schema;
}

SongPtr PlaylistItem::song()
{
    return m_song;
}

void PlaylistItem::setSong( const SongPtr &song )
{
    m_song = song;
}

QString PlaylistItem::fileName() const
{
    QString fileName = m_namingSchema;

    if(fileName.isEmpty()) { // m_namingSchema is erroneously empty
        qWarning() << "GrooveOff ::" << "naming schema is empty. Set to default.";
        fileName = QLatin1String( "%artist/%album/%track - %title" );
    }

    fileName.replace(QLatin1String("%title"), m_song->songName(), Qt::CaseInsensitive);
    fileName.replace(QLatin1String("%artist"), m_song->artistName(), Qt::CaseInsensitive);
    fileName.replace(QLatin1String("%album"), m_song->albumName(), Qt::CaseInsensitive);
    fileName.replace(QLatin1String("%track"), QString::number(m_song->trackNum()), Qt::CaseInsensitive);

    return fileName + ".mp3";
}

void PlaylistItem::setState( const Phonon::State &state )
{
    m_state = state;
    emit stateChanged( m_state );
}

QDataStream& operator<<( QDataStream& dataStream, const PlaylistItemPtr item )
{
    for( int i=0; i< item->metaObject()->propertyCount(); ++i ) {
        if( item->metaObject()->property(i).isStored( item.data() ) ) {
            dataStream << item->metaObject()->property(i).read( item.data() );
        }
    }
    return dataStream;
}

QDataStream & operator>>( QDataStream & dataStream, PlaylistItemPtr item ) {
    QVariant var;
    for( int i = 0; i < item->metaObject()->propertyCount(); ++i ) {
        if( item->metaObject()->property(i).isStored( item.data() ) ) {
            dataStream >> var;
            item->metaObject()->property(i).write( item.data(), var );
        }
    }
    return dataStream;
}
