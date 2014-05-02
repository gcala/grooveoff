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


#include "SessionManager.h"
#include "ui_SessionManager.h"
#include "SessionItem.h"
#include "DownloadItem.h"
#include "SessionReaderWriter.h"
#include "Utility.h"

#include <QDir>

SessionManager::SessionManager(const QString &path, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SessionManager),
    m_sessionsPath(path)
{
    ui->setupUi(this);
//     setWindowTitle(QLatin1String("GrooveOff - ") + trUtf8("Session Manager"));
    loadSessionFiles();
    connect(ui->sessionsList, SIGNAL(appendItem(const QByteArray &, const QString &)),
                              SLOT(addTrack(const QByteArray &, const QString &))
    );
}

SessionManager::~SessionManager()
{
    foreach(QString sessionName, m_changesFlag.keys()) {
        if(m_changesFlag[sessionName]) {
            The::sessionReaderWriter()->write(m_sessionsPath + sessionName + ".xml", m_trackCollection[sessionName]);
        }
    }

    delete ui;
}

void SessionManager::on_sessionsList_currentRowChanged(int currentRow)
{
    QString session = ui->sessionsList->item(currentRow)->data(Qt::UserRole).toString();

    if(m_currentSessionName == session)
        return;

    ui->tracksList->clear();

    // disable temporarily AutoScroll
    ui->tracksList->setAutoScroll(false);

    m_currentSessionName = session;

    QList<PlaylistItemPtr> tracks = m_trackCollection[session];

    foreach(PlaylistItemPtr track, tracks) {
        DownloadItem *item = new DownloadItem(track, this, GrooveOff::Track);
        
        connect( item, SIGNAL( removeMeFromSession(quint32) ),
                       SLOT( removeTrack(quint32) ));
        
        QListWidgetItem *wItem = new QListWidgetItem(ui->tracksList);
        wItem->setData(Qt::UserRole, track->song()->songID());
        ui->tracksList->addItem(wItem);
        ui->tracksList->setItemWidget(wItem, item);
        wItem->setSizeHint(QSize(Utility::coverSize + Utility::marginSize * 2,
                          Utility::coverSize + Utility::marginSize * 2));
    }

    ui->tracksList->setAutoScroll(true);
}

void SessionManager::on_closeButton_clicked()
{
    close();
}

void SessionManager::loadSessionFiles()
{
    QDir sessionPath(m_sessionsPath);
    QStringList sessions = sessionPath.entryList(QStringList() << "*.xml", QDir::Files, QDir::Name | QDir::IgnoreCase);

    foreach(QString session, sessions) {
        session.remove(".xml", Qt::CaseInsensitive);
        generateTrackList(session);

        SessionItem *sessionItem = new SessionItem(session, m_trackCollection[session].count(), this);
        if( session == QLatin1String( "default" ) )
            sessionItem->deleteButtonEnabled(false);
        else {
            connect( sessionItem, SIGNAL( remove(const QString &) ),
                                  SLOT( removeSession(const QString &) )
            );
        }
        QListWidgetItem *wItem = new QListWidgetItem;
        wItem->setData(Qt::UserRole, session);
        ui->sessionsList->addItem(wItem);
        ui->sessionsList->setItemWidget(wItem, sessionItem);
        wItem->setSizeHint(QSize(40,40));
    }

    ui->sessionsList->setCurrentRow(0);
}

void SessionManager::generateTrackList(const QString& sessionName)
{
    QList<PlaylistItemPtr> items = The::sessionReaderWriter()->read(m_sessionsPath + sessionName + QLatin1String(".xml"));
    generateTrackList(sessionName, items);
}

void SessionManager::generateTrackList(const QString& sessionName, QList< PlaylistItemPtr > tracklist)
{
    m_trackCollection[sessionName] = tracklist;
    m_changesFlag[sessionName] = false;
}


void SessionManager::removeSession(const QString &sessionName)
{
    for( int i = 0; i < ui->sessionsList->count(); i++) {
        QListWidgetItem *item = ui->sessionsList->item(i);
        if(item->data(Qt::UserRole) == sessionName) {
            ui->sessionsList->takeItem(ui->sessionsList->row(item));
            delete item;
            QFile::remove(m_sessionsPath + sessionName + QLatin1String(".xml"));
            m_trackCollection.remove(sessionName);
            m_changesFlag.remove(sessionName);
            break;
        }
    }
}

void SessionManager::removeTrack(quint32 songID)
{
    for( int i = 0; i < ui->tracksList->count(); i++) {
        QListWidgetItem *item = ui->tracksList->item(i);
        if(item->data(Qt::UserRole) == songID) {
            ui->tracksList->takeItem(ui->tracksList->row(item));
            delete item;
            foreach(PlaylistItemPtr track, m_trackCollection[m_currentSessionName]) {
                if(track->song()->songID() == songID)
                    m_trackCollection[m_currentSessionName].removeOne(track);
            }
            m_changesFlag[m_currentSessionName] = true;
            break;
        }
    }

    ((SessionItem *)ui->sessionsList->indexWidget(ui->sessionsList->currentIndex()))->updateNumTrack(ui->tracksList->count());
}

void SessionManager::addTrack(const QByteArray& data, const QString& sessionName)
{
    PlaylistItemPtr track = PlaylistItemPtr(new PlaylistItem());;
    QDataStream inStream( data );
    inStream >> track;

    // check if duplicated
    m_trackCollection[sessionName].append(track);

    // reload?
}
