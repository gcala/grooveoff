/*
    GrooveOff - Offline Grooveshark.com music
    Copyright (C) 2013-2014  Giuseppe Calà <jiveaxe@gmail.com>

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


#ifndef COVERDOWNLOADER_H
#define COVERDOWNLOADER_H

#include <QNetworkAccessManager>
#include <QPixmap>

class CoverDownloader : public QObject
{
    Q_OBJECT
public:
    explicit CoverDownloader(QString name, QObject *parent = 0);
    ~CoverDownloader();

    bool isSuccess() { return success_; }
    QString coverName() { return coverName_; }

signals:
    void done();

public slots:
    void downloadFinished();

private:
    QString coverName_;
    bool success_;
    QNetworkAccessManager qnam_;
    QNetworkReply *reply_;
};

#endif // COVERDOWNLOADER_H
