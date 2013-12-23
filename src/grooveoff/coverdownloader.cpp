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

#include "coverdownloader.h"
#include "utility.h"

#include <QNetworkRequest>
#include <QNetworkReply>
#include <QFile>
#include <QDir>

CoverDownloader::CoverDownloader(QString name, QObject *parent) :
    QObject(parent),
    coverName_(name)
{
    QNetworkRequest request;
    request.setUrl(QUrl(QString("http://images.gs-cdn.net/static/albums/70_%1").arg(coverName_)));
    request.setHeader(QNetworkRequest::ContentTypeHeader, QLatin1String("application/x-www-form-urlencoded"));
    reply_ = qnam_.get(request);
    connect(reply_, SIGNAL(finished()), this, SLOT(downloadFinished()));
}

CoverDownloader::~CoverDownloader()
{
    reply_->deleteLater();
}


void CoverDownloader::downloadFinished()
{
    switch(reply_->error()) {
    case QNetworkReply::NoError: {
        // if no error
        QByteArray data = reply_->readAll();
        success_ = true;
        QFile file(Utility::coversCachePath + coverName_);
        file.open(QIODevice::WriteOnly);
        file.write(data);
        file.close();
        break;
    }
    default:
        qDebug() << "GrooveOff ::" << "Error downloading cover of" << coverName_;
        success_ = false;
    }

    emit done();
}

