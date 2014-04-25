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


#ifndef SONG_H
#define SONG_H

#include <QObject>
#include <QPainter>

class QNetworkAccessManager;
class QNetworkReply;

class Song : public QObject
{
    Q_OBJECT

public:
    explicit Song ( QObject* parent = 0 );
    ~Song();

    void setTitle(const QString &title) {m_title = title;}
    void setAlbum(const QString &album) {m_album = album;}
    void setArtist(const QString &artist) {m_artist = artist;}
    void setYear(const QString &year) {m_year = year;}
    void setId(const QString &id) {m_id = id;}
    void setCoverName(const QString &cover);

    QString title()       const { return m_title; }
    QString album()       const { return m_album; }
    QString artist()      const { return m_artist; }
    QString year()        const { return m_year; }
    QString id()          const { return m_id; }
    QString coverName()   const { return m_coverName; }
    QPixmap coverPixmap() const { return m_coverPixmap; }

signals:
    void trigRepaint();

public slots:
    void onFinished(QNetworkReply*);

private:
    QString m_title;
    QString m_album;
    QString m_artist;
    QString m_year;
    QString m_id;
    QString m_coverName;
    QPixmap m_coverPixmap;
    QNetworkAccessManager *m_qnam;
    QNetworkReply *m_reply;
};

#endif // SONG_H
