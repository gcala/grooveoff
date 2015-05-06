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

DownloaderPrivate::DownloaderPrivate ( Downloader* qq, QString path, QString fileName, uint id, QString token, QObject* parent )
    : QNetworkAccessManager ( parent )
    , m_path(path)
    , m_fileName(fileName)
    , m_id(id)
    , m_reply(0)
    , q ( qq )
    , m_error ( QNetworkReply::NoError )
{
    m_streamKey = ApiRequest::instance()->streamKey(id, token);

    connect( m_streamKey.data(), SIGNAL(finished()), SLOT(streamKeyFinished()) );
    connect( m_streamKey.data(), SIGNAL(parseError()), SLOT(streamKeyError()) );
    connect( m_streamKey.data(), SIGNAL(requestError(QNetworkReply::NetworkError)), SLOT(streamKeyError()) );
}

QString DownloaderPrivate::errorString() const
{
    return m_errorString;
}

void DownloaderPrivate::error ( QNetworkReply::NetworkError error )
{
    m_error = error;
    m_errorString = m_reply->errorString();
    emit q->requestError ( error );
}

void DownloaderPrivate::stopDownload()
{
    // stopping a download we can be in two states:
    // 1. we are still retrieving the stream key
    // 2. we are downloading the file
    if(m_reply) { // we are downloading the file
        m_reply->abort();
    } else { // we are still retrieving the stream key
        m_streamKey->abort();
        emit q->downloadCompleted(false);
    }
}

void DownloaderPrivate::streamKeyFinished()
{
    QString key = m_streamKey->content();
    QString ip = m_streamKey->ip();

    connect( this, SIGNAL(finished(QNetworkReply*)), SLOT(onFinished(QNetworkReply*)) );

    m_file = new QFile;
    m_file->setFileName(m_path + QDir::separator() + m_fileName);
    
    if( !QDir().exists(m_path) ) {
        if( !QDir().mkpath( m_path ) ) {
            qDebug() << "Cannot create" << m_path << "path";
            emit q->downloadCompleted(false);
            return;
        }
    }

    if(!m_file->open(QIODevice::WriteOnly)) {
        qDebug() << m_file->fileName() << m_file->errorString();
        emit q->downloadCompleted(false);
        return;
    }

#if QT_VERSION >= QT_VERSION_CHECK( 5, 0, 0 )
    QUrlQuery postData;
#else
    QUrl postData;
#endif

    postData.addQueryItem( QLatin1String( "streamKey" ), m_streamKey->content() );

    // common headers
    m_mainRequest.setRawHeader( QByteArray( "User-Agent" ), Config::instance()->userAgent() );
    m_mainRequest.setRawHeader( QByteArray( "Referer" ), QString( "http://%1/JSQueue.swf?%2" ).arg( Config::instance()->host() ).arg( MapBuilder::jsqueue().at(1) ).toLatin1() );
    m_mainRequest.setUrl( QUrl( QString( "http://%1/stream.php" ).arg( m_streamKey->ip() ) ) );
    m_mainRequest.setHeader( QNetworkRequest::ContentTypeHeader, QLatin1String( "application/x-www-form-urlencoded" ) );

#if QT_VERSION >= QT_VERSION_CHECK( 5, 0, 0 )
    m_reply = post( m_mainRequest, postData.query( QUrl::EncodeUnicode ).toLatin1() );
#else
    m_reply = post( m_mainRequest, postData.encodedQuery() );
#endif

    connect( m_reply, SIGNAL(downloadProgress(qint64,qint64)), SLOT(onDownloadProgress(qint64,qint64)) );
    connect( m_reply, SIGNAL(readyRead()), SLOT(onReadyRead()) );
    connect( m_reply, SIGNAL(finished()), SLOT(onReplyFinished()) );
}

void DownloaderPrivate::streamKeyError()
{
    m_errorString = m_streamKey->errorString();
    emit q->downloadCompleted(false);
}

/*!
  \brief onFinished: manage end of download
  \return void
*/
void DownloaderPrivate::onFinished( QNetworkReply * )
{
    if( !m_reply->error() == QNetworkReply::NoError ) { // if error occurred
        m_errorString = m_reply->errorString();

        if( m_file->isOpen() ) {
            m_file->close();
            m_file->remove();
            m_file->deleteLater();
        }

        emit q->downloadCompleted( false );

        return;
    }

    if( m_file->isOpen() ) {
        m_file->close();
        m_file->deleteLater();
    }

    emit q->downloadCompleted( true );
}

/*!
  \brief onReadyRead: write on disk
  \return void
*/
void DownloaderPrivate::onReadyRead()
{
    m_file->write( m_reply->readAll() );
}

/*!
  \brief onReplyFinished
  \return void
*/
void DownloaderPrivate::onReplyFinished()
{
    if( m_file->isOpen() ) {
        m_file->close();
        m_file->deleteLater();
    }
}

void DownloaderPrivate::onDownloadProgress( qint64 bytesReceived, qint64 bytesTotal )
{
    emit q->progress( bytesReceived, bytesTotal );
}

Downloader::Downloader ( QString path, QString fileName, uint id, QString token, QObject* parent )
    : QNetworkAccessManager ( parent )
    , d ( new DownloaderPrivate ( this, path, fileName, id, token ) )
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
