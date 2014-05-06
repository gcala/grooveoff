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

#include "SessionList.h"

#include <QDropEvent>
#include <QMimeData>

SessionList::SessionList(QWidget *parent)
: QListWidget(parent)
{
    setDragEnabled(true);
    setAcceptDrops(true);
    setDragDropMode(QAbstractItemView::DropOnly);
//    setDefaultDropAction(Qt::MoveAction);
}

SessionList::~SessionList()
{

}

void SessionList::dragEnterEvent(QDragEnterEvent *event)
{
    event->accept();
}

void SessionList::dragMoveEvent(QDragMoveEvent *event)
{
    event->accept();
}

void SessionList::dropEvent(QDropEvent *event)
{
    const QMimeData *content = event->mimeData();

    QString sessionName = itemAt(event->pos())->data(Qt::UserRole).toString();
    //adapt the dropped text and insert it at current cursor position
    if (content->hasFormat(QLatin1String("application/x-grooveoff"))) {
        QByteArray data = content->data(QLatin1String("application/x-grooveoff"));
        emit appendItem(data, sessionName);
        //insert(text);

        setFocus();
        event->accept();
    } else
        event->ignore();
}
