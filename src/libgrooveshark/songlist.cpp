#include "songlist_p.h"

#include <QDebug>

#include <QJsonParseError>
#include <QJsonDocument>

#include <utility>

using namespace GrooveShark;

SongListPrivate::SongListPrivate( SongList* qq, QNetworkReply* reply ) :
    q( qq ),
    m_reply( reply ),
    m_songs( QVariant() ),
    m_error( QNetworkReply::NoError )
{
    QObject::connect( m_reply, &QNetworkReply::finished, this, &SongListPrivate::parseData );
    QObject::connect( m_reply, &QNetworkReply::errorOccurred, this, &SongListPrivate::error );
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
    for( const QVariant &var : std::as_const(varList) )
    {
        QVariant v;
        v.setValue<GrooveShark::SongPtr>( SongPtr( new Song( var, isPlaylist ) ) );
        songList.append( v );
    }
    m_songs = QVariant( songList );
    return true;
}

bool SongListPrivate::parse( const QByteArray& data )
{
    bool ok;
    QVariant variant;

    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(data, &err);
    if (err.error != QJsonParseError::NoError) {
        m_errorString = err.errorString();
        ok = false;
    } else {
        variant = doc.toVariant();
        ok = true;
    }

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

