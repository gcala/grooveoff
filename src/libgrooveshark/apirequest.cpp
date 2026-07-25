#include "apirequest.h"

#include "apirequest_p.h"

#include "urlbuilder.h"
#include "mapbuilder.h"

#include <QString>
#include <QByteArray>
#include <QLatin1String>
#include <QCryptographicHash>
#include <QStringList>

#include <QJsonDocument>


using namespace GrooveShark;

ApiRequest* ApiRequest::s_instance = 0;

ApiRequestPrivate::ApiRequestPrivate()
{
    m_nam = new QNetworkAccessManager;
    m_requestHandler = new RequestHandler(m_nam);
}

ApiRequestPrivate::~ApiRequestPrivate()
{
    delete m_requestHandler;
    m_nam->deleteLater();
}

SongListPtr ApiRequestPrivate::songs(QString match, QString token )
{
    QVariantMap map = MapBuilder::getSearchMap(match, token);
    QByteArray json;

    QJsonDocument doc = QJsonDocument::fromVariant(map);
    json = doc.toJson();

    QString requestUrl = UrlBuilder::getSearchUrl() + map.value(QLatin1String("method")).toString();

    QNetworkReply *reply;
    reply = m_requestHandler->postRequest( requestUrl, json );

    SongListPtr songList( new SongList( reply ) );
    return songList;
}

TokenPtr ApiRequestPrivate::token()
{
    QByteArray json;

    QJsonDocument doc = QJsonDocument::fromVariant(MapBuilder::getTokenMap());
    json = doc.toJson();

    QString requestUrl = UrlBuilder::getTokenUrl();

    QNetworkReply *reply;
    reply = m_requestHandler->postRequest( requestUrl, json );

    TokenPtr token( new Token( reply ) );
    return token;
}

StreamKeyPtr ApiRequestPrivate::streamKey(uint id, QString token)
{
    StreamKeyPtr streamKey( new StreamKey( id, token ) );
    return streamKey;
}

DownloaderPtr ApiRequestPrivate::downloadSong(QString path, QString fileName, uint id, QString token)
{
    DownloaderPtr downloader(new Downloader( path, fileName, id, token ));
    return downloader;
}

ApiRequest::ApiRequest() : d( new ApiRequestPrivate() )
{
}

ApiRequest::~ApiRequest()
{
    delete d;
}

ApiRequest* ApiRequest::instance()
{
    if ( !s_instance ) {
        // Qt6 registers the QDataStream operators along with the metatype
        qRegisterMetaType<SongPtr>("SongPtr");
        s_instance = new ApiRequest;
    }

    return s_instance;
}

SongListPtr ApiRequest::songs( QString match, QString token )
{
    return d->songs( match, token );
}

TokenPtr ApiRequest::token()
{
    return d->token();
}

StreamKeyPtr ApiRequest::streamKey(uint id, QString token)
{
    return d->streamKey(id, token);
}

DownloaderPtr ApiRequest::downloadSong(QString path, QString fileName, uint id, QString token)
{
    return d->downloadSong(path, fileName, id, token);
}

