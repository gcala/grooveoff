#ifndef REQUESTHANDLER_H
#define REQUESTHANDLER_H

#include <QNetworkAccessManager>
#include <QNetworkReply>

namespace GroovesharkCom
{

class RequestHandler
{

public:

    /**
     * @param username The username that should be used for authentication if required.
     * @param password The password that should be used for authentication if required
     */
    RequestHandler( QNetworkAccessManager* nam );

    virtual ~RequestHandler();

    /**
     * Sends a GET request with the given url and receives the servers response.
     * @param response The servers response will be written into this QByteArray
     * @param url The request url (without http://) as QString
     * @return 0 if the request was successful, corresponding ErrorCode if unsuccessful
     */
    QNetworkReply* getRequest( const QString& url );

    /**
     * Sends a POST request with the given url and data, adds auth Data and receives the servers response
     * @param data The data to send to the url
     * @param url The request url (without http://) as QString
     * @return 0 if the request was successful, corresponding ErrorCode if unsuccessful
     */
    QNetworkReply* postRequest( const QString& url, const QByteArray data );

    QNetworkReply* postFileRequest( const QString& url, const QString& referer, const QByteArray data );

private:
    QNetworkAccessManager* m_nam;

    void addUserAgent( QNetworkRequest& request );
    void addContentType( QNetworkRequest& request );
    void addReferer( QNetworkRequest& request, const QString& referer );
};
}

#endif // REQUESTHANDLER_H
