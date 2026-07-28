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

#include "PlaylistDelegate.h"

#include "../GrooveOffNamespace.h"
#include "../Roles.h"
#include "../SvgHandler.h"

#include <QAbstractItemView>
#include <QApplication>
#include <QPainter>

namespace {

/** the air on either side of the led */
const int ledMargin = 8;

/** what a led may grow to, however tall the row is */
const int ledMaximum = 14;

int ledSide( const QRect &row )
{
    return qBound( 8, row.height() - 4, ledMaximum );
}

/**
 * The led with its colour taken out. The theme has an "Online" and an
 * "Offline" and nothing between them, and what is wanted for a torrent nobody
 * has asked about yet is neither: same glass, same shape, no colour — a led
 * that is off. A red one would say it had been asked and found empty, which is
 * a different thing entirely.
 */
QImage colourless( QImage image )
{
    for( int y = 0; y < image.height(); ++y ) {
        for( int x = 0; x < image.width(); ++x ) {
            const QColor pixel = image.pixelColor( x, y );
            const int grey = qGray( pixel.red(), pixel.green(), pixel.blue() );

            image.setPixelColor( x, y, QColor( grey, grey, grey, pixel.alpha() ) );
        }
    }

    return image;
}

} // namespace

PlaylistDelegate::PlaylistDelegate( QObject *parent )
    : QStyledItemDelegate( parent )
{
    // The leds are rendered out of the same svg as every other icon here, and
    // that svg is recoloured whenever the palette changes. Keeping pixmaps of
    // it means throwing them away at exactly that moment.
    connect( The::svgHandler(), &SvgHandler::retinted, this, [this] { m_leds.clear(); } );
}

/*!
  \brief led: the pixmap for one state, rendered once and kept

  renderSvg() renders on every call — its key argument is a leftover that
  nothing looks at — and this runs for every row of every repaint, which
  includes the mouse merely crossing the sidebar. So the pixmaps are kept here.
*/
QPixmap PlaylistDelegate::led( int health, int side ) const
{
    const int key = health * 1000 + side;

    const auto cached = m_leds.constFind( key );
    if( cached != m_leds.constEnd() )
        return cached.value();

    // Out of the theme rather than out of two colours chosen by hand. These are
    // the same two leds the window used to carry at the top, they are tinted
    // with the palette like everything else, and picking a green of our own
    // would be undoing on purpose what the tinting is there to do.
    const QLatin1String element( health == GrooveOff::LiveSource ? "Online" : "Offline" );

    QPixmap pixmap = The::svgHandler()->renderSvg( element, side, side, element );

    if( health == GrooveOff::UnknownSource )
        pixmap = QPixmap::fromImage( colourless( pixmap.toImage() ) );

    m_leds.insert( key, pixmap );
    return pixmap;
}

/*!
  \brief drawPlayingMark: which playlist the music is coming from

  It is not always the one being looked at, and once those two can differ the
  question "why is this playing?" has to have an answer on screen. A triangle
  in the text colour — small, because it says where the music is rather than
  offering to do anything.
*/
void PlaylistDelegate::drawPlayingMark( QPainter *painter, const QStyleOptionViewItem &option,
                                        int right, int side ) const
{
    const QRect box( right - side, option.rect.center().y() - side / 2, side, side );

    QPolygonF triangle;
    triangle << QPointF( box.left(), box.top() )
             << QPointF( box.right(), box.center().y() )
             << QPointF( box.left(), box.bottom() );

    painter->save();
    painter->setRenderHint( QPainter::Antialiasing, true );
    painter->setPen( Qt::NoPen );
    painter->setBrush( option.palette.color( option.state & QStyle::State_Selected
                                             ? QPalette::HighlightedText
                                             : QPalette::WindowText ) );
    painter->drawPolygon( triangle );
    painter->restore();
}

/*!
  \brief paint: the row, with whatever it has to say at its right edge

  Two marks, either of which may be absent: the led belongs to a torrent
  playlist, the triangle to whichever playlist the music is coming from — which
  is often a list made by hand. So the right edge is walked leftwards and the
  name is elided to what is left. The first version of this drew the triangle
  after the point at which a row without a led had already returned, which is
  to say never on the one playlist most likely to be playing.
*/
void PlaylistDelegate::paint( QPainter *painter, const QStyleOptionViewItem &option,
                              const QModelIndex &index ) const
{
    QStyleOptionViewItem opt = option;
    initStyleOption( &opt, index );

    const QVariant health = index.data( PlaylistRoles::Health );
    const bool playing    = index.data( PlaylistRoles::Playing ).toBool();

    if( !health.isValid() && !playing ) {
        QStyledItemDelegate::paint( painter, option, index );
        return;
    }

    QStyle *style = opt.widget ? opt.widget->style() : QApplication::style();

    const int side = ledSide( option.rect );
    const int mark = side - 2;

    // The room is taken out of the text and not out of the row: shrinking the
    // rectangle the row is drawn with would stop the selection short of the
    // right edge, which looks like a painting fault rather than like a margin.
    // The width comes from the style rather than from arithmetic of our own,
    // which would be wrong the moment an icon changed size.
    const QRect textRect = style->subElementRect( QStyle::SE_ItemViewItemText, &opt, opt.widget );

    int reserved = ledMargin;
    if( health.isValid() )
        reserved += ledMargin + side;
    if( playing )
        reserved += ledMargin + mark;

    opt.text = opt.fontMetrics.elidedText( opt.text, Qt::ElideRight,
                                           qMax( 0, textRect.width() - reserved ) );

    style->drawControl( QStyle::CE_ItemViewItem, &opt, painter, opt.widget );

    int right = option.rect.right();

    if( health.isValid() ) {
        painter->drawPixmap( right - ledMargin - side, option.rect.center().y() - side / 2,
                             led( health.toInt(), side ) );
        right -= ledMargin + side;
    }

    if( playing )
        drawPlayingMark( painter, option, right - ledMargin, mark );
}

QSize PlaylistDelegate::sizeHint( const QStyleOptionViewItem &option,
                                 const QModelIndex &index ) const
{
    QSize size = QStyledItemDelegate::sizeHint( option, index );

    // A row is never wider than what can be seen of it. Left to itself the
    // view makes every row as wide as the longest name in the list, and a
    // torrent called "The Complete Recordings 1959-1972, Remastered [FLAC]"
    // pushes the right edge past the viewport — taking the led with it, out
    // of sight behind a horizontal scrollbar that has no business in a
    // sidebar. The name is elided instead, which is what a sidebar is for.
    if( const QAbstractItemView *view = qobject_cast<const QAbstractItemView *>( option.widget ) )
        size.setWidth( qMin( size.width(), view->viewport()->width() ) );

    return size;
}
