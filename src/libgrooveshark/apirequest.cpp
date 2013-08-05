#include "apirequest.h"

#include "apirequest_p.h"

#include "urlbuilder.h"

#include <QString>
#include <QByteArray>
#include <QLatin1String>
#include <QUuid>
#include <QCryptographicHash>
#include <QStringList>
#include <qjson/serializer.h>

using namespace GrooveShark;

ApiRequestPrivate::ApiRequestPrivate( QNetworkAccessManager* nam ) : m_requestHandler( nam )
{
    m_secret = randomHex(32).toAscii();
    m_uuid = QUuid::createUuid().toString().remove('{').remove('}').toAscii();
}

SongListPtr ApiRequestPrivate::songs(QString match, QString token )
{
    QVariantMap map = searchMap(match, token);
    QJson::Serializer serializer;
    QByteArray json = serializer.serialize(map);

    QString requestUrl = UrlBuilder::getSearchUrl() + map.value(QLatin1String("method")).toString();

    QNetworkReply *reply;
    reply = m_requestHandler.postRequest( requestUrl, json );

    SongListPtr songList( new SongList( reply ) );
    return songList;
}

TokenPtr ApiRequestPrivate::token()
{
    QJson::Serializer serializer;
    QByteArray json = serializer.serialize(getTokenMap());

    QString requestUrl = UrlBuilder::getTokenUrl();

    QNetworkReply *reply;
    reply = m_requestHandler.postRequest( requestUrl, json );

    TokenPtr token( new Token( reply ) );
    return token;
}

StreamKeyPtr ApiRequestPrivate::streamKey(uint id, QString token)
{
    QVariantMap map = downloadMap(QString::number(id), token);
    QJson::Serializer serializer;
    QByteArray json = serializer.serialize(map);

    QString requestUrl = UrlBuilder::getSongStreamKeyUrl() + map.value("method").toByteArray();
    QString refererUrl = UrlBuilder::getRefererUrl() + jsqueue().at(1);

    QNetworkReply *reply;
    reply = m_requestHandler.postFileRequest( requestUrl, refererUrl, json );

    StreamKeyPtr streamKey( new StreamKey( reply ) );
    return streamKey;
}

ApiRequest::ApiRequest( QNetworkAccessManager* nam ) : d( new ApiRequestPrivate( nam ) )
{
}

/*!
  \brief randomHex: generate a session unique random hex
  \param length : length of the random string
  \return random hex as string
*/
QString ApiRequestPrivate::randomHex(const int &length)
{
    QString randomHex;

    for(int i = 0; i < length; i++) {
        int n = qrand() % 16;
        randomHex.append(QString::number(n,16));
    }

    return randomHex;
}

/*!
  \brief setHeader: setup http headers
  \return void
*/
QVariantMap ApiRequestPrivate::header()
{
    QVariantMap country;
    country.insert(QLatin1String("CC1"), (qlonglong)72057594037927940);
    country.insert(QLatin1String("CC2"), 0);
    country.insert(QLatin1String("CC3"), 0);
    country.insert(QLatin1String("CC4"), 0);
    country.insert(QLatin1String("ID"), 57);
    country.insert(QLatin1String("IPR"), 0);

    QVariantMap header;
    header.insert(QLatin1String("country"), country);
    header.insert(QLatin1String("session"), m_secret);
    header.insert(QLatin1String("uuid"), m_uuid);
    header.insert(QLatin1String("privacy"), 0);

    return header;
}

QStringList ApiRequestPrivate::htmlclient()
{
    QStringList list;
    list << QLatin1String("htmlshark") << QLatin1String("20130520") << QLatin1String("nuggetsOfBaller");
    return list;
}

QStringList ApiRequestPrivate::jsqueue()
{
    QStringList list;
    list << QLatin1String("jsqueue") << QLatin1String("20130520") << QLatin1String("nuggetsOfBaller");
    return list;
}

QVariantMap ApiRequestPrivate::getTokenMap()
{
    QVariantMap parameters;
    parameters.insert(QLatin1String("secretKey"), QCryptographicHash::hash(header().value(QLatin1String("session")).toByteArray(), QCryptographicHash::Md5).toHex());

    QVariantMap h = header();
    h.insert(QLatin1String("clientRevision"), htmlclient().at(1));
    h.insert(QLatin1String("client"), htmlclient().at(0));

    QVariantMap p;
    p.insert(QLatin1String("parameters"), parameters);
    p.insert(QLatin1String("method"), QLatin1String("getCommunicationToken"));
    p.insert(QLatin1String("header"), h);

    return p;
}

/*!
  \brief prepToken : prepare token used in request
  \param method : one of the methods accepted by api
  \param secret : a secret key
  \return an elaborated string
*/
QString ApiRequestPrivate::prepToken(const QString &method, const QString &secret, const QString &token)
{
    QString rnd = randomHex(6);
    return rnd + QCryptographicHash::hash(QString("%1:%2:%3:%4").arg(method).arg(token).arg(secret).arg(rnd).toAscii(), QCryptographicHash::Sha1).toHex();
}

QVariantMap ApiRequestPrivate::searchMap(const QString &query, const QString &token)
{
    QVariantMap parameters;
    parameters.insert(QLatin1String("type"), "Songs");
    parameters.insert(QLatin1String("query"), query);

    QVariantMap h = header();
    h.insert(QLatin1String("clientRevision"), htmlclient().at(1));
    h.insert(QLatin1String("client"), htmlclient().at(0));
    h.insert(QLatin1String("token"), prepToken(QLatin1String("getResultsFromSearch"), htmlclient().at(2), token)); // 'Searching for the Song' method

    QVariantMap p;
    p.insert(QLatin1String("parameters"), parameters);
    p.insert(QLatin1String("method"), QLatin1String("getResultsFromSearch"));
    p.insert(QLatin1String("header"), h);

    return p;
}

QVariantMap ApiRequestPrivate::downloadMap(const QString &id, const QString &token)
{
    QVariantMap parameters;
    parameters.insert(QLatin1String("type"), 8);
    parameters.insert(QLatin1String("mobile"), false);
    parameters.insert(QLatin1String("prefetch"), false);
    parameters.insert(QLatin1String("songIDs"), id);
    parameters.insert(QLatin1String("country"), header()[QLatin1String("country")]);

    QVariantMap h = header();
    h.insert(QLatin1String("clientRevision"), jsqueue().at(1));
    h.insert(QLatin1String("client"), jsqueue().at(0));
    h.insert(QLatin1String("token"), prepToken(QLatin1String("getStreamKeysFromSongIDs"), jsqueue().at(2), token));

    QVariantMap p;
    p.insert(QLatin1String("parameters"), parameters);
    p.insert(QLatin1String("method"), QLatin1String("getStreamKeysFromSongIDs"));
    p.insert(QLatin1String("header"), h);

    return p;
}

ApiRequest::~ApiRequest()
{
    delete d;
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

#include "apirequest.moc"
