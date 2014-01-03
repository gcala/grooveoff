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


#include "CoverManager.h"
#include "CoverDownloader.h"
#include "Utility.h"

#include <QFile>
#include <QDir>

CoverManager::CoverManager(QObject *parent) :
    QObject(parent)
{
    if(!QFile::exists(Utility::coversCachePath)) {
            QDir dir;
            dir.mkdir(Utility::coversCachePath);
    }
}

void CoverManager::addItem(const PlaylistItemPtr &playlistItemPtr)
{
    QString coverArtFilename = playlistItemPtr->song()->coverArtFilename();
    if(coverArtFilename == "0")
        return;
    if(coverItems_.contains(coverArtFilename)) {
        coverItems_[coverArtFilename].append(playlistItemPtr);
    } else {
        QList< PlaylistItemPtr > listOfSongsWithSameCover;
        listOfSongsWithSameCover.append(playlistItemPtr);
        coverItems_.insert(coverArtFilename, listOfSongsWithSameCover);
        CoverDownloader *downloader = new CoverDownloader(coverArtFilename, this);
        connect(downloader, SIGNAL(done()), this, SLOT(setCover()));
    }
}

void CoverManager::clear()
{
    coverItems_.clear();
}

void CoverManager::setCover()
{
    CoverDownloader *coverDownloader = (CoverDownloader *)QObject::sender();
    if(coverDownloader->isSuccess()) {
        QString coverArtFilename = coverDownloader->coverName();
        foreach (PlaylistItemPtr playlistItem, coverItems_.value(coverArtFilename)) {
            playlistItem.data()->requireCoverReload();
        }
    }
}

