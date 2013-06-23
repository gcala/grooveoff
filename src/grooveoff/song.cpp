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


#include "grooveoff/song.h"
#include "grooveoff/utility.h"

#include <QPixmapCache>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QIcon>
#include <QDebug>

/*!
  \brief Song: this is the Song constructor.
  \param parent: The Parent Widget
*/
Song::Song ( QObject* parent ) :
    QObject(parent)
{
    qnam_ = new QNetworkAccessManager(this);
    connect(qnam_, SIGNAL(finished(QNetworkReply*)), this, SLOT(onFinished(QNetworkReply*)));
}

/*!
  \brief ~Song: this is the Song destructor.
*/
Song::~Song()
{
    qnam_->deleteLater();
}

/*!
  \brief setCoverArtFilename: setup cover name
  \param cover: name of album cover
  \return void
*/
void Song::setCoverName(const QString &cover)
{
    coverName_ = cover;

    // 'NoCoverArt' is a custom name used to tell to use a default pixmap
    if(coverName_ == QLatin1String("NoCoverArt")) {
        if (!QPixmapCache::find(coverName_, &coverPixmap_)) {
            coverPixmap_ = QIcon::fromTheme(QLatin1String("media-optical"), QIcon(QLatin1String(":/resources/media-optical.png"))).pixmap(Utility::coverSize);
            coverPixmap_ = coverPixmap_.scaledToWidth(Utility::coverSize, Qt::SmoothTransformation);
            QPixmapCache::insert(QLatin1String("NoCoverArt"), coverPixmap_);
        }

        return;
    }

    // first of all search PixmapCache for current cover name and use it if found
    if (!QPixmapCache::find(coverName_, &coverPixmap_)) {
        //...not found; download it!
        QNetworkRequest request;
        request.setUrl(QUrl(QString("http://images.gs-cdn.net/static/albums/" + QString::number(Utility::coverSize) + "_%1").arg(coverName_)));
        request.setHeader(QNetworkRequest::ContentTypeHeader, QLatin1String("application/x-www-form-urlencoded"));
        qnam_->get(request);
    }
}

/*!
  \brief onFinished: reply finished
  \param reply: current reply object
  \return void
*/
void Song::onFinished(QNetworkReply *reply)
{
    switch(reply->error()) {

    case QNetworkReply::NoError:
        // if no error
        if (!QPixmapCache::find(coverName_, &coverPixmap_)) {
            coverPixmap_.loadFromData(reply->readAll());
            QPixmapCache::insert(coverName_, coverPixmap_);
        }
        break;

    default:
        // id reply returned error...
        qDebug() << "GrooveOff ::" << "Error downloading cover" << coverName_ << ":: " << reply->errorString();
        // use standard cover
        coverPixmap_ = QIcon::fromTheme(QLatin1String("media-optical"), QIcon(QLatin1String(":/resources/media-optical.png"))).pixmap(Utility::coverSize);
    }

    emit trigRepaint();
}

#include "song.moc"
