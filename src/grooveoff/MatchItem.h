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


#ifndef MATCHITEM_H
#define MATCHITEM_H

#include "PlaylistItem.h"

#include <QWidget>
#include <qvarlengtharray.h>


namespace Ui {
class MatchItem;
}

class QLabel;
class QPushButton;

class MatchItem : public QWidget
{
    Q_OBJECT

public:
    explicit MatchItem( const PlaylistItemPtr &playlistItem, QWidget *parent = 0 );
    virtual ~MatchItem();

    inline const PlaylistItemPtr playlistItem() {
        return m_playlistItem;
    }

Q_SIGNALS:
    void download( PlaylistItemPtr );

public Q_SLOTS:
    void setDownloadIcon();

private Q_SLOTS:
    void loadCover();
    void downloadItem();

private:
    Ui::MatchItem *ui;
    PlaylistItemPtr m_playlistItem;

    void setupShadows();
    void setupLabels();
    void setupWidgetsSizes();
    void setupConnections();
};

#endif // MATCHITEM_H
