/***************************************************************************
 *   Copyright (C) 2010 by P. Sereno                                       *
 *   http://www.sereno-online.com                                          *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Lesser General Public License           *
 *   version 2.1 as published by the Free Software Foundation              *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Lesser General Public License for more details.                   *
 *   http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.               *
 ***************************************************************************/


#ifndef QLED_H
#define QLED_H

#include <QWidget>

// My Qt designer widget plugin class

class QColor;
class QSvgRenderer;

class QLed : public QWidget
{
    Q_OBJECT
    Q_ENUMS(ledColor)
    Q_ENUMS (ledShape)
    Q_PROPERTY(bool value READ value WRITE setValue);
    Q_PROPERTY(ledColor onColor READ onColor WRITE setOnColor);
    Q_PROPERTY(ledColor offColor READ offColor WRITE setOffColor);
    Q_PROPERTY(ledShape shape READ shape WRITE setShape)

public:
    explicit QLed(QWidget *parent = 0);
    virtual ~QLed();
    bool value() const { return value_; }
    enum ledColor { Red=0,Green };
    enum ledShape { Circle=0};
    ledColor onColor() const { return onColor_; }
    ledColor offColor() const { return offColor_; }
    ledShape shape() const { return shape_; }

public slots:
    void setValue(bool);
    void setOnColor(ledColor);
    void setOffColor(ledColor);
    void setShape(ledShape);
    void toggleValue();

protected:
    bool value_;
    ledColor onColor_, offColor_;
    int id_Timer_;
    ledShape shape_;
    QStringList shapes_;
    QStringList colors_;
    void paintEvent(QPaintEvent *event);

private:
    QSvgRenderer *renderer_ ;
};

#endif // QLED_H
