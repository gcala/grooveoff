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

#include "nextbutton.h"

#include "svghandler.h"

#include <QMouseEvent>
#include <QPainter>

NextButton::NextButton( QWidget *parent ) : IconButton( parent )
    , m_isEnabled(false)
{
    connect (this, SIGNAL(clicked()), this, SLOT(clicked()) );
    setToolTip( trUtf8( "Next" ) );
}

void NextButton::enterEvent( QEvent * )
{
    setIcon( m_isEnabled ? m_icon.next[1] : m_icon.next[2], 3 );
}

void NextButton::leaveEvent( QEvent * )
{
    setIcon( m_isEnabled ? m_icon.next[0] : m_icon.next[2], 6 );
}

void NextButton::mousePressEvent( QMouseEvent *me )
{
    setIcon( m_isEnabled ? m_icon.next[0] : m_icon.next[2] );
    IconButton::mousePressEvent( me );
}

void NextButton::mouseReleaseEvent( QMouseEvent *me )
{
    setIcon( m_isEnabled ? m_icon.next[1] : m_icon.next[2] );
    IconButton::mouseReleaseEvent( me );
}

void NextButton::clicked()
{
    emit nextButtonClicked();
}

void NextButton::reloadContent( const QSize &sz )
{
    const int width  = sz.width();
    const int height = sz.height();
    //NOTICE this is a bit cumbersome, as Qt renders faster to images than to pixmaps
    // However we need the Image and generate the pixmap ourself - maybe extend the SvgHandler API
    m_icon.next[0] = The::svgHandler()->renderSvg( "next", width, height, "next" ).toImage();
    m_icon.next[1] = The::svgHandler()->renderSvg( "next_active", width, height, "next_active" ).toImage();
    m_icon.next[2] = The::svgHandler()->renderSvg( "next_disabled", width, height, "next_disabled" ).toImage();
    if( layoutDirection() == Qt::RightToLeft )
    {
        for ( int i = 0; i < 3; ++i )
        {
            m_icon.next[i] = m_icon.next[i].mirrored( true, false );
        }
    }
    setIcon( m_isEnabled ? m_icon.next[underMouse()] : m_icon.next[2] );
}

void NextButton::setButtonEnabled(bool ok)
{
    m_isEnabled = ok;
    setIcon( m_isEnabled ? m_icon.next[underMouse()] : m_icon.next[2] );
}
