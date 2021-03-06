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

#ifndef SESSIONREADERWRITER_H
#define SESSIONREADERWRITER_H

#include "PlaylistItem.h"

#include <QObject>

class QDomElement;
class SessionReaderWriter;

namespace The {
    SessionReaderWriter* sessionReaderWriter();
}

class SessionReaderWriter : public QObject
{
    Q_OBJECT
    friend SessionReaderWriter* The::sessionReaderWriter();
public:
    ~SessionReaderWriter(){}

    QList<PlaylistItemPtr> read( const QString & file );
    bool write( const QString &file, QList<PlaylistItemPtr> tracks );

private:
    explicit SessionReaderWriter( QObject* parent = 0 ) : QObject(parent) {}

    void parsePlaylistItem( const QDomElement& element, PlaylistItemPtr item );
    void parseSong( const QDomElement& element, GrooveShark::SongPtr song );
};

#endif // SESSIONREADERWRITER_H
