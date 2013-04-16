/*
    GrooveOff - Offline Grooveshark.com music
    Copyright (C) 2013  Giuseppe Calà <jiveaxe@gmail.com>

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


#ifndef MATCHESLISTMODEL_H
#define MATCHESLISTMODEL_H

#include "grooveoff/song.h"

#include <QtCore/QAbstractItemModel>

class MatchesListModel : public QAbstractListModel
{
    Q_OBJECT

public:
    explicit MatchesListModel(QObject *parent = 0);

    virtual QVariant data ( const QModelIndex& index, int role = Qt::DisplayRole ) const;
    virtual int rowCount ( const QModelIndex& parent = QModelIndex() ) const;
    void setElements(QList<Song *> songs);
    void clear();

public slots:
    void forceRepaint();

private:
    QList<Song *> songs_;
};

#endif // MATCHESLISTMODEL_H
