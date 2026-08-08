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
#include "TrackInfo.h"

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

    /** the same for whoever holds the two fields without the whole row */
    static QString playlistName( const QString &storedName, bool pinned );

    static const int buttonSize;
    static const int coverSize;

    /**
     * Where «Donate» goes. Here because two places open it — the menu entry and
     * the button in the about box — and a url spelled twice eventually stops
     * agreeing with itself.
     */
    static const QString donateUrl;
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

    /**
     * The new contents of a torrent's playlist after the selector has been
     * used on it: what was already there keeps the order it was put in, and
     * what has just been ticked is dropped in where its place in the album
     * says — after the last track already there that comes before it.
     *
     * @p current is the playlist as it stands, @p chosen everything ticked, in
     * file order.
     *
     * Here rather than in the window for the same reason as the function above
     * it: taking a track out by mistake and putting it back is one of the two
     * things that dialog is for, and "back" has to mean where it was rather
     * than the end of the list. That is arithmetic, and arithmetic can be
     * asked without a window.
     */
    static QList<TrackInfoPtr> mergedSelection( const QList<TrackInfoPtr> &current,
                                                const QList<TrackInfoPtr> &chosen );

    /**
     * The name a magnet carries for itself, from its @c dn field, or an empty
     * string when it has none. It is the only thing known about a torrent
     * before its metadata arrive, and that wait is up to a minute long, so it
     * is what a row can be called meanwhile.
     *
     * Here rather than in the window for the reason the two functions above it
     * are: it is a question that can be asked without a window, hence one that
     * can be tested.
     */
    static QString magnetDisplayName( const QString &magnetUri );

    static QString elidedText( const QString &text, const Qt::TextElideMode &elideMode, const int &width, const QFont::Weight &weight );

    static QFont font( const QFont::Weight &weight = QFont::Normal, const int &delta = 0 );

    static QFont monoFont( const QFont::Weight &weight = QFont::Normal, const int &delta = 0 );

    // from trojita Util.cpp
    static QColor tintColor( const QColor &color, const QColor &tintColor );
};

#endif // UTILITY_H
