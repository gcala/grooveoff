/*
    GrooveOff - Offline Grooveshark.com music
    Copyright (C) 2013  Giuseppe Calà <jiveaxe@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/


#include "utility.h"

#include <QFontMetrics>
#include <QUuid>
#include <QCryptographicHash>
#include <QDir>

const int Utility::coverSize  = 40; // possible values are 40,50,70,80,90,120
const int Utility::buttonSize = 24;
const int Utility::marginSize = 5;
const QByteArray Utility::userAgent("Mozilla/5.0 (Windows NT 6.1; WOW64) AppleWebKit/536.5 (KHTML, like Gecko) Chrome/19.0.1084.56 Safari/536.5");
const QLatin1String Utility::host("grooveshark.com");
const QByteArray Utility::secret = Utility::randomHex(32).toAscii();
const QByteArray Utility::uuid = QUuid::createUuid().toString().remove('{').remove('}').toAscii();
const QString Utility::coversCachePath = QDir::tempPath() + QDir::separator() +
                                         QLatin1String("grooveoff_cache") + QDir::separator();

QString Utility::elidedText(const QString& text,
                            const Qt::TextElideMode& elideMode,
                            const int& width,
                            const QFont::Weight &weight)
{
    QFont systemFont;
    systemFont.setWeight(weight);
    QFontMetrics fm(systemFont);

    return fm.elidedText(text, elideMode, width);
}

QFont Utility::font(const QFont::Weight& weight, const int &delta)
{
    QFont systemFont;
    systemFont.setPointSize(systemFont.pointSize() + delta);
    systemFont.setWeight(weight);

    return systemFont;
}

QFont Utility::monoFont(const QFont::Weight& weight, const int &delta)
{
    QFont systemFont;
    systemFont.setStyleHint(QFont::Monospace);
    systemFont.setPointSize(systemFont.pointSize() + delta);
    systemFont.setWeight(weight);

    return systemFont;
}

/*!
  \brief randomHex: generate a session unique random hex
  \param length : length of the random string
  \return random hex as string
*/
QString Utility::randomHex(const int &length)
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
QVariantMap Utility::header()
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
    header.insert(QLatin1String("session"), Utility::secret);
    header.insert(QLatin1String("uuid"), uuid);
    header.insert(QLatin1String("privacy"), 0);

    return header;
}

QStringList Utility::htmlclient()
{
    QStringList list;
    list << QLatin1String("htmlshark") << QLatin1String("20130520") << QLatin1String("nuggetsOfBaller");
    return list;
}

QStringList Utility::jsqueue()
{
    QStringList list;
    list << QLatin1String("jsqueue") << QLatin1String("20130520") << QLatin1String("nuggetsOfBaller");
    return list;
}

QVariantMap Utility::getTokenMap()
{
    QVariantMap parameters;
    parameters.insert(QLatin1String("secretKey"), QCryptographicHash::hash(Utility::header().value(QLatin1String("session")).toByteArray(), QCryptographicHash::Md5).toHex());

    QVariantMap h = Utility::header();
    h.insert(QLatin1String("clientRevision"), Utility::htmlclient().at(1));
    h.insert(QLatin1String("client"), Utility::htmlclient().at(0));

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
    QString rnd = Utility::randomHex(6);
    return rnd + QCryptographicHash::hash(QString("%1:%2:%3:%4").arg(method).arg(token).arg(secret).arg(rnd).toAscii(), QCryptographicHash::Sha1).toHex();
}

QVariantMap Utility::searchMap(const QString &query, const QString &what, const QString &token)
{
    QVariantMap parameters;
    parameters.insert(QLatin1String("type"), what);
    parameters.insert(QLatin1String("query"), query);

    QVariantMap h = Utility::header();
    h.insert(QLatin1String("clientRevision"), Utility::htmlclient().at(1));
    h.insert(QLatin1String("client"), Utility::htmlclient().at(0));
    h.insert(QLatin1String("token"), prepToken(QLatin1String("getResultsFromSearch"), Utility::htmlclient().at(2), token)); // 'Searching for the Song' method

    QVariantMap p;
    p.insert(QLatin1String("parameters"), parameters);
    p.insert(QLatin1String("method"), QLatin1String("getResultsFromSearch"));
    p.insert(QLatin1String("header"), h);

    return p;
}

QVariantMap Utility::downloadMap(const QString &id, const QString &token)
{
    QVariantMap parameters;
    parameters.insert(QLatin1String("type"), 8);
    parameters.insert(QLatin1String("mobile"), false);
    parameters.insert(QLatin1String("prefetch"), false);
    parameters.insert(QLatin1String("songIDs"), id);
    parameters.insert(QLatin1String("country"), Utility::header()[QLatin1String("country")]);

    QVariantMap h = Utility::header();
    h.insert(QLatin1String("clientRevision"), Utility::jsqueue().at(1));
    h.insert(QLatin1String("client"), Utility::jsqueue().at(0));
    h.insert(QLatin1String("token"), prepToken(QLatin1String("getStreamKeysFromSongIDs"), Utility::jsqueue().at(2), token));

    QVariantMap p;
    p.insert(QLatin1String("parameters"), parameters);
    p.insert(QLatin1String("method"), QLatin1String("getStreamKeysFromSongIDs"));
    p.insert(QLatin1String("header"), h);

    return p;
}


