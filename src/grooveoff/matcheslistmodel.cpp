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


#include "grooveoff/matcheslistmodel.h"
#include "grooveoff/roles.h"

/*!
  \brief MatchesListModel: this is the MatchesListModel constructor.
  \param parent: The Parent Widget
*/
MatchesListModel::MatchesListModel(QObject *parent) :
    QAbstractListModel(parent)
{
    // Adding new roles
    QHash<int, QByteArray> roles;
    roles.insert(SongRoles::Title, QByteArray("title"));
    roles.insert(SongRoles::Album, QByteArray("album"));
    roles.insert(SongRoles::Artist, QByteArray("artist"));
    roles.insert(SongRoles::Year, QByteArray("year"));
    roles.insert(SongRoles::CoverName, QByteArray("coverName"));
    roles.insert(SongRoles::Id, QByteArray("id"));
    setRoleNames(roles);
}

/*!
  \brief data: return data from item
  \param index: item index
  \param role: data role
  \return variant
*/
QVariant MatchesListModel::data ( const QModelIndex& index, int role ) const
{
    if (!index.isValid())
        return QVariant();

    if (index.row() >= songs_.size())
        return QVariant();

    switch (role) {
        case SongRoles::Title:
            return QVariant(songs_.at(index.row())->title());
        case SongRoles::Artist:
            return QVariant(songs_.at(index.row())->artist());
        case SongRoles::Album:
            return QVariant(songs_.at(index.row())->album());
        case SongRoles::Id:
            return QVariant(songs_.at(index.row())->id());
        case SongRoles::CoverName:
             return QVariant(songs_.at(index.row())->coverName());
        default:
            return QVariant();
    }
}

/*!
  \brief rowCount: return number of elements
  \param parent: parent modelindex
  \return int
*/
int MatchesListModel::rowCount ( const QModelIndex& parent ) const
{
    Q_UNUSED(parent);
    return songs_.count();
}

/*!
  \brief setElements: setup model elements
  \param songs: used elements list
  \return void
*/
void MatchesListModel::setElements(QList<Song *> songs)
{
    clear();
    songs_ = songs;
}

/*!
  \brief clear: remove items
  \return void
*/
void MatchesListModel::clear()
{
    reset();

    for(int i = 0; i < songs_.count() -1; i++) {
        delete songs_[i];
    }

    songs_.clear();
}

/*!
  \brief forceRepaint: force repaint when a new cover is downloaded
  \return void
*/
void MatchesListModel::forceRepaint()
{
    emit dataChanged(index(0), index(2));
}

#include "matcheslistmodel.moc"
