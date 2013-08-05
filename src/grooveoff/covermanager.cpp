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
#include "songobject.h"
#include "coverdownloader.h"

CoverManager::CoverManager(QObject *parent) :
    QObject(parent)
{
}

void CoverManager::addItem(const QSharedPointer<SongObject> &song)
{
    QString coverName = song.data()->coverName();
    if(coverItems_.contains(coverName)) {
        coverItems_[coverName].append(song);
    } else {
        QList< QSharedPointer<SongObject> > listOfSongsWithSameCover;
        listOfSongsWithSameCover.append(song);
        coverItems_.insert(coverName, listOfSongsWithSameCover);
        CoverDownloader *downloader = new CoverDownloader(coverName, this);
        connect(downloader, SIGNAL(done()), this, SLOT(setCover()));
    }
}

void CoverManager::clear()
{
    coverItems_.clear();
}

void CoverManager::setCover()
{
    CoverDownloader *downloader = (CoverDownloader *)QObject::sender();
    if(downloader->isSuccess()) {
        QString coverName = downloader->coverName();
        foreach (QSharedPointer<SongObject> song, coverItems_.value(coverName)) {
            song.data()->requireCoverReload();
        }
    }
//        emit coverDownloaded();
//    disconnect(downloader, SIGNAL(done()), this, SLOT(setCover()));
//    delete downloader;
//    downloader = 0;
}

#include "covermanager.moc"
