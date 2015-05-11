/*
    GrooveOff - Offline Grooveshark.com music
    Copyright (C) 2013-2015  Giuseppe Calà <jiveaxe@gmail.com>

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

#ifndef PLAYLISTACTION_H
#define PLAYLISTACTION_H

#include "PlaylistItem.h"

#include <QAction>

class PlaylistAction : public QAction
{
    Q_OBJECT
public:
    PlaylistAction(const PlaylistItemPtr &track, QObject *parent = 0);
    virtual ~PlaylistAction();
    
    inline PlaylistItemPtr track() const {
        return m_track;
    }
    
private Q_SLOTS:
    void trackStateChanged( Phonon::State state );
    
private:
    PlaylistItemPtr m_track;
};

#endif // PLAYLISTACTION_H
