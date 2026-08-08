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


#ifndef TRACKSELECTOR_H
#define TRACKSELECTOR_H

#include "TrackInfo.h"

#include <QDialog>

namespace Ui {
class TrackSelector;
}

class QTreeWidgetItem;

/**
  * @class TrackSelector
  * @short What of a torrent goes into its playlist.
  *
  * Shown every time a torrent is added, with everything ticked, and reopened
  * later to change one's mind: two ways of doing the same thing would be one
  * too many. It appears even for a torrent holding a single track, because a
  * behaviour that is always the same is worth the extra click.
  *
  * A file has two states and no third: in the playlist or not. There used to be
  * a sticky refusal beside them, kept so that «Select all» would step over the
  * files somebody had thrown out; it is gone, and select all means all. A list
  * of refusals the program applies quietly makes people remember something they
  * cannot see anywhere, in order to contradict them on the most explicit
  * gesture there is.
  *
  * Unticking is therefore the same act as the bin on a row, and costs the same:
  * the track leaves every list and its bytes go with it, after one question if
  * any of them are in a list made by hand.
  *
  * Non audio files are hidden behind a switch rather than dropped, because an
  * extension nobody thought of would otherwise become a track that simply is
  * not there, with no way of telling why.
  */
class TrackSelector : public QDialog
{
    Q_OBJECT

public:
    /**
     * @param torrentName what to show in the header
     * @param tracks every file of the torrent, audio or not
     * @param chosen the files already in the playlist; empty means a new torrent,
     *        and then everything playable starts ticked
     */
    explicit TrackSelector( const QString &torrentName,
                            const QList<TrackInfoPtr> &tracks,
                            const QList<qint64> &chosen = QList<qint64>(),
                            QWidget *parent = nullptr );
    ~TrackSelector() override;

    /** the files to put in the playlist, in the order they have in the torrent */
    QList<TrackInfoPtr> selectedTracks() const;


private Q_SLOTS:
    void applyFilter( const QString &text );
    void toggleAllFiles( bool showAll );
    void selectAll();
    void selectNone();
    void updateCount();

private:
    void fill();

    /** whether the row is one the current filters let through */
    bool isVisible( const QTreeWidgetItem *item ) const;

    Ui::TrackSelector *ui;

    QList<TrackInfoPtr> m_tracks;
    QList<qint64>       m_chosen;
    bool                m_hasAudio;
};

#endif // TRACKSELECTOR_H
