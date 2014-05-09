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


#include "PlaylistAction.h"
#include "Utility.h"

#include <QMouseEvent>
#include <QDebug>

PlaylistAction::PlaylistAction( const PlaylistItemPtr &track, QObject *parent )
    : QAction( parent )
    , m_track( track )
{
    setText( track->song()->songName() );
    
    connect( m_track.data(), SIGNAL(stateChanged(Phonon::State)),
                      SLOT(trackStateChanged(Phonon::State))
           );
    
    trackStateChanged( track->state() );
}

PlaylistAction::~PlaylistAction()
{
}
    

void PlaylistAction::trackStateChanged( Phonon::State state )
{
    switch( state ) {
        case Phonon::PlayingState:
            setFont( Utility::font( QFont::Bold ) );
            setIcon( QIcon( QLatin1String ( ":/resources/playPAUSE.png" ) ) );
            break;
        case Phonon::PausedState:
            setIcon( QIcon( QLatin1String ( ":/resources/PLAYpause.png" ) ) );
            setFont( Utility::font( QFont::Bold ) );
            break;
            
        default:
            setIcon( QIcon() );
            setFont( Utility::font( QFont::Normal ) );
    }
}
