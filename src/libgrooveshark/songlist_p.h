#ifndef SONGLIST_P_H
#define SONGLIST_P_H

#include "songlist.h"

namespace GrooveShark
{

class SongListPrivate : public QObject
{
    Q_OBJECT

public:
    SongListPrivate( SongList* qq, QNetworkReply* reply );
    virtual ~SongListPrivate();
    QList<SongPtr> list() const;
    QVariant songs() const;
    QString errorString() const;

private:
    SongList* const q;
    QNetworkReply* m_reply;
    QVariant m_songs;
    QString m_errorString;

    QNetworkReply::NetworkError m_error;

    bool parse( const QVariant& data );
    bool parse( const QByteArray& data );

private slots:
    void parseData();
    void error( QNetworkReply::NetworkError error );
};

}

#endif // SONGLIST_P_H
