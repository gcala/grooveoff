#include "downloader_p.h"
#include "apirequest.h"
#include "config.h"
#include "mapbuilder.h"

#include <QFile>
#include <QDir>
#include <QDebug>

using namespace GrooveShark;

DownloaderPrivate::DownloaderPrivate ( Downloader* qq, QString path, QString fileName, uint id, QString token, QObject* parent ) :
    QNetworkAccessManager ( parent ),
    m_path(path),
    m_fileName(fileName),
    m_id(id),
    q ( qq ),
    m_error ( QNetworkReply::NoError )
{
    streamKey_ = ApiRequest::instance()->streamKey(id, token);
    connect( streamKey_.data(), SIGNAL(finished()), this, SLOT(streamKeyRetrieved()));
}

QString DownloaderPrivate::errorString() const
{
    return m_errorString;
}

void DownloaderPrivate::error ( QNetworkReply::NetworkError error )
{
    this->m_error = error;
    emit q->requestError ( error );
}

void DownloaderPrivate::stopDownload()
{
    reply_->abort();
}

void DownloaderPrivate::streamKeyRetrieved()
{
    QString key = streamKey_->streamKey();
    QString ip = streamKey_->ip();

    //FIXME: if streamkey is valid:
    aborted_ = false;
    connect(this, SIGNAL(finished(QNetworkReply*)), this, SLOT(onFinished(QNetworkReply*)));

    file_ = new QFile;
    file_->setFileName(m_path + QDir::separator() + m_fileName + ".mp3");

    file_->open(QIODevice::WriteOnly);

    QUrl postData;
    postData.addQueryItem(QLatin1String("streamKey"), streamKey_->streamKey());

    // common headers
    mainRequest_.setRawHeader(QByteArray("User-Agent"), Config::instance()->userAgent());
    mainRequest_.setRawHeader(QByteArray("Referer"), QString("http://%1/JSQueue.swf?%2").arg(Config::instance()->host()).arg(MapBuilder::jsqueue().at(1)).toAscii());
    mainRequest_.setUrl(QUrl(QString("http://%1/stream.php").arg(streamKey_->ip())));
    mainRequest_.setHeader(QNetworkRequest::ContentTypeHeader, QLatin1String("application/x-www-form-urlencoded"));
    reply_ = post(mainRequest_, postData.encodedQuery());
    connect(reply_, SIGNAL(downloadProgress(qint64,qint64)), this, SLOT(onDownloadProgress(qint64,qint64)));
    connect(reply_, SIGNAL(readyRead()), this, SLOT(onReadyRead()));
    connect(reply_, SIGNAL(finished()), this, SLOT(onReplyFinished()));
}

/*!
  \brief onFinished: manage end of download
  \return void
*/
void DownloaderPrivate::onFinished(QNetworkReply *)
{
    if(!reply_->error() == QNetworkReply::NoError) { // if error occurred
        qDebug() << "GrooveOff ::" << "Reply Error ::" << reply_->errorString() << "for" << m_fileName;

        emit q->downloadCompleted(false);
        if(file_->isOpen()) {
            file_->close();
            file_->remove();
            file_->deleteLater();
        }
        return;
    }

    if(file_->isOpen()) {
        file_->close();
        file_->deleteLater();
    }

    emit q->downloadCompleted(true);
    qDebug() << "GrooveOff ::" << "Finished download of" << m_fileName;
}

/*!
  \brief onReadyRead: write on disk
  \return void
*/
void DownloaderPrivate::onReadyRead()
{
    file_->write(reply_->readAll());
}

/*!
  \brief onReplyFinished
  \return void
*/
void DownloaderPrivate::onReplyFinished()
{
    if(file_->isOpen()) {
        file_->close();
        file_->deleteLater();
    }
}

void DownloaderPrivate::onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
    emit q->progress(bytesReceived, bytesTotal);
}

Downloader::Downloader ( QString path, QString fileName, uint id, QString token, QObject* parent ) : QNetworkAccessManager ( parent ), d ( new DownloaderPrivate ( this, path, fileName, id, token ) )
{

}

Downloader::~Downloader()
{
    delete d;
}

QString Downloader::errorString() const
{
    return d->errorString();
}

void Downloader::stopDownload()
{
    return d->stopDownload();
}

#include "downloader_p.moc"
#include "downloader.moc"

