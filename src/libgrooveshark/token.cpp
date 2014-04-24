#include "token_p.h"

#include <QDebug>

#if QT_VERSION >= QT_VERSION_CHECK( 5, 0, 0 )
#include <QJsonParseError>
#include <QJsonDocument>
#else
#include <qjson/parser.h>
#endif

using namespace GrooveShark;

TokenPrivate::TokenPrivate ( Token* qq, QNetworkReply* reply, QObject* parent ) :
    QObject ( parent ),
    m_reply ( reply ),
    q ( qq ),
    m_error ( QNetworkReply::NoError )
{
    QObject::connect ( m_reply, SIGNAL ( finished() ),
                       this, SLOT ( parseData() ) );
    QObject::connect ( m_reply, SIGNAL ( error ( QNetworkReply::NetworkError ) ),
                       this, SLOT ( error ( QNetworkReply::NetworkError ) ) );
}

TokenPrivate::TokenPrivate ( Token* qq, const QVariant& variant, QObject* parent ) :
    QObject ( parent ),
    m_reply ( 0 ),
    q ( qq )
{
    parse ( variant );
}

QString TokenPrivate::result() const
{
    return m_result;
}

bool TokenPrivate::prefetchEnabled() const
{
    return m_prefetchEnabled;
}

QString TokenPrivate::serviceVersion() const
{
    return m_serviceVersion;
}

QString TokenPrivate::session() const
{
    return m_session;
}

QString TokenPrivate::errorString() const
{
    return m_errorString;
}

bool TokenPrivate::parse( const QVariant& data )
{
    if( !data.canConvert( QVariant::Map ) ) {
        m_errorString = "  received data is not valid";
        return false;
    }

    QVariantMap tokenMap = data.toMap();

    if(tokenMap.keys().contains("fault")) {
        m_errorString = tokenMap.value("fault").toMap().value("message").toString();
        return false;
    }

    QVariant v = tokenMap.value( QLatin1String( "result" ) );
    if( !v.canConvert( QVariant::String ) )
        qDebug() << "'result' field is invalid";
    m_result = v.toString();

    QVariantMap headerMap = tokenMap.value(QLatin1String( "header" )).toMap();

    v = headerMap.value( QLatin1String( "prefetchEnabled" ) );
    if( !v.canConvert( QVariant::Bool ) )
        qDebug() << "'prefetchEnabled' field is invalid";
    m_prefetchEnabled = v.toBool();

    v = headerMap.value( QLatin1String( "serviceVersion" ) );
    if( !v.canConvert( QVariant::String ) )
        qDebug() << "'serviceVersion' field is invalid";
    m_serviceVersion = v.toString();

    v = headerMap.value( QLatin1String( "session" ) );
    if( !v.canConvert( QVariant::String ) )
        qDebug() << "'session' field is invalid";
    m_session = v.toString();

    return true;
}

bool TokenPrivate::parse ( const QByteArray& data )
{
    bool ok;
    QVariant variant;

#if QT_VERSION >= QT_VERSION_CHECK( 5, 0, 0 )
    QJsonParseError *err = new QJsonParseError();
    QJsonDocument doc = QJsonDocument::fromJson(data, err);
    if (err->error != 0) {
        m_errorString = err->errorString();
        ok = false;
    } else {
        variant = doc.toVariant();
        ok = true;
    }
#else
    QJson::Parser parser;
    variant = parser.parse ( data, &ok );
    if(!ok)
        m_errorString = parser.errorString();
#endif

    if( ok )
    {
        ok = ( parse( variant ) );
    }
    return ok;
}

void TokenPrivate::parseData()
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

void TokenPrivate::error ( QNetworkReply::NetworkError error )
{
    this->m_error = error;
    m_errorString = m_reply->errorString();
    emit q->requestError ( error );
}

Token::Token ( QNetworkReply* reply, QObject* parent ) :
    QObject ( parent ),
    d ( new TokenPrivate ( this, reply ) )
{

}

Token::Token ( const QVariant& variant, QObject* parent ) :
    QObject ( parent ),
    d ( new TokenPrivate ( this, variant ) )
{

}

Token::~Token()
{
    delete d;
}

QString Token::result() const
{
    return d->result();
}

bool Token::prefetchEnabled() const
{
    return d->prefetchEnabled();
}

QString Token::serviceVersion() const
{
    return d->serviceVersion();
}

QString Token::session() const
{
    return d->session();
}

QString Token::errorString() const
{
    return d->errorString();
}

