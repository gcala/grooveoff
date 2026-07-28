/****************************************************************************************
 * Copyright (c) 2008 Nikolaj Hald Nielsen <nhn@kde.org>                                *
 * Copyright (c) 2008 Jeff Mitchell <kde-dev@emailgoeshere.com>                         *
 * Copyright (c) 2009-2013 Mark Kretschmann <kretschmann@kde.org>                       *
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

#include "SvgHandler.h"

#include "App.h"
#include "PaletteHandler.h"
#include "SvgTinter.h"

#include <QHash>
#include <QPainter>
#include <QPalette>
#include <QReadLocker>
#include <QStyleOptionSlider>
#include <QWriteLocker>

#define SHARE_PATH "/../share/apps/grooveoff"

namespace The {
    static SvgHandler* s_SvgHandler_instance = 0;

    SvgHandler* svgHandler()
    {
        if( !s_SvgHandler_instance )
            s_SvgHandler_instance = new SvgHandler();

        return s_SvgHandler_instance;
    }
}

SvgHandler::SvgHandler( QObject* parent )
    : QObject( parent )
    , m_themeFile(  QCoreApplication::applicationDirPath() + QLatin1String( SHARE_PATH "/resources/default-theme.svg" ) )
{
    connect( The::paletteHandler(), &PaletteHandler::newPalette, this, &SvgHandler::discardCache );
}

SvgHandler::~SvgHandler()
{
    qDeleteAll( m_renderers );
    m_renderers.clear();
}


bool SvgHandler::loadSvg( const QString& name )
{
    const QString &svgFilename = name;
    QSvgRenderer *renderer = new QSvgRenderer( The::svgTinter()->tint( svgFilename ) );

    if ( !renderer->isValid() )
    {
        delete renderer;
        return false;
    }
    QWriteLocker writeLocker( &m_lock );

    if( m_renderers[name] )
        delete m_renderers[name];

    m_renderers[name] = renderer;
    return true;
}


/*!
  \brief renderSvg: an element of the theme, at a size, as a pixmap

  Kept once rendered, which the keyname argument has always promised and never
  done: nothing read it, and every call rasterised the svg again. That is
  cheap enough on its own — about three tenths of a millisecond — and ruinous
  in bulk, because a single row of the track list builds five buttons that
  render several elements each, twice over, once for setType and once for the
  resize that follows. Forty renders a row, eleven milliseconds a row, and half
  a second of frozen window on a playlist of forty tracks.

  The entries go when the palette changes, which is when they stop being true:
  reTint() rebuilds the renderer and everything drawn from it is a shade out of
  date.
*/
QPixmap SvgHandler::renderSvg( const QString& keyname,
                               int width,
                               int height,
                               const QString& element,
                               const qreal opacity )
{
    const QString cacheKey = QStringLiteral( "%1|%2|%3x%4|%5" )
                                 .arg( keyname, element )
                                 .arg( width ).arg( height ).arg( opacity );

    {
        QReadLocker cacheLocker( &m_lock );
        const auto cached = m_pixmapCache.constFind( cacheKey );
        if( cached != m_pixmapCache.constEnd() )
            return cached.value();
    }

    QPixmap pixmap;
    pixmap = QPixmap( width, height );
    pixmap.fill( Qt::transparent );

    QReadLocker readLocker( &m_lock );
    if( ! m_renderers[m_themeFile] )
    {
        readLocker.unlock();
        if( !loadSvg( m_themeFile ) )
        {
            return pixmap;
        }
        readLocker.relock();
    }

    QPainter pt( &pixmap );
    pt.setOpacity( opacity );

    if ( element.isEmpty() )
        m_renderers[m_themeFile]->render( &pt, QRectF( 0, 0, width, height ) );
    else
        m_renderers[m_themeFile]->render( &pt, element, QRectF( 0, 0, width, height ) );

    pt.end();
    readLocker.unlock();

    QWriteLocker cacheLocker( &m_lock );
    m_pixmapCache.insert( cacheKey, pixmap );

    return pixmap;
}

void SvgHandler::discardCache()
{
    {
        QWriteLocker cacheLocker( &m_lock );
        m_pixmapCache.clear();
    }

    //redraw entire app....
    reTint();
    App::instance()->mainWindow()->update();
}

void SvgHandler::reTint()
{
    The::svgTinter()->init();
    if ( !loadSvg( m_themeFile ))
        qWarning() << "Unable to load theme file: " << m_themeFile;
    emit retinted();
}
