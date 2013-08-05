#ifndef APIREQUEST_P_H
#define APIREQUEST_P_H

#include "apirequest.h"

#include "requesthandler.h"

namespace GrooveShark
{

class ApiRequestPrivate
{
public:
    //Constructors
    ApiRequestPrivate( QNetworkAccessManager* nam );
    //Member Functions
    SongListPtr songs(QString match, QString token );
    SongPtr song( uint id );
    TokenPtr token();
    StreamKeyPtr streamKey(uint id, QString token);

private:
    RequestHandler m_requestHandler;
    QByteArray m_secret;
    QByteArray m_uuid;

    QString randomHex(const int &length);
    QVariantMap header();
    QStringList htmlclient();
    QStringList jsqueue();
    QVariantMap getTokenMap();
    QVariantMap searchMap(const QString &query, const QString &token);
    QVariantMap downloadMap(const QString &id, const QString &token);
    QString prepToken(const QString &method, const QString &secret, const QString &token);
};

}

#endif // APIREQUEST_P_H
