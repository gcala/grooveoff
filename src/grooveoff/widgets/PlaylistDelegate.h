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

#ifndef PLAYLISTDELEGATE_H
#define PLAYLISTDELEGATE_H

#include <QHash>
#include <QPixmap>
#include <QStyledItemDelegate>

/**
 * @class PlaylistDelegate
 * @short A sidebar row, with the health of its source at the right edge.
 *
 * The led used to be one, at the top of the window, and it answered two
 * questions at once: whether the machine had a network, and whether the
 * torrent being listened to had peers. The first is a fact about the machine
 * and the second is a fact about one torrent out of several, so one widget
 * could only ever be right about one of them at a time.
 *
 * Here there is one led per torrent, which is where the question is actually
 * asked — the interesting torrent is the one being considered, not the one
 * already playing. A playlist made by hand gets none: it holds tracks of
 * several torrents, so its source has no single state to draw.
 */
class PlaylistDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    explicit PlaylistDelegate( QObject *parent = nullptr );

    void paint( QPainter *painter, const QStyleOptionViewItem &option,
                const QModelIndex &index ) const override;

    QSize sizeHint( const QStyleOptionViewItem &option,
                    const QModelIndex &index ) const override;

private:
    /** the led of one state at one size, out of the theme and kept afterwards */
    QPixmap led( int health, int side ) const;

    mutable QHash<int, QPixmap> m_leds;
};

#endif // PLAYLISTDELEGATE_H
