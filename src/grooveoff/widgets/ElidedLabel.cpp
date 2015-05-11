/*
    GrooveOff - Offline Grooveshark.com music
    Copyright (C) 2013-2015  Giuseppe Calà <jiveaxe@gmail.com>

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

ElidedLabel::ElidedLabel(QWidget* parent, Qt::WindowFlags f)
    : QLabel(parent, f)
    , m_elideMode(Qt::ElideRight)
    , m_drawShadow(false)
    , m_shadowEffect(0)
{
    setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
}

ElidedLabel::ElidedLabel(const QString& text, QWidget* parent, Qt::WindowFlags f)
    : QLabel(text, parent, f)
    , m_elideMode(Qt::ElideRight)
    , m_drawShadow(false)
{
    setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
}

ElidedLabel::ElidedLabel(const QString& text, Qt::TextElideMode elideMode, QWidget* parent, Qt::WindowFlags f)
    : QLabel(text, parent, f)
    , m_elideMode(elideMode)
    , m_drawShadow(false)
{
    setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
}

void ElidedLabel::setText(const QString& text)
{
    QLabel::setText(text);
    cacheElidedText(geometry().width());
    setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    if(m_drawShadow) {
        setVisible(false);
        setVisible(true);
    }
}

void ElidedLabel::cacheElidedText(int w)
{
    m_cachedElidedText = fontMetrics().elidedText(text(), m_elideMode, w, Qt::TextShowMnemonic);
}

void ElidedLabel::paintEvent(QPaintEvent* e)
{
    if(m_elideMode == Qt::ElideNone) {
        QLabel::paintEvent(e);
    } else {
        QPainter p(this);
        p.drawText(0, 0,
                   geometry().width(),
                   geometry().height(),
                   alignment(),
                   m_cachedElidedText);
    }
}

void ElidedLabel::resizeEvent(QResizeEvent* e)
{
    QLabel::resizeEvent(e);
    cacheElidedText(e->size().width());
}

void ElidedLabel::enableShadow(bool ok)
{
    m_drawShadow = ok;
    if(!m_shadowEffect) {
        m_shadowEffect = new QGraphicsDropShadowEffect(this);
        m_shadowEffect->setBlurRadius(15.0);
        m_shadowEffect->setColor(palette().color(QPalette::Shadow));
        m_shadowEffect->setOffset(0.0);
        setGraphicsEffect(m_shadowEffect);
    }
}

void ElidedLabel::changeEvent(QEvent* event)
{
    if(m_shadowEffect) {
        if(event->type() == QEvent::PaletteChange) {
            m_shadowEffect->setColor(palette().color(QPalette::Shadow));
        }
    }
    
    QLabel::changeEvent(event);
}
