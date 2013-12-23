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


#include "downloadlist.h"
#include "downloaditem.h"
#include "utility.h"
#include "playlist.h"
#include "audioengine.h"

#include <QDir>

#define ITEM(x)  ((DownloadItem *)itemWidget(item(x)))

DownloadList::DownloadList(QWidget *parent) :
    QListWidget(parent)
{
    connect(model(), SIGNAL(layoutChanged()),
            this, SLOT(reloadPlaylist()));
}

void DownloadList::reloadPlaylist()
{
    Playlist::instance()->clear();

    for(int i = 0; i < count(); i++) {
        if(ITEM(i)->downloadState() == GrooveOff::FinishedState) {
            Playlist::instance()->appendItem(ITEM(i)->playlistItem());
        }
    }
}

/*!
  \brief removeFailedDeletedAborted : remove failed/deleted/aborted items from download list
  \return void
*/
void DownloadList::removeFailedDeletedAborted()
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
void DownloadList::removeDownloaded()
{
    AudioEngine::instance()->stop();
    for(int i = count() - 1; i >= 0; i--) {
        GrooveOff::DownloadState state = ITEM(i)->downloadState();
        if(state == GrooveOff::FinishedState) {
            AudioEngine::instance()->removingTrack(ITEM(i)->playlistItem());
            QListWidgetItem *item = takeItem(i);
            removeItemWidget(item);
            delete item;
        }
    }

    Playlist::instance()->clear();
}

void DownloadList::abortAllDownloads()
{
    for(int i = count() - 1; i >= 0; i--) {
        GrooveOff::DownloadState state = ITEM(i)->downloadState();
        if(state == GrooveOff::DownloadingState || state == GrooveOff::QueuedState) {
            ITEM(i)->abortDownload();
        }
    }
}

