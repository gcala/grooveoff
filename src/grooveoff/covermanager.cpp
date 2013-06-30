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
#include "song.h"
#include "coverdownloader.h"

CoverManager::CoverManager(QObject *parent) :
    QObject(parent)
{
}

void CoverManager::addItem(Song *song)
{
    QString coverName = song->coverName();
    if(coverItems_.contains(coverName)) {
        coverItems_[coverName].append(song);
    } else {
        QList<Song *> listOfSongsWithSameCover;
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
    if(downloader->isSuccess())
        emit coverDownloaded();
}

#include "covermanager.moc"
