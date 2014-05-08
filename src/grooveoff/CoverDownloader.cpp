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


#include "CoverDownloader.h"
#include "Utility.h"

#include <QNetworkRequest>
#include <QNetworkReply>
#include <QFile>
#include <QDir>

// Cover sizes available: 40 50 70 80 90 120 200 500

CoverDownloader::CoverDownloader(const QString &name, QObject *parent)
    : QObject(parent)
    , m_coverName(name)
{
    QNetworkRequest request;
    request.setUrl( QUrl( QString( "http://images.gs-cdn.net/static/albums/200_%1" ).arg( m_coverName ) ) );
    request.setHeader( QNetworkRequest::ContentTypeHeader, QLatin1String( "application/x-www-form-urlencoded" ) );
    m_reply = m_qnam.get( request );
    connect( m_reply, SIGNAL(finished()), 
                      SLOT(downloadFinished())
           );
}

CoverDownloader::~CoverDownloader()
{
    m_reply->deleteLater();
}


void CoverDownloader::downloadFinished()
{
    switch( m_reply->error() ) {
    case QNetworkReply::NoError: {
        // if no error
        const QByteArray &data = m_reply->readAll();
        m_success = true;
        QFile file( Utility::coversCachePath + m_coverName );
        file.open( QIODevice::WriteOnly );
        file.write( data );
        file.close();
        break;
    }
    default:
        qDebug() << "GrooveOff ::" << "Error downloading cover of" << m_coverName;
        m_success = false;
    }

    emit done();
}

