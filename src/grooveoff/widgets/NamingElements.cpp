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

// this code was "borrowed" from ktp-kded-integration-module

#include "NamingElements.h"

#include <QScrollBar>
#include <QDrag>
#include <QMimeData>

NamingElements::NamingElements(QWidget *parent)
: QListWidget(parent)
{
    setFlow(QListWidget::LeftToRight);
    setDragEnabled(true);

    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setHorizontalScrollMode(ScrollPerPixel);
}

NamingElements::~NamingElements()
{

}

void NamingElements::setLocalizedTagNames(QStringList tagNames)
{
    m_localizedTagNames = tagNames;
}

void NamingElements::setItemsIcons(QStringList itemsIcons)
{
    m_itemsIcons = itemsIcons;
}

void NamingElements::setupItems()
{
    QString tagName;

    //items adding order is based on that in config/telepathy-kded-config.cpp
    for (int i = 0; i < m_localizedTagNames.size(); i++) {
        tagName = m_localizedTagNames.at(i);
        tagName = tagName.right(tagName.size() - 1); //cut the '%' character
        tagName = tagName.left(1).toUpper() + tagName.mid(1); //capitalize tag name    

        QListWidgetItem *newItem = new QListWidgetItem(QIcon::fromTheme(m_itemsIcons.at(i), QIcon(QLatin1String(":/resources/" + m_itemsIcons.at(i).toLatin1() + ".png"))), tagName);
        addItem(newItem);
    }
}

void NamingElements::resizeEvent(QResizeEvent* event)
{
    QListWidget::resizeEvent(event);

    int height = sizeHintForRow(0) + 2 * ( frameWidth() + 3 ); // add 2*3 for top/bottom padding
    if (horizontalScrollBar() && horizontalScrollBar()->isVisible()) {
        height += horizontalScrollBar()->size().height();
    }

    setMaximumHeight(height);
}

void NamingElements::dragEnterEvent(QDragEnterEvent *event)
{
    event->accept();
}

void NamingElements::dragMoveEvent(QDragMoveEvent *event)
{
    event->accept();
}

void NamingElements::mousePressEvent(QMouseEvent *event)
{
    //after checking if we are in presence of a drag operation, we can then encapsulate
    //the data to be sent and let start the drag operation
    if (event->button() == Qt::LeftButton && itemAt(event->pos())) {
        QDrag *drag = new QDrag(this);
        QMimeData *mimeData = new QMimeData();

        mimeData->setText(itemAt(event->pos())->text()); //who receives expects plain text data
        drag->setMimeData(mimeData);
        drag->exec();
    }
}

QStringList NamingElements::mimeTypes() const
{
    QStringList types;
    types << QLatin1String("text/plain");
    return types;
}
