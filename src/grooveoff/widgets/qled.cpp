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

#include "qled.h"

#include <QColor>
#include <QtGlobal>
#include <QPolygon>
#include <QPainter>
#include <QSvgRenderer>

/*!
  \brief QLed: this is the QLed constructor.
  \param parent: The Parent Widget
*/
QLed::QLed(QWidget *parent)
    : QWidget(parent)
{
    value_=false;
    onColor_=Green;
    offColor_=Red;
    shape_=Circle;
    shapes_ << QLatin1String(":/resources/circle_");
    colors_ << QLatin1String("red.svg") << QLatin1String("green.svg");

    renderer_ = new QSvgRenderer();
}

QLed::~QLed()
{
    delete renderer_;
}

/*!
  \brief paintEvent: painting method
  \param QPaintEvent *
  \return void
*/
void QLed::paintEvent(QPaintEvent *)
{

    QString ledShapeAndColor;
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    ledShapeAndColor=shapes_[shape_];

    if(value_)
        ledShapeAndColor.append(colors_[onColor_]);
    else
        ledShapeAndColor.append(colors_[offColor_]);

    renderer_->load(ledShapeAndColor);
    renderer_->render(&painter);
}

/*!
  \brief setOnColor: this method allows to change the On color {Red,Green,Yellow,Grey,Orange,Purple,blue}
  \param ledColor newColor
  \return void
*/
void QLed::setOnColor(ledColor newColor)
{
   onColor_ = newColor;
   update();
}

/*!
  \brief setOffColor: this method allows to change the Off color {Red,Green,Yellow,Grey,Orange,Purple,blue}
  \param ledColor newColor
  \return void
*/
void QLed::setOffColor(ledColor newColor)
{
   offColor_=newColor;
   update();
}

/*!
  \brief setShape: this method allows to change the led shape {Circle,Square,Triangle,Rounded rectangle}
  \param ledColor newColor
  \return void
*/
void QLed::setShape(ledShape newShape)
{
   shape_=newShape;
   update();
}

/*!
  \brief setValue: this method allows to set the led value {true,false}
  \param ledColor newColor
  \return void
*/
void QLed::setValue(bool value)
{
   value_=value;
   update();
}

/*!
  \brief toggleValue: this method toggles the led value
  \param ledColor newColor
  \return void
*/
void QLed::toggleValue()
{
    value_=!value_;
    update();
    return;
}

