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

// Slightly modified for GrooveOff

#include "PlayPauseButton.h"
#include "SvgHandler.h"

#include <QMouseEvent>
#include <QPainter>
#include <QDebug>


PlayPauseButton::PlayPauseButton( QWidget *parent ) : IconButton( parent )
    , m_isPlaying( false )
    , m_isEnabled( false )
{
    connect (this, SIGNAL(clicked()), this, SLOT(clicked()) );
    setToolTip( trUtf8( "Play" ) );
}

void PlayPauseButton::enterEvent( QEvent * )
{
    setIcon( icon(), 3);
}

void PlayPauseButton::leaveEvent( QEvent * )
{
    setIcon( icon(), m_isEnabled ? 6 : 3);
}

void PlayPauseButton::mousePressEvent( QMouseEvent *me )
{
    setIcon( icon(), m_isEnabled ? 0 : 3);
    IconButton::mousePressEvent( me );
}

void PlayPauseButton::mouseReleaseEvent( QMouseEvent *me )
{
    setIcon( icon(), m_isEnabled ? 0 : 3);
    IconButton::mouseReleaseEvent( me );
}

void PlayPauseButton::clicked()
{
    // emit signal only if button is enabled
    if(m_isEnabled)
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
    setIcon( icon() );
}

void PlayPauseButton::setPlaying( bool playing )
{
    if ( m_isPlaying == playing )
        return;

    setToolTip( playing ? trUtf8( "Pause" ) : trUtf8( "Play" ) );

    m_isPlaying = playing;
    setIcon( icon(), 4 );
}

void PlayPauseButton::setButtonEnabled(bool ok)
{
    qDebug() << (ok ? "abilitato" : "disabilitato");
    m_isEnabled = ok;
    setIcon( icon() );
}

QImage PlayPauseButton::icon()
{
    if(m_isEnabled) {
        if(m_isPlaying)
            return m_icon.pause[underMouse()];
        else
            return m_icon.play[underMouse()];
    }

    return m_icon.play[2];
}



