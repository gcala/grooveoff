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


#ifndef DOWNLOADITEM_H
#define DOWNLOADITEM_H

#include "GrooveOffNamespace.h"
#include "PlaylistItem.h"
#include <../libgrooveshark/downloader.h>

#include <QWidget>
#include <QVariantMap>
#include <phonon/MediaObject>

namespace Ui {
class DownloadItem;
}

class QLabel;
class QPushButton;
class QProgressBar;
class TimerButton;

class DownloadItem : public QWidget
{
    Q_OBJECT

public:
    DownloadItem(const PlaylistItemPtr &playlistItemPtr, QWidget *parent = 0);
    virtual ~DownloadItem();
    QString songFile();

    const PlaylistItemPtr playlistItem() { return playlistItem_; }
    GrooveOff::DownloadState downloadState() { return downloadState_; }
    void setDownloadState(GrooveOff::DownloadState state) { downloadState_ = state; }

    void startDownload();
    void abortDownload();

    bool operator==(DownloadItem &) const;

public slots:
    void setPlayerState(Phonon::State);

signals:
//    void remove();
    void downloadFinished();
    void addToQueue(DownloadItem *);
    void pauseResumePlaying();
    void stateChangedSignal();
    void play(QString);

    void reloadPlaylist();

protected:
    virtual void leaveEvent ( QEvent * event );
    virtual void enterEvent ( QEvent * event );

private slots:
    void downloadFinished(bool ok);
    void setProgress(const qint64 &bytesReceived, const qint64 &bytesTotal);
    void playSong();
    void multiFuncBtnClicked();
    void removeSong();
    void openFolder();
    void loadCover();

private:
    Ui::DownloadItem *ui_;
    PlaylistItemPtr playlistItem_;
    GrooveOff::DownloadState downloadState_;
    Phonon::State playerState_;
    GrooveShark::DownloaderPtr downloader_;

    void setupUi();
    void setupConnections();
    void stateChanged();
};

#endif // DOWNLOADITEM_H
