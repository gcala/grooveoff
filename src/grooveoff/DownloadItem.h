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


#ifndef DOWNLOADITEM_H
#define DOWNLOADITEM_H

#include "GrooveOffNamespace.h"
#include "PlaylistItem.h"
#include <../libgrooveshark/downloader.h>

#include <QWidget>
#include <qlistwidget.h>
#include <QVariantMap>
#include <phonon/MediaObject>

namespace Ui {
class DownloadItem;
}

class QLabel;
class QPushButton;
class QProgressBar;
class TimerButton;
class QDir;

class DownloadItem : public QWidget
{
    Q_OBJECT

public:
    DownloadItem( const PlaylistItemPtr &playlistItemPtr, QWidget *parent = 0, GrooveOff::Context context = GrooveOff::Download );
    virtual ~DownloadItem();
    QString songFile();

    inline const PlaylistItemPtr playlistItem() {
        return m_playlistItem;
    }
    
    inline GrooveOff::DownloadState downloadState() {
        return m_downloadState;
    }
    
    inline void setDownloadState(GrooveOff::DownloadState state) {
        m_downloadState = state;
    }

    void startDownload();
    void abortDownload();

    bool operator==( DownloadItem & ) const;

public Q_SLOTS:
    void setPlayerState( Phonon::State );

Q_SIGNALS:
    void downloadFinished();
    void addToQueue( DownloadItem * );
    void pauseResumePlaying();
    void stateChangedSignal();
    void play( QString );
    void reloadPlaylist();
    void removeMeFromSession( quint32 );
    void removeMe( DownloadItem * );

protected:
    virtual void leaveEvent ( QEvent * event );
    virtual void enterEvent ( QEvent * event );

private Q_SLOTS:
    void downloadFinished( bool ok );
    void setProgress( const qint64 &bytesReceived, const qint64 &bytesTotal );
    void playSong();
    void timerButtonClicked();
    void removeSong();
    void openFolder();
    void loadCover();
    void stopDownload();
    void deleteItem();
    void unqueueItem();
    void queueItem();

private:
    Ui::DownloadItem *ui;
    PlaylistItemPtr m_playlistItem;
    GrooveOff::DownloadState m_downloadState;
    Phonon::State m_playerState;
    GrooveShark::DownloaderPtr m_downloader;
    bool m_oneShot;
    GrooveOff::Context m_context;

    void setupUi();
    void setupConnections();
    void stateChanged();
    void removeEmptyFolder( QDir folder );
};

#endif // DOWNLOADITEM_H
