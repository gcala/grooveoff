#include "streamkey_p.h"
#include <qjson/parser.h>
#include <QDebug>

using namespace GrooveShark;

StreamKeyPrivate::StreamKeyPrivate ( StreamKey* qq, QNetworkReply* reply, QObject* parent ) :
    QObject ( parent ),
    m_reply ( reply ),
    q ( qq ),
    m_error ( QNetworkReply::NoError )
{
    QObject::connect ( m_reply, SIGNAL ( finished() ), this, SLOT ( parseData() ) );
    QObject::connect ( m_reply, SIGNAL ( error ( QNetworkReply::NetworkError ) ), this, SLOT ( error ( QNetworkReply::NetworkError ) ) );
}

StreamKeyPrivate::StreamKeyPrivate ( StreamKey* qq, const QVariant& variant, QObject* parent ) : QObject ( parent ), m_reply ( 0 ), q ( qq )
{
    parse ( variant );
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

bool StreamKeyPrivate::parse( const QVariant& data )
{
    if( !data.canConvert( QVariant::Map ) )
        return false;

    QVariantMap streamKeyMap = data.toMap();

    if(!streamKeyMap.keys().contains("result"))
        return false;

    QVariantMap resultMap = streamKeyMap.value("result").toMap();

    if(resultMap.keys().count() == 0)
        return false;

    if(resultMap.value(resultMap.keys().at(0)).toMap().value("streamKey").toString().isEmpty())
        return false;

    if(resultMap.value(resultMap.keys().at(0)).toMap().value("ip").toString().isEmpty())
        return false;

    m_ip  = resultMap.value(resultMap.keys().at(0)).toMap().value("ip").toString();
    m_streamKey = resultMap.value(resultMap.keys().at(0)).toMap().value("streamKey").toString();
    m_id  = resultMap.value(resultMap.keys().at(0)).toMap().value("SongID").toUInt();

    return true;
}

bool StreamKeyPrivate::parse ( const QByteArray& data )
{
    QJson::Parser parser;
    bool ok;
    QVariant variant = parser.parse ( data, &ok );
    if ( ok )
    {
        if ( !parse ( variant ) ) return false;
        return true;
    }
    else
    {
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
}

void StreamKeyPrivate::error ( QNetworkReply::NetworkError error )
{
    this->m_error = error;
    emit q->requestError ( error );
}

StreamKey::StreamKey ( QNetworkReply* reply, QObject* parent ) : QObject ( parent ), d ( new StreamKeyPrivate ( this, reply ) )
{

}

StreamKey::StreamKey ( const QVariant& variant, QObject* parent ) : QObject ( parent ), d ( new StreamKeyPrivate ( this, variant ) )
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

#include "streamkey_p.moc"
#include "streamkey.moc"
