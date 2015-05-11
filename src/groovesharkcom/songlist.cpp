#include "songlist_p.h"

#include <QDebug>

#if QT_VERSION >= QT_VERSION_CHECK( 5, 0, 0 )
#include <QJsonParseError>
#include <QJsonDocument>
#else
#include <qjson/parser.h>
#endif

using namespace GroovesharkCom;

SongListPrivate::SongListPrivate( SongList* qq, QNetworkReply* reply ) :
    q( qq ),
    m_reply( reply ),
    m_songs( QVariant() ),
    m_error( QNetworkReply::NoError )
{
    QObject::connect( m_reply, SIGNAL(finished()), this, SLOT(parseData()) );
    QObject::connect( m_reply, SIGNAL(error(QNetworkReply::NetworkError)),
                      this, SLOT(error(QNetworkReply::NetworkError)) );
}

SongListPrivate::~SongListPrivate()
{
}

int SongListPrivate::count() const
{
    return m_songs.toList().count();
}

SongPtr SongListPrivate::item ( int row )
{
    return m_songs.toList().at( row ).value<SongPtr>();
}

QVariant SongListPrivate::songs() const
{
    return m_songs;
}

QString SongListPrivate::errorString() const
{
    return m_errorString;
}

bool SongListPrivate::parse( const QVariant& data )
{
    if(!data.toMap()[QLatin1String("result")].toMap().keys().contains("result")
        && !data.toMap()[QLatin1String("result")].toMap().keys().contains("Songs")
    ) {
        m_errorString = "received data is not valid";
        return false;
    }

    bool isPlaylist = data.toMap()[QLatin1String("result")].toMap().keys().contains("Songs");

    QVariantList varList;
    if(isPlaylist)
        varList = data.toMap()[QLatin1String("result")].toMap()[QLatin1String("Songs")].toList();
    else
        varList = data.toMap()[QLatin1String("result")].toMap()[QLatin1String("result")].toList();

    QVariantList songList;
    foreach( QVariant var, varList )
    {
        QVariant v;
        v.setValue<GroovesharkCom::SongPtr>( SongPtr( new Song( var, isPlaylist ) ) );
        songList.append( v );
    }
    m_songs = QVariant( songList );
    return true;
}

bool SongListPrivate::parse( const QByteArray& data )
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


void SongListPrivate::parseData()
{
    if( m_reply->error() == QNetworkReply::NoError )
    {
        if( parse( m_reply->readAll() ) )
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

void SongListPrivate::error( QNetworkReply::NetworkError error )
{
    this->m_error = error;
    m_errorString = m_reply->errorString();
    emit q->requestError( error );
}

SongList::SongList( QNetworkReply* reply, QObject* parent ) :
    QObject( parent ),
    d( new SongListPrivate( this, reply ) )
{

}

SongList::~SongList()
{
    delete d;
}

QVariant SongList::songs() const
{
    return d->songs();
}

QString SongList::errorString() const
{
    return d->errorString();
}

int SongList::count() const
{
    return d->count();
}

SongPtr SongList::item ( int row )
{
    return d->item( row );
}

