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


#ifndef MATCHITEM_H
#define MATCHITEM_H

#include "PlaylistItem.h"

#include <QWidget>


namespace Ui {
class MatchItem;
}

class QLabel;
class QPushButton;

class MatchItem : public QWidget
{
    Q_OBJECT

public:
    MatchItem(const PlaylistItemPtr &playlistItem, QWidget *parent = 0);
    virtual ~MatchItem();

    const PlaylistItemPtr playlistItem() { return playlistItem_; }

Q_SIGNALS:
    void download(PlaylistItemPtr);

public Q_SLOTS:
    void setDownloadIcon();

private Q_SLOTS:
    void loadCover();
    void downloadSlot();

private:
    Ui::MatchItem *ui_;
    PlaylistItemPtr playlistItem_;

    void setupUi();
};

#endif // MATCHITEM_H
