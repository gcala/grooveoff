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


#ifndef NAMINGELEMENTS_H
#define NAMINGELEMENTS_H

#include <QListWidget>
#include <QDragEnterEvent>

class NamingElements : public QListWidget
{
     Q_OBJECT

public:
    NamingElements(QWidget *parent = 0);
    virtual ~NamingElements();

    void setLocalizedTagNames(QStringList tagNames);
    void setItemsIcons(QStringList itemsIcons);
    void setupItems();

protected:
    void resizeEvent(QResizeEvent *event);
    void dragEnterEvent(QDragEnterEvent *event);
    void dragMoveEvent(QDragMoveEvent *event);
    void mousePressEvent(QMouseEvent *event);
    QStringList mimeTypes() const;

private:
    QStringList m_localizedTagNames;
    QStringList m_itemsIcons;
};

#endif // NAMINGELEMENTS_H
