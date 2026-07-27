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

/*!
  \brief setText: sets the text, and nothing else

  What used to be here did two more things, and both of them were somebody
  else's business. It reset the size policy, undoing whatever the container had
  asked for — DownloadItem sets one deliberately, with a comment reading "fix
  hidden label", which is what fighting this looks like from the outside. And it
  hid and showed the widget again to make the drop shadow repaint, which is a
  layout round trip to achieve what update() achieves.
*/
void ElidedLabel::setText(const QString& text)
{
    QLabel::setText(text);
    update();
}

/*!
  \brief paintEvent: the text is elided here, against the width it really has

  It used to be elided in setText, against whatever geometry the widget happened
  to have at that moment, and kept in a cache refreshed only by a resize. Called
  while the label was squeezed — which in a track row happens the moment the
  info message appears beside it — that cached an empty string, and with no
  resize afterwards to correct it the label stayed blank for good. The title of
  a track vanished the first time its tags were read, and never came back.

  Eliding at paint time cannot go stale: there is no width but the current one.
*/
void ElidedLabel::paintEvent(QPaintEvent* e)
{
    if(m_elideMode == Qt::ElideNone) {
        QLabel::paintEvent(e);
        return;
    }

    QPainter p(this);
    p.drawText(rect(),
               alignment(),
               fontMetrics().elidedText(text(), m_elideMode, width(), Qt::TextShowMnemonic));
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
