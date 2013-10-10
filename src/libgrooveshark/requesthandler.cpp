#include "requesthandler.h"
#include "config.h"

#include <QAuthenticator>
#include <QCoreApplication>

using namespace GrooveShark;

RequestHandler::RequestHandler( QNetworkAccessManager* nam ) : m_nam( nam )
{
}

RequestHandler::~RequestHandler()
{
    m_nam->deleteLater();
}

QNetworkReply* RequestHandler::getRequest( const QString& url )
{
    QUrl reqUrl( url );
    QNetworkRequest request( reqUrl );
    addUserAgent( request );
    QNetworkReply* reply = m_nam->get( request );
    return reply;
}


QNetworkReply* RequestHandler::postRequest( const QString& url, const QByteArray data )
{
    QNetworkRequest request( url );
    addUserAgent( request );
    addContentType( request );
    QNetworkReply* reply = m_nam->post( request, data );
    return reply;
}

QNetworkReply* RequestHandler::postFileRequest( const QString& url, const QString& referer, const QByteArray data )
{
    QNetworkRequest request( url );
    addUserAgent( request );
    addReferer( request, referer );
    addContentType( request );
    QNetworkReply* reply = m_nam->post( request, data );
    return reply;
}

void RequestHandler::addUserAgent( QNetworkRequest &request )
{
    request.setRawHeader("User-Agent", Config::instance()->userAgent() );
}

void RequestHandler::addContentType( QNetworkRequest &request )
{
    request.setHeader(QNetworkRequest::ContentTypeHeader, Config::instance()->contentType().toAscii() );
}

void RequestHandler::addReferer( QNetworkRequest &request, const QString &referer )
{
    request.setRawHeader("Referer", referer.toAscii() );
}

#include "requesthandler.moc"
