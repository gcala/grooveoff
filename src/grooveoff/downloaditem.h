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
#include "grooveoff/song.h"

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
    DownloadItem(const QString &path, const QString &token, const Song &song, QWidget *parent = 0);
    virtual ~DownloadItem();
    QString songFile();

    const QString id() { return song_.id(); }
    const QString fileName() { return fileName_; }
    const QString title() { return song_.title(); }
    const QString album() { return song_.album(); }
    const QString artist() { return song_.artist(); }
    const QString coverName() { return song_.coverName(); }
    const QString & path() { return path_; }
    GrooveOff::DownloadState downloadState() { return downloadState_; }

    void startDownload();
    void setToken(const QString &token);
    void setPlayerState(Phonon::State);

    bool operator==(DownloadItem &) const;

signals:
    void play(DownloadItem *i);
    void remove(DownloadItem *i);
    void downloadFinished();
    void addToQueue(DownloadItem *);
    void pauseResumePlaying();
    void stateChangedSignal();

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
//    void reloadCover();

private:
    Ui::DownloadItem *ui_;
    QString path_;
    QString token_;
    QString fileName_;
    Song song_;
    QString ip_;
    SongDownloader *songDownloader_;
    GrooveOff::DownloadState downloadState_;
    Phonon::State playerState_;
    bool standardCover_;

    void setupUi();
    void setupConnections();
    void stateChanged();
};

#endif // DOWNLOADITEM_H
