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

#include "TrackList.h"
#include "DownloadItem.h"

#include <QDrag>
#include <QMimeData>

TrackList::TrackList(QWidget *parent)
: QListWidget(parent)
{
    setDragEnabled(true);
    setAcceptDrops(true);
    setDragDropMode(QAbstractItemView::DragDrop);
    setDefaultDropAction(Qt::MoveAction);
}

TrackList::~TrackList()
{

}

void TrackList::dragEnterEvent(QDragEnterEvent *event)
{
    event->accept();
}

void TrackList::dragMoveEvent(QDragMoveEvent *event)
{
    event->accept();
}

void TrackList::mousePressEvent(QMouseEvent *event)
{
    //after checking if we are in presence of a drag operation, we can then encapsulate
    //the data to be sent and let start the drag operation
    if (event->button() == Qt::LeftButton && itemAt(event->pos())) {
        QByteArray itemData;
        QDataStream dataStream(&itemData, QIODevice::WriteOnly);
        dataStream << ((DownloadItem *)itemWidget(itemAt(event->pos())))->playlistItem();

        QMimeData *mimeData = new QMimeData();
        mimeData->setData(QLatin1String("application/x-grooveoff"), itemData);

        QDrag *drag = new QDrag(this);
        drag->setMimeData(mimeData);
        drag->exec();
    }
}

QStringList TrackList::mimeTypes() const
{
    QStringList types;
    types << QLatin1String("application/x-grooveoff");
    return types;
}
