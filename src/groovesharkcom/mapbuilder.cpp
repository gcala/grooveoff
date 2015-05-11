#include "mapbuilder.h"
#include "config.h"

#include <QStringList>
#include <QUuid>
#include <QCryptographicHash>

using namespace GroovesharkCom;

/*!
  \brief randomHex: generate a session unique random hex
  \param length : length of the random string
  \return random hex as string
*/
QString randomHex(const int &length)
{
    QString randomHex;

    for(int i = 0; i < length; i++) {
        int n = qrand() % 16;
        randomHex.append(QString::number(n,16));
    }

    return randomHex;
}

const QByteArray MapBuilder::s_secret = randomHex(32).toLatin1();
const QByteArray MapBuilder::s_uuid = QUuid::createUuid().toString().remove('{').remove('}').toLatin1();

/*!
  \brief setHeader: setup http headers
  \return void
*/
QVariantMap MapBuilder::header()
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
    header.insert(QLatin1String("session"), MapBuilder::s_secret);
    header.insert(QLatin1String("uuid"), MapBuilder::s_uuid);
    header.insert(QLatin1String("privacy"), 0);

    return header;
}

QStringList htmlclient()
{
    QStringList list;
    list << QLatin1String("htmlshark") << QLatin1String("20130520") << QLatin1String("nuggetsOfBaller");
    return list;
}

QStringList MapBuilder::jsqueue()
{
    QStringList list;
    list << QLatin1String("jsqueue") << QLatin1String("20130520") << QLatin1String("nuggetsOfBaller");
    return list;
}

QVariantMap MapBuilder::getTokenMap()
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
QString prepToken(const QString &method, const QString &secret, const QString &token)
{
    QString rnd = randomHex(6);
    return rnd + QCryptographicHash::hash(QString("%1:%2:%3:%4").arg(method).arg(token).arg(secret).arg(rnd).toLatin1(), QCryptographicHash::Sha1).toHex();
}

QVariantMap MapBuilder::getSearchMap(const QString &query, const QString &token)
{
    bool isPlaylist;
    int playlistId = query.toInt(&isPlaylist, 10);

    QVariantMap parameters;
    if(isPlaylist) {
        parameters.insert(QLatin1String("playlistID"), playlistId);
    } else {
        parameters.insert(QLatin1String("type"), "Songs");
        parameters.insert(QLatin1String("query"), query);
    }

    QVariantMap h = header();
    h.insert(QLatin1String("clientRevision"), htmlclient().at(1));
    h.insert(QLatin1String("client"), htmlclient().at(0));
    h.insert(QLatin1String("token"), prepToken(QLatin1String("getResultsFromSearch"), htmlclient().at(2), token)); // 'Searching for the Song' method

    QVariantMap p;
    p.insert(QLatin1String("parameters"), parameters);
    if(isPlaylist)
        p.insert(QLatin1String("method"), QLatin1String("playlistGetSongs"));
    else
        p.insert(QLatin1String("method"), QLatin1String("getResultsFromSearch"));
    p.insert(QLatin1String("header"), h);

    return p;
}

QVariantMap MapBuilder::getDownloadMap(const QString &id, const QString &token)
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


