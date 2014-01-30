#include "downloader_p.h"
#include "apirequest.h"
#include "config.h"
#include "mapbuilder.h"

#include <QFile>
#include <QDir>
#include <QDebug>

#if QT_VERSION >= QT_VERSION_CHECK( 5, 0, 0 )
#include <QUrlQuery>
#endif

using namespace GrooveShark;

DownloaderPrivate::DownloaderPrivate ( Downloader* qq, QString path, QString fileName, uint id, QString token, QObject* parent ) :
    QNetworkAccessManager ( parent ),
    m_path(path),
    m_fileName(fileName),
    m_id(id),
    reply_(0),
    q ( qq ),
    m_error ( QNetworkReply::NoError )
{
    streamKey_ = ApiRequest::instance()->streamKey(id, token);

    connect( streamKey_.data(), SIGNAL(finished()),
             this, SLOT(streamKeyFinished()));

    connect( streamKey_.data(), SIGNAL(parseError()),
             this, SLOT(streamKeyError()));

    connect( streamKey_.data(), SIGNAL(requestError(QNetworkReply::NetworkError)),
             this, SLOT(streamKeyError()));
}

QString DownloaderPrivate::errorString() const
{
    return m_errorString;
}

void DownloaderPrivate::error ( QNetworkReply::NetworkError error )
{
    this->m_error = error;
    m_errorString = reply_->errorString();
    emit q->requestError ( error );
}

void DownloaderPrivate::stopDownload()
{
    // stopping a download we can be in two states:
    // 1. we are still retrieving the stream key
    // 2. we are downloading the file
    if(reply_) { // we are downloading the file
        reply_->abort();
    } else { // we are still retrieving the stream key
        streamKey_->abort();
        emit q->downloadCompleted(false);
    }
}

void DownloaderPrivate::streamKeyFinished()
{
    QString key = streamKey_->streamKey();
    QString ip = streamKey_->ip();

    connect(this, SIGNAL(finished(QNetworkReply*)),
            this, SLOT(onFinished(QNetworkReply*)));

    file_ = new QFile;
    file_->setFileName(m_path + QDir::separator() + m_fileName);

    file_->open(QIODevice::WriteOnly);

#if QT_VERSION >= QT_VERSION_CHECK( 5, 0, 0 )
    QUrlQuery postData;
#else
    QUrl postData;
#endif

    postData.addQueryItem(QLatin1String("streamKey"), streamKey_->streamKey());

    // common headers
    mainRequest_.setRawHeader(QByteArray("User-Agent"), Config::instance()->userAgent());
    mainRequest_.setRawHeader(QByteArray("Referer"), QString("http://%1/JSQueue.swf?%2").arg(Config::instance()->host()).arg(MapBuilder::jsqueue().at(1)).toLatin1());
    mainRequest_.setUrl(QUrl(QString("http://%1/stream.php").arg(streamKey_->ip())));
    mainRequest_.setHeader(QNetworkRequest::ContentTypeHeader, QLatin1String("application/x-www-form-urlencoded"));

#if QT_VERSION >= QT_VERSION_CHECK( 5, 0, 0 )
    reply_ = post(mainRequest_, postData.query(QUrl::EncodeUnicode).toLatin1());
#else
    reply_ = post(mainRequest_, postData.encodedQuery());
#endif

    connect(reply_, SIGNAL(downloadProgress(qint64,qint64)),
            this, SLOT(onDownloadProgress(qint64,qint64)));

    connect(reply_, SIGNAL(readyRead()),
            this, SLOT(onReadyRead()));

    connect(reply_, SIGNAL(finished()),
            this, SLOT(onReplyFinished()));
}

void DownloaderPrivate::streamKeyError()
{
    m_errorString = streamKey_->errorString();
    emit q->downloadCompleted(false);
}

/*!
  \brief onFinished: manage end of download
  \return void
*/
void DownloaderPrivate::onFinished(QNetworkReply *)
{
    if(!reply_->error() == QNetworkReply::NoError) { // if error occurred
        m_errorString = reply_->errorString();

        if(file_->isOpen()) {
            file_->close();
            file_->remove();
            file_->deleteLater();
        }

        emit q->downloadCompleted(false);

        return;
    }

    if(file_->isOpen()) {
        file_->close();
        file_->deleteLater();
    }

    emit q->downloadCompleted(true);
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
