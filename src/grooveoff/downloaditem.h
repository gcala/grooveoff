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

#include <QWidget>
#include <QVariantMap>
#include <phonon/MediaObject>
#include "grooveoff/grooveoffnamespace.h"
#include "grooveoff/songobject.h"

namespace Ui {
class DownloadItem;
}

class QLabel;
class QPushButton;
class QProgressBar;
class SongDownloader;
class TimerButton;

class DownloadItem : public QWidget
{
    Q_OBJECT

public:
    DownloadItem(const QSharedPointer<SongObject> &song, const QString &token, QWidget *parent = 0);
    virtual ~DownloadItem();
    QString songFile();

    const QSharedPointer<SongObject> song() { return song_; }
    uint id() { return song_.data()->id(); }
    //------->
    const QString fileName() { return fileName_; }
    const QString title() { return song_.data()->title(); }
    const QString album() { return song_.data()->album(); }
    const QString artist() { return song_.data()->artist(); }
    const QString coverName() { return song_.data()->coverName(); }
    const QString path() { return song_.data()->path(); }
    //<-------
    GrooveOff::DownloadState downloadState() { return downloadState_; }

    void startDownload();
    void setToken(const QString &token);

    void setStreamKey(const QString &streamKey) { streamKey_ = streamKey; }
    void setIp(const QString &ip) { ip_ = ip; }

    bool operator==(DownloadItem &) const;

public slots:
    void setPlayerState(Phonon::State);

signals:
    void remove(DownloadItem *i);
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
    QSharedPointer<SongObject> song_;
    QString token_;
    QString fileName_;
    SongDownloader *songDownloader_;
    GrooveOff::DownloadState downloadState_;
    Phonon::State playerState_;

    QString ip_;
    QString streamKey_;

    void setupUi();
    void setupConnections();
    void stateChanged();
};

#endif // DOWNLOADITEM_H
