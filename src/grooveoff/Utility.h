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


#ifndef UTILITY_H
#define UTILITY_H

#include "../db/PlaylistStore.h"

#include <QCoreApplication>
#include <QList>
#include <QString>
#include <QFont>
#include <QStringList>


class Utility
{
    Q_DECLARE_TR_FUNCTIONS( Utility )

private:
    Utility(){};

public:

    /**
     * What to show a playlist as. Everything except the pinned one goes by the
     * name in its row; that one has no name of its own to show, because a row
     * in a database can only be written in one language and this is the one
     * playlist nobody chose the name of.
     *
     * Here rather than in the sidebar because the sidebar is not the only place
     * that says it: the menu that offers to add a track to a playlist says it
     * too, and it said "To listen" in Italian for exactly as long as there were
     * two answers to one question.
     */
    static QString playlistName( const PlaylistStore::PlaylistInfo &playlist );

    static const int buttonSize;
    static const int coverSize;
    static const int marginSize;

    /** where torrents keep their sparse files, one directory per info hash */
    static QString torrentsCachePath;

    /** where the fast resume blobs live, flat so that start up is one listing */
    static QString resumeCachePath;

    /**
     * Where @p count rows end up when the ones at @p rows are dropped just
     * before @p targetRow: the answer is the new order, as indexes into the old
     * one.
     *
     * It lives here, away from the widget that asks for it, because it is the
     * one part of a drag that is arithmetic rather than interface — and the one
     * part where an off-by-one is invisible until somebody drags a track down
     * by one place and watches it not move.
     */
    static QList<int> reorderedIndexes( int count, const QList<int> &rows, int targetRow );

    static QString elidedText( const QString &text, const Qt::TextElideMode &elideMode, const int &width, const QFont::Weight &weight );

    static QFont font( const QFont::Weight &weight = QFont::Normal, const int &delta = 0 );

    static QFont monoFont( const QFont::Weight &weight = QFont::Normal, const int &delta = 0 );

    // from trojita Util.cpp
    static QColor tintColor( const QColor &color, const QColor &tintColor );
};

#endif // UTILITY_H
