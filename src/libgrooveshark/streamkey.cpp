#include "streamkey_p.h"
#include "mapbuilder.h"
#include "urlbuilder.h"
#include "config.h"

#if QT_VERSION < QT_VERSION_CHECK( 5, 0, 0 )
#include <qjson/parser.h>
#include <qjson/serializer.h>
#else
#include <QJsonParseError>
#include <QJsonDocument>
#endif

#include <QStringList>
#include <QDebug>

using namespace GrooveShark;

StreamKeyPrivate::StreamKeyPrivate ( StreamKey* qq, uint id, QString token, QObject* parent ) :
    QObject ( parent ),
    m_id(id),
    q ( qq ),
    m_error ( QNetworkReply::NoError )
{
    QVariantMap map = MapBuilder::getDownloadMap(QString::number(id), token);
    QByteArray json;

#if QT_VERSION < QT_VERSION_CHECK( 5, 0, 0 )
    QJson::Serializer serializer;
    json = serializer.serialize(map);
#else
    QJsonDocument doc = QJsonDocument::fromVariant(map);
    json = doc.toJson();
#endif

    QString requestUrl = UrlBuilder::getSongStreamKeyUrl() + map.value("method").toByteArray();
    QString refererUrl = UrlBuilder::getRefererUrl() + MapBuilder::jsqueue().at(1);

    m_nam = new QNetworkAccessManager(this);

    //FIXME:duplicated code from requesthandler.cpp
    QNetworkRequest request( requestUrl );
    request.setRawHeader("User-Agent", Config::instance()->userAgent() );
    request.setRawHeader("Referer", refererUrl.toLatin1() );
    request.setHeader(QNetworkRequest::ContentTypeHeader, Config::instance()->contentType().toLatin1() );

    m_reply = m_nam->post( request, json );

    QObject::connect ( m_reply, SIGNAL ( finished() ), this, SLOT ( parseData() ) );
    QObject::connect ( m_reply, SIGNAL ( error ( QNetworkReply::NetworkError ) ), this, SLOT ( error ( QNetworkReply::NetworkError ) ) );
}

QString StreamKeyPrivate::streamKey() const
{
    return m_streamKey;
}

QString StreamKeyPrivate::ip() const
{
    return m_ip;
}

uint StreamKeyPrivate::id() const
{
    return m_id;
}

void StreamKeyPrivate::abort()
{
    if(m_reply->isRunning()) {
        m_reply->abort();
    }
}

bool StreamKeyPrivate::parse( const QVariant& data )
{
    if( !data.canConvert( QVariant::Map ) ) {
        qDebug() << "Can't convert streamkey data to map";
        return false;
    }

    QVariantMap streamKeyMap = data.toMap();

    if(!streamKeyMap.keys().contains("result")) {
        qDebug() << "No 'result' field found";
        return false;
    }

    QVariantMap resultMap = streamKeyMap.value("result").toMap();

    if(resultMap.keys().count() == 0) {
        qDebug() << "No keys found";
        return false;
    }

    if(resultMap.value(resultMap.keys().at(0)).toMap().value("streamKey").toString().isEmpty()) {
        qDebug() << "No 'streamKey' field found";
        return false;
    }

    if(resultMap.value(resultMap.keys().at(0)).toMap().value("ip").toString().isEmpty()) {
        qDebug() << "No 'ip' field found";
        return false;
    }

    m_ip  = resultMap.value(resultMap.keys().at(0)).toMap().value("ip").toString();
    m_streamKey = resultMap.value(resultMap.keys().at(0)).toMap().value("streamKey").toString();
    m_id  = resultMap.value(resultMap.keys().at(0)).toMap().value("SongID").toUInt();

    return true;
}

bool StreamKeyPrivate::parse ( const QByteArray& data )
{
    bool ok;
    QVariant variant;

#if QT_VERSION < QT_VERSION_CHECK( 5, 0, 0 )
    QJson::Parser parser;
    variant = parser.parse ( data, &ok );
#else
    QJsonParseError *err = new QJsonParseError();
    QJsonDocument doc = QJsonDocument::fromJson(data, err);
    if (err->error != 0) {
        qDebug() << err->errorString();
        ok = false;
    } else {
        variant = doc.toVariant();
        ok = true;
    }
#endif

    if ( ok )
    {
        if ( !parse ( variant ) ) {
            qDebug() << data;
            return false;
        }
        return true;
    }
    else
    {
        qDebug() << "Error converting data to json";
        qDebug() << data;
        return false;
    }
}

void StreamKeyPrivate::parseData()
{
    //parse and send signal
    if ( m_reply->error() == QNetworkReply::NoError )
    {
        if ( parse ( m_reply->readAll() ) )
        {
            emit q->finished();
        }
        else
        {
            emit q->parseError();
        }
    }
    m_reply->deleteLater();
    m_nam->deleteLater();
}

void StreamKeyPrivate::error ( QNetworkReply::NetworkError error )
{
    this->m_error = error;
    emit q->requestError ( error );
}

StreamKey::StreamKey ( uint id, QString token, QObject* parent ) : QObject ( parent ), d ( new StreamKeyPrivate ( this, id, token ) )
{

}

StreamKey::~StreamKey()
{
    delete d;
}

QString StreamKey::streamKey() const
{
    return d->streamKey();
}

QString StreamKey::ip() const
{
    return d->ip();
}

uint StreamKey::id() const
{
    return d->id();
}

void StreamKey::abort()
{
    return d->abort();
}

