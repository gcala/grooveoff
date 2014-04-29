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


#include "SvgButton.h"
#include "SvgHandler.h"

#include <QMouseEvent>
#include <QPainter>

SvgButton::SvgButton( QWidget *parent )
    : IconButton( parent )
    , m_isEnabled(true)
    , m_isPlaying(false)
    , m_fileExists(false)
{
    connect (this, SIGNAL(clicked()), 
                   SLOT(clicked()) );
}

void SvgButton::enterEvent( QEvent * )
{
    setIcon( icon(), 3);
}

void SvgButton::leaveEvent( QEvent * )
{
    setIcon( icon(), m_isEnabled ? 6 : 3);
}

void SvgButton::mousePressEvent( QMouseEvent *me )
{
    setIcon( icon(), m_isEnabled ? 0 : 3);
    IconButton::mousePressEvent( me );
}

void SvgButton::mouseReleaseEvent( QMouseEvent *me )
{
    setIcon( icon(), m_isEnabled ? 0 : 3);
    IconButton::mouseReleaseEvent( me );
}

void SvgButton::clicked()
{
    // emit signal only if button is enabled
    if(m_isEnabled)
        emit buttonClicked();
}

void SvgButton::reloadContent( const QSize &sz )
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
    m_icon.next[0] = The::svgHandler()->renderSvg( "next", width, height, "next" ).toImage();
    m_icon.next[1] = The::svgHandler()->renderSvg( "next_active", width, height, "next_active" ).toImage();
    m_icon.next[2] = The::svgHandler()->renderSvg( "next_disabled", width, height, "next_disabled" ).toImage();
    m_icon.previous[0] = The::svgHandler()->renderSvg( "previous", width, height, "previous" ).toImage();
    m_icon.previous[1] = The::svgHandler()->renderSvg( "previous_active", width, height, "previous_active" ).toImage();
    m_icon.previous[2] = The::svgHandler()->renderSvg( "previous_disabled", width, height, "previous_disabled" ).toImage();
    m_icon.search[0] = The::svgHandler()->renderSvg( "Search", width, height, "Search" ).toImage();
    m_icon.search[1] = The::svgHandler()->renderSvg( "Search_active", width, height, "Search_active" ).toImage();
    m_icon.search[2] = The::svgHandler()->renderSvg( "Search_disabled", width, height, "Search_disabled" ).toImage();
    m_icon.browse[0] = The::svgHandler()->renderSvg( "Browse", width, height, "Browse" ).toImage();
    m_icon.browse[1] = The::svgHandler()->renderSvg( "Browse_active", width, height, "Browse_active" ).toImage();
    m_icon.batch[0] = The::svgHandler()->renderSvg( "BatchDownload", width, height, "BatchDownload" ).toImage();
    m_icon.batch[1] = The::svgHandler()->renderSvg( "BatchDownload_active", width, height, "BatchDownload_active" ).toImage();
    m_icon.download[0] = The::svgHandler()->renderSvg( "Download", width, height, "Download" ).toImage();
    m_icon.download[1] = The::svgHandler()->renderSvg( "Download_active", width, height, "Download_active" ).toImage();
    m_icon.redownload[0] = The::svgHandler()->renderSvg( "Redownload", width, height, "Redownload" ).toImage();
    m_icon.redownload[1] = The::svgHandler()->renderSvg( "Redownload_active", width, height, "Redownload_active" ).toImage();
    m_icon.stop[0] = The::svgHandler()->renderSvg( "Stop", width, height, "Stop" ).toImage();
    m_icon.stop[1] = The::svgHandler()->renderSvg( "Stop_active", width, height, "Stop_active" ).toImage();
    m_icon.trash[0] = The::svgHandler()->renderSvg( "Trash", width, height, "Trash" ).toImage();
    m_icon.trash[1] = The::svgHandler()->renderSvg( "Trash_active", width, height, "Trash_active" ).toImage();
    m_icon.remove[0] = The::svgHandler()->renderSvg( "Stop", width, height, "Stop" ).toImage();
    m_icon.remove[1] = The::svgHandler()->renderSvg( "Stop_active", width, height, "Stop_active" ).toImage();
    m_icon.clock[0] = The::svgHandler()->renderSvg( "Clock", width, height, "Clock" ).toImage();
    m_icon.clock[1] = The::svgHandler()->renderSvg( "Clock_active", width, height, "Clock_active" ).toImage();
    m_icon.aborted[0] = The::svgHandler()->renderSvg( "Aborted", width, height, "Aborted" ).toImage();
    m_icon.aborted[1] = The::svgHandler()->renderSvg( "Aborted_active", width, height, "Aborted_active" ).toImage();
    m_icon.warning[0] = The::svgHandler()->renderSvg( "Warning", width, height, "Warning" ).toImage();
    m_icon.warning[1] = The::svgHandler()->renderSvg( "Warning_active", width, height, "Warning_active" ).toImage();

    // mirroring icons if layout direction is right-to-left
    if( layoutDirection() == Qt::RightToLeft )
    {
        for ( int i = 0; i < 3; ++i )
        {
            m_icon.play[i] = m_icon.play[i].mirrored( true, false );
            m_icon.next[i] = m_icon.next[i].mirrored( true, false );
            m_icon.previous[i] = m_icon.previous[i].mirrored( true, false );
            m_icon.search[i] = m_icon.search[i].mirrored( true, false );
        }
        
        for ( int i = 0; i < 2; ++i )
        {
            m_icon.pause[i] = m_icon.pause[i].mirrored( true, false );
            m_icon.browse[i] = m_icon.browse[i].mirrored( true, false );
            m_icon.batch[i] = m_icon.batch[i].mirrored( true, false );
            m_icon.download[i] = m_icon.download[i].mirrored( true, false );
            m_icon.clock[i] = m_icon.clock[i].mirrored( true, false );
            m_icon.aborted[i] = m_icon.aborted[i].mirrored( true, false );
            m_icon.warning[i] = m_icon.warning[i].mirrored( true, false );
        }
    }

    setIcon( icon() );
}

void SvgButton::setButtonEnabled(bool ok)
{
    m_isEnabled = ok;
    setIcon( icon() );
}

void SvgButton::setExists(bool ok)
{
    m_fileExists = ok;
    setIcon( icon() );
}


void SvgButton::setPlaying( bool playing )
{
    if ( m_isPlaying == playing )
        return;

    setToolTip( playing ? trUtf8( "Pause" ) : trUtf8( "Play" ) );

    m_isPlaying = playing;
    setIcon( icon(), 4 );
}

QImage SvgButton::icon()
{
    if(m_isEnabled) {
        switch( type() ) {
        case Next:
            return m_icon.next[underMouse()];
        case Previous:
            return m_icon.previous[underMouse()];
        case Search:
            return m_icon.search[underMouse()];
        case Browse:
            return m_icon.browse[underMouse()];
        case Batch:
            return m_icon.batch[underMouse()];
        case Stop:
            return m_icon.stop[underMouse()];
        case Trash:
            return m_icon.trash[underMouse()];
        case Remove:
            return m_icon.remove[underMouse()];
        case Redownload:
            return m_icon.redownload[underMouse()];
        case Clock:
            return m_icon.clock[underMouse()];
        case Aborted:
            return m_icon.aborted[underMouse()];
        case Warning:
            return m_icon.warning[underMouse()];
        case PlayPause:
            if(m_isPlaying)
                return m_icon.pause[underMouse()];
            else
                return m_icon.play[underMouse()];
        case Download:
            if(m_fileExists)
                return m_icon.redownload[underMouse()];
            else
                return m_icon.download[underMouse()];
        }
    }

    // button disabled
    switch( type() ) {
        case Next:
            return m_icon.next[2];
        case Previous:
            return m_icon.previous[2];
        case Search:
            return m_icon.search[2];
    }

    // play button disabled
    return m_icon.play[2];
}
