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

class Song : public QObject
{
    Q_OBJECT

public:
    explicit Song ( const QString &title = QString(),
                    const QString &album = QString(),
                    const QString &artist = QString(),
                    const QString &year = QString(),
                    const QString &id = QString(),
                    const QString &coverName = QString());
    ~Song();

    void setTitle(const QString &title)     {title_ = title;}
    void setAlbum(const QString &album)     {album_ = album;}
    void setArtist(const QString &artist)   {artist_ = artist;}
    void setYear(const QString &year)       {year_ = year;}
    void setId(const QString &id)           {id_ = id;}
    void setCoverName(const QString &cover) {coverName_ = cover;}

    QString title()     const { return title_; }
    QString album()     const { return album_; }
    QString artist()    const { return artist_; }
    QString year()      const { return year_; }
    QString id()        const { return id_; }
    QString coverName() const { return coverName_; }

    void requireCoverReload();
    void requireDownloadIconReload();

signals:
    void reloadCover();
    void reloadIcon();

public slots:

private:
    QString title_;
    QString album_;
    QString artist_;
    QString year_;
    QString id_;
    QString coverName_;
};

#endif // SONG_H
