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


#include "playlist.h"
#include "grooveoff/downloaditem.h"
#include "grooveoff/utility.h"

#include <QDir>

#define ITEM(x)  ((DownloadItem *)itemWidget(item(x)))

Playlist::Playlist(QWidget *parent) :
    QListWidget(parent)
{
    connect(model(), SIGNAL(layoutChanged()), this, SLOT(reloadPlaylist()));
}

void Playlist::reloadPlaylist()
{
    Utility::audioSources.clear();
    Utility::playlist.clear();

    for(int i = 0; i < count(); i++)
        if(ITEM(i)->downloadState() == GrooveOff::FinishedState) {
            Utility::audioSources.append(ITEM(i)->song().data()->source());
            Utility::playlist.append(ITEM(i)->song());
        }
}

void Playlist::cambioStato(Phonon::State newState, QString source)
{
    for(int i = 0; i < count(); i++) {
        if(source == ITEM(i)->path() + QDir::separator() + ITEM(i)->fileName() + ".mp3") {
            ITEM(i)->setPlayerState(newState);
            break;
        }
    }
}

/*!
  \brief removeFailedDeletedAborted : remove failed/deleted/aborted items from download list
  \return void
*/
void Playlist::removeFailedDeletedAborted()
{
    for(int i = count() - 1; i >= 0; i--) {
        GrooveOff::DownloadState state = ITEM(i)->downloadState();
        if(state == GrooveOff::AbortedState ||
           state == GrooveOff::DeletedState ||
           state == GrooveOff::ErrorState ) {
            QListWidgetItem *item = takeItem(i);
            removeItemWidget(item);
            delete item;
        }
    }
}

/*!
  \brief removeDownloaded : remove successfully downloaded items from download list
  \return void
*/
void Playlist::removeDownloaded()
{
    for(int i = count() - 1; i >= 0; i--) {
        GrooveOff::DownloadState state = ITEM(i)->downloadState();
        if(state == GrooveOff::FinishedState) {
            QListWidgetItem *item = takeItem(i);
            removeItemWidget(item);
            delete item;
        }
    }
}

DownloadItem * Playlist::elemento(uint id)
{
    int i;
    for(i = 0; i < count(); i++) {
        if(id == ITEM(i)->id()) {
            break;
        }
    }

    return ITEM(i);
}


#include "playlist.moc"
