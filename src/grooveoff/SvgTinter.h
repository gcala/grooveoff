/****************************************************************************************
 * Copyright (c) 2007 Nikolaj Hald Nielsen <nhn@kde.org>                                *
 * Copyright (c) 2007 Mark Kretschmann <kretschmann@kde.org>                            *
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

#ifndef SVGTINTER_H
#define SVGTINTER_H

#include <QColor>
#include <QFile>
#include <QHash>
#include <QString>
#include <QPalette>

class SvgTinter;

namespace The {
    SvgTinter* svgTinter();
}

/**
This singleton class is used to tint the svg artwork to attempt to better match the users color scheme.

	@author Nikolaj Hald Nielsen <nhn@kde.org>
*/
class SvgTinter
{
    friend SvgTinter* The::svgTinter();

public:
    ~SvgTinter();

    QByteArray tint( const QString &filename );
    void init();

    QColor blendColors( const QColor& color1, const QColor& color2, int percent );

private:
    SvgTinter();

    QHash<QByteArray, QString> m_tintMap;

    QPalette m_lastPalette;
    bool m_firstRun;
};

#endif
