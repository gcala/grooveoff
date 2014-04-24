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

#ifndef SESSIONMANAGER_H
#define SESSIONMANAGER_H

#include "PlaylistItem.h"
#include "../libgrooveshark/song.h"

#include <QDialog>
#include <qlistwidget.h>
#include <QModelIndex>

class QDomElement;

namespace Ui {
class SessionManager;
}

class SessionManager : public QDialog
{
    Q_OBJECT

public:
    explicit SessionManager(const QString &path, QWidget *parent = 0);
    ~SessionManager();

private Q_SLOTS:
    void on_sessionsList_currentRowChanged(int currentRow);
    void on_closeButton_clicked();
    void removeSession(const QString &name);
    void removeTrack(quint32 songID);
    void addTrack(const QByteArray &data, const QString &sessionName);

private:
    Ui::SessionManager *ui;
    const QString sessionsPath_;
    QHash< QString, QList<PlaylistItemPtr> > m_trackCollection;
    QHash< QString, bool > m_changesFlag;
    QString currentSessionName_;
    QList<PlaylistItemPtr> currentTracklist_;

    void loadSessionFiles();
    void generateTrackList(const QString &sessionName);
    void generateTrackList(const QString &sessionName, QList<PlaylistItemPtr> tracklist);
};

#endif // SESSIONMANAGER_H
