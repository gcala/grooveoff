/****************************************************************************************
* Copyright (c) 2009 Thomas Luebking <thomas.luebking@web.de>                          *
*                                                                                      *
* This program is free software; you can redistribute it and/or modify it under        *
* the terms of the GNU General Public License as published by the Free Software        *
* Foundation; either version 2 of the License, or (at your option) any later           *
* version.                                                                             *
*                                                                                      *
* This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
* WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
* PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
*                                                                                      *
* You should have received a copy of the GNU General Public License along with         *
* this program.  If not, see <http://www.gnu.org/licenses/>.                           *
****************************************************************************************/

#include "PlayPauseButton.h"
#include "SvgHandler.h"

#include <QMouseEvent>
#include <QPainter>


PlayPauseButton::PlayPauseButton( QWidget *parent ) : IconButton( parent )
    , m_isPlaying( false )
    , m_isEnabled( false )
{
    connect (this, SIGNAL(clicked()), this, SLOT(clicked()) );
    setToolTip( trUtf8( "Play" ) );
}

void PlayPauseButton::enterEvent( QEvent * )
{
    if(!m_isEnabled)
        setIcon( m_icon.play[2], 3 );
    else
        setIcon( m_isPlaying ? m_icon.pause[1] : m_icon.play[1], 3 );
}

void PlayPauseButton::leaveEvent( QEvent * )
{
    if(!m_isEnabled)
        setIcon( m_icon.play[2], 3 );
    else
        setIcon( m_isPlaying ? m_icon.pause[0] : m_icon.play[0], 6 );
}

void PlayPauseButton::mousePressEvent( QMouseEvent *me )
{
    if(!m_isEnabled)
        setIcon( m_icon.play[2], 3 );
    else
        setIcon( m_isPlaying ? m_icon.pause[0] : m_icon.play[0] );

    IconButton::mousePressEvent( me );
}

void PlayPauseButton::mouseReleaseEvent( QMouseEvent *me )
{
    if(!m_isEnabled)
        setIcon( m_icon.play[2], 3 );
    else
        setIcon( m_isPlaying ? m_icon.pause[1] : m_icon.play[1] );

    IconButton::mouseReleaseEvent( me );
}

void PlayPauseButton::clicked()
{
    emit playButtonClicked();
}

void PlayPauseButton::reloadContent( const QSize &sz )
{
    const int width  = sz.width();
    const int height = sz.height();
    //NOTICE this is a bit cumbersome, as Qt renders faster to images than to pixmaps
    // However we need the Image and generate the pixmap ourself - maybe extend the SvgHandler API
    m_icon.play[0] = The::svgHandler()->renderSvg( "PLAYpause", width, height, "PLAYpause" ).toImage();
    m_icon.play[1] = The::svgHandler()->renderSvg( "PLAYpause_active", width, height, "PLAYpause_active" ).toImage();
    m_icon.play[2] = The::svgHandler()->renderSvg( "PLAYpause_disabled", width, height, "PLAYpause_disabled" ).toImage();
    m_icon.pause[0] = The::svgHandler()->renderSvg( "playPAUSE", width, height, "playPAUSE" ).toImage();
    m_icon.pause[1] = The::svgHandler()->renderSvg( "playPAUSE_active", width, height, "playPAUSE_active" ).toImage();
    if( layoutDirection() == Qt::RightToLeft )
    {
        for ( int i = 0; i < 3; ++i )
        {
            m_icon.play[i] = m_icon.play[i].mirrored( true, false );
        }

        for ( int i = 0; i < 2; ++i )
        {
            m_icon.pause[i] = m_icon.pause[i].mirrored( true, false );
        }
    }
    setIcon( m_isEnabled ? ( m_isPlaying ? m_icon.pause[underMouse()] : m_icon.play[underMouse()] ) : m_icon.play[2] );
}

void PlayPauseButton::setPlaying( bool playing )
{
    if ( m_isPlaying == playing )
        return;

    setToolTip( playing ? trUtf8( "Pause" ) : trUtf8( "Play" ) );

    m_isPlaying = playing;
    setIcon( m_isPlaying ? m_icon.pause[underMouse()] : m_icon.play[underMouse()], 4 );
}

void PlayPauseButton::setButtonEnabled(bool ok)
{
    m_isEnabled = ok;
}


