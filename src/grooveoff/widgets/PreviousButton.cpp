/*
 * GrooveOff - Offline Grooveshark.com music
 * Copyright (C) 2013  Giuseppe Calà <jiveaxe@gmail.com>
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

#include "PreviousButton.h"
#include "SvgHandler.h"

#include <QMouseEvent>
#include <QPainter>

PreviousButton::PreviousButton( QWidget *parent ) : IconButton( parent )
    , m_isEnabled(false)
{
    connect (this, SIGNAL(clicked()), this, SLOT(clicked()) );
//    setToolTip( trUtf8( "Previous" ) );
}

void PreviousButton::enterEvent( QEvent * )
{
    setIcon( m_isEnabled ? m_icon.previous[1] : m_icon.previous[2], 3 );
}

void PreviousButton::leaveEvent( QEvent * )
{
    setIcon( m_isEnabled ? m_icon.previous[0] : m_icon.previous[2], 6 );
}

void PreviousButton::mousePressEvent( QMouseEvent *me )
{
    setIcon( m_isEnabled ? m_icon.previous[0] : m_icon.previous[2] );
    IconButton::mousePressEvent( me );
}

void PreviousButton::mouseReleaseEvent( QMouseEvent *me )
{
    setIcon( m_isEnabled ? m_icon.previous[1] : m_icon.previous[2] );
    IconButton::mouseReleaseEvent( me );
}

void PreviousButton::clicked()
{
    // emit signal only if button is enabled
    if(m_isEnabled)
        emit previousButtonClicked();
}

void PreviousButton::reloadContent( const QSize &sz )
{
    const int width  = sz.width();
    const int height = sz.height();
    //NOTICE this is a bit cumbersome, as Qt renders faster to images than to pixmaps
    // However we need the Image and generate the pixmap ourself - maybe extend the SvgHandler API
    m_icon.previous[0] = The::svgHandler()->renderSvg( "previous", width, height, "previous" ).toImage();
    m_icon.previous[1] = The::svgHandler()->renderSvg( "previous_active", width, height, "previous_active" ).toImage();
    m_icon.previous[2] = The::svgHandler()->renderSvg( "previous_disabled", width, height, "previous_disabled" ).toImage();
    if( layoutDirection() == Qt::RightToLeft )
    {
        for ( int i = 0; i < 3; ++i )
        {
            m_icon.previous[i] = m_icon.previous[i].mirrored( true, false );
        }
    }
    setIcon( m_isEnabled ? m_icon.previous[underMouse()] : m_icon.previous[2] );
}

void PreviousButton::setButtonEnabled(bool ok)
{
    m_isEnabled = ok;
    setIcon( m_isEnabled ? m_icon.previous[underMouse()] : m_icon.previous[2] );
}

