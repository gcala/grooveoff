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


#include "grooveoff/songdownloader.h"
#include "grooveoff/utility.h"
#include "grooveoff/grooveoffnamespace.h"

#include <qjson/serializer.h>
#include <qjson/parser.h>
#include <QFile>
#include <QDebug>
#include <QDir>
#include <QNetworkReply>

/*!
  \brief SongDownloader: this is the SongDownloader constructor.
  \param path: save location
  \param fileName: file name of the song
  \param id: id of the song
  \param token: token of connection
  \param parent: The Parent Widget
*/
SongDownloader::SongDownloader(const QString &path,
                               const QString &fileName,
                               const uint &id,
                               const QString &token,
                               const QString &streamKey,
                               const QString &ip,
                               QObject *parent) :
    QNetworkAccessManager(parent),
    path_(path),
    fileName_(fileName),
    id_(id),
    token_(token),
    streamKey_(streamKey),
    ip_(ip)
{
    aborted_ = false;

    connect(this, SIGNAL(finished(QNetworkReply*)), this, SLOT(onFinished(QNetworkReply*)));

    download();
}

SongDownloader::~SongDownloader()
{
    deleteLater();
}

/*!
  \brief download: with the key we can start song download
  \return void
*/
void SongDownloader::download()
{
    currentJob_ = GrooveOff::SongJob;
    file_ = new QFile;
    file_->setFileName(path_ + QDir::separator() + fileName_.replace('/','-') + ".mp3");

    file_->open(QIODevice::WriteOnly);

    QUrl postData;
    postData.addQueryItem(QLatin1String("streamKey"), streamKey_);

    // common headers
    mainRequest_.setRawHeader(QByteArray("User-Agent"), Utility::userAgent);
    mainRequest_.setRawHeader(QByteArray("Referer"), QString("http://%1/JSQueue.swf?%2").arg(Utility::host).arg(Utility::jsqueue().at(1)).toAscii());
    mainRequest_.setUrl(QUrl(QString("http://%1/stream.php").arg(ip_)));
    mainRequest_.setHeader(QNetworkRequest::ContentTypeHeader, QLatin1String("application/x-www-form-urlencoded"));
    reply_ = post(mainRequest_, postData.encodedQuery());
    connect(reply_, SIGNAL(downloadProgress(qint64,qint64)), this, SLOT(onDownloadProgress(qint64,qint64)));
    connect(reply_, SIGNAL(readyRead()), this, SLOT(onReadyRead()));
    connect(reply_, SIGNAL(finished()), this, SLOT(onReplyFinished()));
}

/*!
  \brief stopDownload: abort a download
  \return void
*/
void SongDownloader::stopDownload()
{
    reply_->abort();
}

/*!
  \brief onFinished: manage end of download
  \return void
*/
void SongDownloader::onFinished(QNetworkReply *)
{
    if(!reply_->error() == QNetworkReply::NoError) { // if error occurred
        qDebug() << "GrooveOff ::" << "Reply Error ::" << reply_->errorString() << "for" << fileName_;

        emit downloadCompleted(false);
        if(currentJob_ == GrooveOff::SongJob) {
            if(file_->isOpen()) {
                file_->close();
                file_->remove();
                file_->deleteLater();
            }
        return;
        }
    }

    if(file_->isOpen()) {
        file_->close();
        file_->deleteLater();
    }

    emit downloadCompleted(true);
    qDebug() << "GrooveOff ::" << "Finished download of" << fileName_;
}

/*!
  \brief onReadyRead: write on disk
  \return void
*/
void SongDownloader::onReadyRead()
{
    file_->write(reply_->readAll());
}

/*!
  \brief onReplyFinished
  \return void
*/
void SongDownloader::onReplyFinished()
{
    if(file_->isOpen()) {
        file_->close();
        file_->deleteLater();
    }
}

void SongDownloader::onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
    emit progress(bytesReceived, bytesTotal);
}

#include "songdownloader.moc"
