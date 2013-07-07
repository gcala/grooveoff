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


#ifndef MATCHITEM_H
#define MATCHITEM_H

#include "song.h"
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
    MatchItem(const Song &song, QWidget *parent = 0);
    virtual ~MatchItem();

    bool coverFound() { return coverFound_; }
    void loadCover();
    const QString & artist() { return song_.artist(); }
    const QString & album() { return song_.album(); }

signals:
    void download(Song);

private slots:
    void downloadSlot();

private:
    Ui::MatchItem *ui_;
    Song song_;
    QString fileName_;
    bool coverFound_;

    void setupUi();
};

#endif // MATCHITEM_H