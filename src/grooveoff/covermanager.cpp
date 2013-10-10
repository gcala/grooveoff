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

#include "covermanager.h"
#include "coverdownloader.h"
#include "utility.h"

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

void CoverManager::addItem(const SongItemPtr &song)
{
    QString coverArtFilename = song->info()->coverArtFilename();
    if(coverItems_.contains(coverArtFilename)) {
        coverItems_[coverArtFilename].append(song);
    } else {
        QList< SongItemPtr > listOfSongsWithSameCover;
        listOfSongsWithSameCover.append(song);
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
        foreach (SongItemPtr song, coverItems_.value(coverArtFilename)) {
            song.data()->requireCoverReload();
        }
    }
}

#include "covermanager.moc"
