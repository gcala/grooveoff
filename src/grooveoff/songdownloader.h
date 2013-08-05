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


#ifndef SONGDOWNLOADER_H
#define SONGDOWNLOADER_H

#include <QNetworkAccessManager>
#include <QVariantMap>
#include <QStringList>
#include <QNetworkRequest>

class QFile;

class SongDownloader : public QNetworkAccessManager
{
    Q_OBJECT

public:
    explicit SongDownloader(const QString &path,
                            const QString &fileName,
                            const uint &id,
                            const QString &token,
                            const QString &streamKey,
                            const QString &ip,
                            QObject *parent = 0);
    virtual ~SongDownloader();

    void download();
    void startDownload();
    void stopDownload();
    void setToken(const QString &token) {token_ = token;}

signals:
    void downloadCompleted(bool);
    void progress(const qint64 &bytesReceived, const qint64 &bytesTotal);

public slots:
    void onDownloadProgress(qint64,qint64);
    void onFinished(QNetworkReply*);
    void onReadyRead();
    void onReplyFinished();

private:
    QNetworkReply *reply_;
    QFile *file_;
    QString path_;
    QString fileName_;
    uint id_;
    QString token_;
    QString ip_;
    QString streamKey_;
    int currentJob_;
    QNetworkRequest mainRequest_;
    bool aborted_;
};

#endif // SONGDOWNLOADER_H
