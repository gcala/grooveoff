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


#include "Utility.h"

#include <QFontMetrics>
#include <QDir>
#include <QColor>

#include <algorithm>

const int Utility::coverSize  = 40; // possible values are 40,50,70,80,90,120
const int Utility::buttonSize = 24;
const int Utility::marginSize = 5;
QString Utility::torrentsCachePath = QString();
QString Utility::resumeCachePath = QString();

QList<int> Utility::reorderedIndexes( int count, const QList<int> &rows, int targetRow )
{
    QList<int> order;
    for( int i = 0; i < count; ++i )
        order << i;

    QList<int> moved;
    int insertAt = targetRow;

    QList<int> sorted = rows;
    std::sort( sorted.begin(), sorted.end() );

    // Backwards, so that taking one row out does not shift the ones not taken
    // yet. Every row that came from above the target pulls the target up with
    // it, which is the whole of the off-by-one this function exists for.
    for( int i = sorted.count() - 1; i >= 0; --i ) {
        const int row = sorted.at( i );
        if( row < 0 || row >= order.count() )
            return QList<int>();

        moved.prepend( order.takeAt( row ) );

        if( row < targetRow )
            --insertAt;
    }

    insertAt = qBound( 0, insertAt, order.count() );

    for( int i = 0; i < moved.count(); ++i )
        order.insert( insertAt + i, moved.at( i ) );

    return order;
}

QString Utility::elidedText( const QString& text,
                             const Qt::TextElideMode& elideMode,
                             const int& width,
                             const QFont::Weight &weight
                           )
{
    QFont systemFont;
    systemFont.setWeight(weight);
    QFontMetrics fm(systemFont);

    return fm.elidedText(text, elideMode, width);
}

QFont Utility::font( const QFont::Weight& weight, const int &delta )
{
    QFont systemFont;
    systemFont.setPointSize( systemFont.pointSize() + delta );
    systemFont.setWeight( weight );

    return systemFont;
}

QFont Utility::monoFont( const QFont::Weight& weight, const int &delta )
{
    QFont systemFont;
    systemFont.setStyleHint( QFont::Monospace );
    systemFont.setPointSize( systemFont.pointSize() + delta );
    systemFont.setWeight( weight );

    return systemFont;
}

/** @short Return the source color tinted with the tintColor

This is shamelessly stolen from Qt5's qtquick1 module.
*/
QColor Utility::tintColor( const QColor &color, const QColor &tintColor )
{
    QColor finalColor;
    int a = tintColor.alpha();
    if( a == 0xff ) {
        finalColor = tintColor;
    } else if( a == 0x00 ) {
        finalColor = color;
    } else {
        qreal a = tintColor.alphaF();
        qreal inv_a = 1.0 - a;

        finalColor.setRgbF( tintColor.redF()   * a + color.redF()   * inv_a,
                            tintColor.greenF() * a + color.greenF() * inv_a,
                            tintColor.blueF()  * a + color.blueF()  * inv_a,
                            a + inv_a * color.alphaF() 
                          );
    }
    return finalColor;
}
