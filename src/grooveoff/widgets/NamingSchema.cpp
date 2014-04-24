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

// this code was "borrowed" from ktp-kded-integration-module

#include "NamingSchema.h"

#include <QLineEdit>
#include <QMimeData>
#include <QMouseEvent>
#include <QDropEvent>

NamingSchema::NamingSchema(QWidget *parent)
: QLineEdit(parent)
{

}

NamingSchema::~NamingSchema()
{

}

void NamingSchema::setLocalizedTagNames(QStringList tagNames)
{
    m_localizedTagNames = tagNames;
}

void NamingSchema::dropEvent(QDropEvent *event)
{
    const QMimeData *content = event->mimeData();

    //adapt the dropped text and insert it at current cursor position
    if (content->hasText()) {
        QString text = content->text();
        text = text.toLower();
        text = text.insert(0, QLatin1Char('%'));
        insert(text);

        setFocus();
        event->accept();
    } else
        event->ignore();
}

void NamingSchema::mousePressEvent(QMouseEvent *event)
{
    //small usability improvement:
    //if we detect that the click is inside a tag name then that tag will be auto selected
    if (event->button() == Qt::LeftButton) {
        int currentCursorPosition = cursorPositionAt(event->pos());

        Q_FOREACH (const QString &tag, m_localizedTagNames) {
            if(text().contains(tag)
            && currentCursorPosition >= text().indexOf(tag) //cursor must be inside tag's bounds
            && currentCursorPosition <= text().indexOf(tag) + tag.size()) {

                setSelection(text().indexOf(tag), tag.size());
                break;
            }
            else {
                //since we are intercepting mouse events, setting manually the cursor's position is needed
                setCursorPosition(currentCursorPosition);
            }
        }
    }
}
