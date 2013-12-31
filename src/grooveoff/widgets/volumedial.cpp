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

#include "volumedial.h"

#include "PaletteHandler.h"
#include "svghandler.h"

#include <QConicalGradient>
#include <QCoreApplication>
#include <QMouseEvent>
#include <QPainter>
#include <QToolBar>
#include <QToolTip>
#include <QDebug>

#include <cmath>
#include <math.h>

static const qreal yc[3] = { 0.34375, 0.5, 0.15625 };

// normalize: like qBound(a, 0.0, 1.0) but without needing the args and with
// "safer" behavior on NaN (isnan(a) -> return 0.0)
static inline qreal normalize(qreal a)
{
    return (a < 1.0 ? (a > 0.0 ? a : 0.0) : 1.0);
}

qreal gamma(qreal n)
{
    return pow(normalize(n), 2.2);
}

VolumeDial::VolumeDial( QWidget *parent ) : QDial( parent )
    , m_isClick( false )
    , m_isDown( false )
    , m_muted( false )
    , m_mouseOver( false )
{
    m_anim.step = 0;
    m_anim.timer = 0;
    setMouseTracking( true );

    connect ( this, SIGNAL(valueChanged(int)), SLOT(valueChangedSlot(int)) );
    connect( The::paletteHandler(), SIGNAL(newPalette(QPalette)), SLOT(paletteChanged(QPalette)) );
}

VolumeDial::~VolumeDial()
{
}


void VolumeDial::addWheelProxies( QList<QWidget*> proxies )
{
    foreach ( QWidget *proxy, proxies )
    {
        if ( !m_wheelProxies.contains( proxy ) )
        {
            proxy->installEventFilter( this );
            connect ( proxy, SIGNAL(destroyed(QObject*)), this, SLOT(removeWheelProxy(QObject*)) );
            m_wheelProxies << proxy;
        }
    }
}

void VolumeDial::paletteChanged( const QPalette &palette )
{
    const QColor &fg = palette.color( foregroundRole() );
    const QColor &hg = palette.color( QPalette::Highlight );
    const qreal contrast = contrastRatio( hg, palette.color( backgroundRole() ) );
    m_highlightColor = mix( hg, fg, 1.0 - contrast/3.0 );
    renderIcons();
}

void VolumeDial::enterEvent( QEvent * )
{
    m_mouseOver = true;
    startFade();
}

// NOTICE: we intercept wheelEvents for ourself to prevent the tooltip hiding on them,
// see ::wheelEvent()
// this is _NOT_ redundant to the code in MainToolbar.cpp
bool VolumeDial::eventFilter( QObject *o, QEvent *e )
{
    if ( e->type() == QEvent::Wheel && !static_cast<QWheelEvent*>(e)->modifiers() )
    {
        if ( o == this || m_wheelProxies.contains( static_cast<QWidget*>( o ) ) )
        {
            QWheelEvent *wev = static_cast<QWheelEvent*>(e);
            if ( o != this )
            {
                QPoint pos( 0, 0 ); // the event needs to be on us or nothing will happen
                QWheelEvent nwev( pos, mapToGlobal( pos ), wev->delta(), wev->buttons(), wev->modifiers() );
                wheelEvent( &nwev );
            }
            else
                wheelEvent( wev );
            return true;
        }
        else // we're not needed globally anymore
            qApp->removeEventFilter( this );
    }
    return false;
}

void VolumeDial::leaveEvent( QEvent * )
{
    m_mouseOver = false;
    startFade();
}

static bool onRing( const QRect &r, const QPoint &p )
{
    const QPoint c = r.center();
    const int dx = p.x() - c.x();
    const int dy = p.y() - c.y();
    return sqrt(dx*dx + dy*dy) > r.width()/4;
}

void VolumeDial::mouseMoveEvent( QMouseEvent *me )
{
    if ( me->buttons() == Qt::NoButton )
        setCursor( onRing( rect(), me->pos() ) ? Qt::PointingHandCursor : Qt::ArrowCursor );
    else if ( m_isClick )
        me->accept();
    else
        QDial::mouseMoveEvent( me );
}

void VolumeDial::mousePressEvent( QMouseEvent *me )
{
    if ( me->button() != Qt::LeftButton )
    {
        QDial::mousePressEvent( me );
        return;
    }

    m_isClick = !onRing( rect(), me->pos() );

    if ( m_isClick )
        update(); // hide the ring
    else
    {
        setCursor( Qt::PointingHandCursor ); // hint dragging
        QDial::mousePressEvent( me ); // this will directly jump to the proper position
    }

    // for value changes caused by mouseevent we'll only let our adjusted value changes be emitted
    // see ::sliderChange()
    m_formerValue = value();
    blockSignals( true );
}

void VolumeDial::mouseReleaseEvent( QMouseEvent *me )
{
    if ( me->button() != Qt::LeftButton )
        return;

    blockSignals( false ); // free signals
    setCursor( Qt::ArrowCursor );
    setSliderDown( false );

    if ( m_isClick )
    {
        m_isClick = !onRing( rect(), me->pos() );
//         if ( m_isClick )
//             emit muteToggled( !m_muted );
        if ( m_isClick ) {
            m_muted = !m_muted;
            emit muteToggled( m_muted );
            setMuted( m_muted );
        }
    }

    m_isClick = false;
}

void VolumeDial::paintEvent( QPaintEvent * )
{
    int offset = m_mouseOver ? 4 : 0;
    QPainter p( this );
    int icon = m_muted ? 0 : 3;
    if ( icon && value() < 66 )
        icon = value() < 33 ? 1 : 2;
    icon += offset;
    p.drawPixmap( 0,0, m_icon[ icon ] );
    if ( !m_isClick )
    {
        p.setPen( QPen( m_sliderGradient, 4, Qt::SolidLine, Qt::RoundCap ) );
        p.setRenderHint( QPainter::Antialiasing );
        p.drawArc( rect().adjusted(4,4,-4,-4), -110*16, - value()*320*16 / (maximum() - minimum()) );
    }
    p.end();
}

void VolumeDial::removeWheelProxy( QObject *w )
{
    m_wheelProxies.removeOne( static_cast<QWidget*>(w) );
}

void VolumeDial::resizeEvent( QResizeEvent *re )
{
    if( width() != height() )
        resize( height(), height() );
    else
        QDial::resizeEvent( re );

    if( re->size() != re->oldSize() )
    {
        renderIcons();
        m_sliderGradient = QPixmap( size() );
        updateSliderGradient();
        update();
    }
}

void VolumeDial::renderIcons()
{
    m_icon[0] = The::svgHandler()->renderSvg( "Muted",      width(), height(), "Muted" );
    m_icon[1] = The::svgHandler()->renderSvg( "Volume_low", width(), height(), "Volume_low" );
    m_icon[2] = The::svgHandler()->renderSvg( "Volume_mid", width(), height(), "Volume_mid" );
    m_icon[3] = The::svgHandler()->renderSvg( "Volume",     width(), height(), "Volume" );
    m_icon[4] = The::svgHandler()->renderSvg( "Muted_active",      width(), height(), "Muted_active" );
    m_icon[5] = The::svgHandler()->renderSvg( "Volume_low_active", width(), height(), "Volume_low_active" );
    m_icon[6] = The::svgHandler()->renderSvg( "Volume_mid_active", width(), height(), "Volume_mid_active" );
    m_icon[7] = The::svgHandler()->renderSvg( "Volume_active",     width(), height(), "Volume_active" );
    if( layoutDirection() == Qt::RightToLeft )
    {
        for ( int i = 0; i < 8; ++i )
            m_icon[i] = QPixmap::fromImage( m_icon[i].toImage().mirrored( true, false ) );
    }
}

void VolumeDial::startFade()
{
    if ( m_anim.timer )
        killTimer( m_anim.timer );
    m_anim.timer = startTimer( 40 );
}

void VolumeDial::stopFade()
{
    killTimer( m_anim.timer );
    m_anim.timer = 0;
    if ( m_anim.step < 0 )
        m_anim.step = 0;
    else if ( m_anim.step > 6 )
        m_anim.step = 6;
}

void VolumeDial::timerEvent( QTimerEvent *te )
{
    if ( te->timerId() != m_anim.timer )
        return;
    if ( underMouse() ) // fade in
    {
        m_anim.step += 2;
        if ( m_anim.step > 5 )
            stopFade();
    }
    else // fade out
    {
        --m_anim.step;
        if ( m_anim.step < 1 )
            stopFade();
    }
    updateSliderGradient();
    repaint();
}

void VolumeDial::updateSliderGradient()
{
    m_sliderGradient.fill( Qt::transparent );
    QColor c = m_highlightColor;
    if ( !m_anim.step )
    {
        c.setAlpha( 99 );
        m_sliderGradient.fill( c );
        return;
    }

    QConicalGradient cg( m_sliderGradient.rect().center(), -90 );

    c.setAlpha( 99 + m_anim.step*156/6 );
    cg.setColorAt( 0, c );
    c.setAlpha( 99 + m_anim.step*42/6 );
    cg.setColorAt( 1, c );

    QPainter p( &m_sliderGradient );
    p.fillRect( m_sliderGradient.rect(), cg );
    p.end();
}

void VolumeDial::wheelEvent( QWheelEvent *wev )
{
    QDial::wheelEvent( wev );
    wev->accept();

    const QPoint tooltipPosition = mapToGlobal( rect().translated( 7, -22 ).bottomLeft() );
    QToolTip::showText( tooltipPosition, toolTip() );

    // NOTICE: this is a bit tricky.
    // the ToolTip "QTipLabel" just installed a global eventfilter that intercepts various
    // events and hides itself on them. Therefore every even wheelevent will close the tip
    // ("works - works not - works - works not - ...")
    // so we post-install our own global eventfilter to handle wheel events meant for us bypassing
    // the ToolTip eventfilter

    // first remove to prevent multiple installations but ensure we're on top of the ToolTip filter
    qApp->removeEventFilter( this );
    // it's ultimately removed in the timer triggered ::hideToolTip() slot
    qApp->installEventFilter( this );
}

void VolumeDial::setMuted( bool mute )
{
    m_muted = mute;

    setToolTip( m_muted ? trUtf8( "Muted" ) : trUtf8( "Volume: %1").arg(value()) );
    update();
}

QSize VolumeDial::sizeHint() const
{
    if ( QToolBar *toolBar = qobject_cast<QToolBar*>( parentWidget() ) )
        return toolBar->iconSize();

    return QDial::sizeHint();
}

void VolumeDial::sliderChange( SliderChange change )
{
    if ( change == SliderValueChange && isSliderDown() && signalsBlocked() )
    {
        int d = value() - m_formerValue;
        if ( d && d < 33 && d > -33 ) // don't allow real "jumps" > 1/3
        {
            if ( d > 5 ) // ease movement
                d = 5;
            else if ( d < -5 )
                d = -5;
            m_formerValue += d;
            blockSignals( false );
            emit sliderMoved( m_formerValue );
            emit valueChanged( m_formerValue );
            blockSignals( true );
        }
        if ( d )
            setValue( m_formerValue );
    }
    QDial::sliderChange(change);
}

void VolumeDial::valueChangedSlot( int v )
{
    m_isClick = false;

    setToolTip( m_muted ? trUtf8( "Muted" ) : trUtf8( "Volume: %1").arg(v) );
    update();
}

/*
 * From KColorUtils
 * */

// BEGIN internal helper functions
static inline qreal mixQreal(qreal a, qreal b, qreal bias)
{
    return a + (b - a) * bias;
}
// END internal helper functions

qreal VolumeDial::luma(const QColor &color)
{
    return gamma(color.redF())*yc[0] + gamma(color.greenF())*yc[1] + gamma(color.blueF())*yc[2];
}

static qreal contrastRatioForLuma(qreal y1, qreal y2)
{
    if (y1 > y2)
        return (y1 + 0.05) / (y2 + 0.05);
    else
        return (y2 + 0.05) / (y1 + 0.05);
}

qreal VolumeDial::contrastRatio(const QColor &c1, const QColor &c2)
{
    return contrastRatioForLuma(luma(c1), luma(c2));
}

QColor VolumeDial::mix(const QColor &c1, const QColor &c2, qreal bias)
{
    if (bias <= 0.0) return c1;
    if (bias >= 1.0) return c2;
    if (isnan(bias)) return c1;

    qreal r = mixQreal(c1.redF(),   c2.redF(),   bias);
    qreal g = mixQreal(c1.greenF(), c2.greenF(), bias);
    qreal b = mixQreal(c1.blueF(),  c2.blueF(),  bias);
    qreal a = mixQreal(c1.alphaF(), c2.alphaF(), bias);

    return QColor::fromRgbF(r, g, b, a);
}
