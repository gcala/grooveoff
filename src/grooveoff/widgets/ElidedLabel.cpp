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


#include "ElidedLabel.h"

#include <QPainter>
#include <QResizeEvent>
#include <QGraphicsDropShadowEffect>

ElidedLabel::ElidedLabel(QWidget* parent, Qt::WindowFlags f):
    QLabel(parent, f),
    elideMode_(Qt::ElideRight)
{
    setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
//     QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect(this);
//     shadow->setBlurRadius(20.0);
//     shadow->setColor(palette().color(QPalette::Highlight));
//     shadow->setOffset(0.0);
//     setGraphicsEffect(shadow);
}

ElidedLabel::ElidedLabel(const QString& text, QWidget* parent, Qt::WindowFlags f):
    QLabel(text, parent, f),
    elideMode_(Qt::ElideRight)
{
    setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
}

ElidedLabel::ElidedLabel(const QString& text, Qt::TextElideMode elideMode, QWidget* parent, Qt::WindowFlags f) :
    QLabel(text, parent, f),
    elideMode_(elideMode)
{
    setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
}

void ElidedLabel::setText(const QString& text)
{
    QLabel::setText(text);
    cacheElidedText(geometry().width());
    setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
}

void ElidedLabel::cacheElidedText(int w)
{
    cachedElidedText_ = fontMetrics().elidedText(text(), elideMode_, w, Qt::TextShowMnemonic);
}

void ElidedLabel::paintEvent(QPaintEvent* e)
{
    if(elideMode_ == Qt::ElideNone) {
        QLabel::paintEvent(e);
    } else {
        QPainter p(this);
        p.drawText(0, 0,
                   geometry().width(),
                   geometry().height(),
                   alignment(),
                   cachedElidedText_);
    }
}

void ElidedLabel::resizeEvent(QResizeEvent* e)
{
    QLabel::resizeEvent(e);
    cacheElidedText(e->size().width());
}

