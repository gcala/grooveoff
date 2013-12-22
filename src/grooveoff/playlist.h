/*
 * GrooveOff - Offline Grooveshark.com music
 * Copyright (C) 2013  Giuseppe Calà <Giuseppe.Cala-1973@poste.it>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef PLAYLIST_H
#define PLAYLIST_H

#include "playlistitem.h"

#include <QObject>

class Playlist : public QObject
{
    Q_OBJECT
public:
    static Playlist * instance();
    ~Playlist();

    void clear();
    void appendItem(PlaylistItemPtr item);
    void removeItem(PlaylistItemPtr item);
    int count() const;
    PlaylistItemPtr item(int row);
    int row(PlaylistItemPtr item);

signals:
    void playlistChanged();

private:
    static bool instanceFlag;
    static Playlist *playlist;
    Playlist();

    QList<PlaylistItemPtr> playlist_;
};

#endif // PLAYLIST_H
