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


#include "ElidedComboBox.h"
#include "Utility.h"

#include <QStylePainter>

/*!
  \brief ElidedComboBox: Custom combobox
  \param parent: The Parent Widget
*/
ElidedComboBox::ElidedComboBox(QWidget* parent):
    QComboBox(parent)
{

}

void ElidedComboBox::paintEvent(QPaintEvent* e)
{
    QVariant itemData = this->itemData( this->currentIndex(), Qt::DisplayRole);

    if(itemData.isValid()) {
        QStylePainter p(this);
        p.setPen(palette().color(QPalette::Text));

        QStyleOptionComboBox opt;
        initStyleOption(&opt);
        p.drawComplexControl(QStyle::CC_ComboBox, opt);

        QPainter painter(this);
        painter.save();
        QRect rect = this->rect();
        rect.adjust(5,0,-5,0);

        painter.drawText(rect,
                         Qt::AlignLeft | Qt::AlignVCenter,
                         Utility::elidedText(itemData.toString(),
                                             Qt::ElideRight,
                                             rect.width()-10,
                                             QFont::Normal)
                        );

        painter.restore();
    } else {
        QComboBox::paintEvent(e);
    }
}

