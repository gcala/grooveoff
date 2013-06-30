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
    DownloadItem(const QString &path, const QString &title, const QString &album, const QString &artist, const QString &id,
                 const QString &token, const QString &name, QWidget *parent = 0);
    virtual ~DownloadItem();
    QString songFile();

    const QString & id() { return id_; }
    const QString & fileName() { return fileName_; }
    const QString & title() { return title_; }
    const QString & album() { return album_; }
    const QString & artist() { return artist_; }
    const QString & path() { return path_; }
    const QString & coverName() { return coverName_; }
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
    void reloadCover();

private:
    Ui::DownloadItem *ui_;
    QString path_;
    QString fileName_;
    QString title_;
    QString album_;
    QString artist_;
    QString id_;
    QString token_;
    QString coverName_;
    QString ip_;
    SongDownloader *song_;
    GrooveOff::DownloadState downloadState_;
    Phonon::State playerState_;
    bool standardCover_;

    void setupUi();
    void setupConnections();
    void stateChanged();
};

#endif // DOWNLOADITEM_H
