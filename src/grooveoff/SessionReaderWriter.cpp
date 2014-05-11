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


#include "SessionReaderWriter.h"

#include <QFile>
#include <QDomDocument>
#include <QMetaProperty>
#include <QXmlStreamWriter>
#include <QDir>
#include <QDebug>

using namespace GrooveShark;

namespace The {
    static SessionReaderWriter* s_SessionReaderWriter_instance = 0;

    SessionReaderWriter* sessionReaderWriter()
    {
        if( !s_SessionReaderWriter_instance )
            s_SessionReaderWriter_instance = new SessionReaderWriter();

        return s_SessionReaderWriter_instance;
    }
}

QList< PlaylistItemPtr > SessionReaderWriter::read( const QString& file )
{
    QFile sessionFile( file );
    if(!sessionFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "SessionReaderWriter :: Unable to read session file" << file.toLocal8Bit().constData(), sessionFile.errorString().toLocal8Bit().constData();
        return QList< PlaylistItemPtr >();
    }

    QList<PlaylistItemPtr> items;

    QDomDocument doc( QLatin1String( "mydocument" ) );
    QString errorStr;
    int errorLine;
    int errorColumn;
    if ( !doc.setContent( &sessionFile, true, &errorStr, &errorLine, &errorColumn ) ) {
        qDebug() << "SessionReaderWriter ::" << errorStr << errorLine << errorColumn;
        sessionFile.close();
        return QList< PlaylistItemPtr >();
    }
    sessionFile.close();

    QDomElement root = doc.documentElement();
    QDomElement itemEl = root.firstChildElement( QLatin1String( "item" ) );
    while (!itemEl.isNull()) {
        PlaylistItemPtr item( new PlaylistItem() );

        items.append( item );
        parsePlaylistItem( itemEl, item );
        itemEl = itemEl.nextSiblingElement( QLatin1String( "item" ) );
    }

    return items;
}

void SessionReaderWriter::parsePlaylistItem( const QDomElement& element, PlaylistItemPtr item )
{
    QVariant var;
    for( int i = 0; i < item->metaObject()->propertyCount(); ++i ) {
        if( item->metaObject()->property(i).isStored( item.data() ) ) {
            // without this check a crash happens when compiled with qt5
            // "song" is the property's name containing a Song object
            if( QString( item->metaObject()->property(i).typeName() ) == "GrooveShark::SongPtr" )
                continue;
            
            var = element.firstChildElement( QString( item->metaObject()->property(i).name() ) ).text();
            item->metaObject()->property(i).write( item.data(), var );
        }
    }

    QDomElement songEl = element.firstChildElement( QLatin1String( "song_info" ) );
    parseSong( songEl, item->song() );
}

void SessionReaderWriter::parseSong( const QDomElement& element, GrooveShark::SongPtr song )
{
    QVariant var;
    for( int i = 0; i < song->metaObject()->propertyCount(); ++i ) {
        if( song->metaObject()->property(i).isStored( song.data() ) ) {
            var = element.firstChildElement( QString( song->metaObject()->property(i).name() ) ).text();
            song->metaObject()->property(i).write( song.data(), var );
        }
    }
}

bool SessionReaderWriter::write( const QString& file, QList< PlaylistItemPtr > tracks )
{
    QFile sessionFile( file );
    if( sessionFile.exists() ) {
        if( !sessionFile.remove() ) {
            qDebug() << "SessionReaderWriter :: Unable to remove old session file" << sessionFile.fileName();
            return false;
        }
    }
    
    QFileInfo fi( file );
    
    if( !QDir().mkpath(fi.absolutePath() ) ) {
        qDebug() << "SessionReaderWriter :: Cannot create session path" << fi.absolutePath();
        return false;
    }
    
    if( !sessionFile.open( QIODevice::WriteOnly | QIODevice::Text ) ) {
        qDebug() << "SessionReaderWriter :: Unable to write to session file" << file.toLocal8Bit().constData(), sessionFile.errorString().toLocal8Bit().constData();
        return false;
    }

    QXmlStreamWriter stream( &sessionFile );
    stream.setAutoFormatting( true );
    stream.writeStartDocument();
    stream.writeStartElement( QLatin1String( "playlist" ) );

    foreach( const PlaylistItemPtr &track, tracks ) {
        stream.writeStartElement( QLatin1String( "item" ) );
        for( int j = 0; j < track->metaObject()->propertyCount(); j++ ) {
            if( track->metaObject()->property( j ).isStored( track.data() ) ) {
                if( QString( track->metaObject()->property(j).name() ) ==  QLatin1String( "objectName" ) )
                    continue;
                if( QString( track->metaObject()->property( j ).name() ) ==  QLatin1String( "song" ) ) {
                    const SongPtr &song = track->song();
                    stream.writeStartElement( QLatin1String( "song_info" ) );
                    for( int k = 0; k < song->metaObject()->propertyCount(); k++ ) {
                        if( song->metaObject()->property( k ).isStored( song.data() ) ) {
                            if( QString( song->metaObject()->property( k ).name() ) ==  QLatin1String( "objectName" ) )
                                continue;
                            if( QString( song->metaObject()->property( k ).name() ) ==  QLatin1String( "errorString" ) )
                                continue;
                            stream.writeTextElement( song->metaObject()->property( k ).name(),
                                                     song->metaObject()->property( k ).read( song.data() ).toString() 
                                                   );
                        }
                    }
                    stream.writeEndElement(); // song_info
                } else {
                    stream.writeTextElement( track->metaObject()->property( j ).name(),
                                             track->metaObject()->property( j ).read( track.data() ).toString() 
                                           );
                }
            }
        }
        stream.writeEndElement(); // item
    }

    stream.writeEndElement(); // playlist

    sessionFile.close();
    
    return true;
}


