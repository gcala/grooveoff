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

#ifndef SVGHANDLER_H
#define SVGHANDLER_H

#include <QReadWriteLock>
#include <QSvgRenderer>

#include <QHash>
#include <QPixmap>
#include <QString>

class QStyleOptionSlider;

class SvgHandler;

namespace The {
    SvgHandler* svgHandler();
}

/**
A class to abstract out some common operations of users of tinted svgs
*/
class SvgHandler : public QObject
{
    Q_OBJECT
    friend SvgHandler* The::svgHandler();

    public:
        ~SvgHandler();

        /**
        * Overloaded function that uses the current theme
        * @param keyname the name of the key to save in the cache
        * @param width Width of the resulting pixmap
        * @param height Height of the resulting pixmap
        * @param element The theme element to render ( if none the entire svg is rendered )
        * @param skipCache If true, the pixmap will always get rendered and never fetched from the cache.
        * @param opacity The opacity used for rendering. Range 0.0 to 1.0.
        * @return The svg element/file rendered into a pixmap
        */
        QPixmap renderSvg( const QString& keyname, int width, int height, const QString& element = QString(), const qreal opacity = 1.0 );

    public slots:
        void reTint();

    signals:
        void retinted();

    private slots:
        void discardCache();

    private:
        SvgHandler( QObject* parent = 0 );

        bool loadSvg( const QString& name );

        QHash<QString,QSvgRenderer*> m_renderers;
        QReadWriteLock m_lock;

        QString m_themeFile;
};

#endif
