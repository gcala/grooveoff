#ifndef STREAMKEY_P_H
#define STREAMKEY_P_H

#include "streamkey.h"

namespace GrooveShark
{

class StreamKeyPrivate : public QObject
{
    Q_OBJECT

public:
    StreamKeyPrivate ( StreamKey* qq, QNetworkReply* reply, QObject* parent = 0 );
    StreamKeyPrivate ( StreamKey* qq, const QVariant& variant, QObject* parent = 0 );

    QString streamKey() const;
    QString ip() const;
    uint id() const;

private:
    QNetworkReply* m_reply;
    StreamKey* const q;

    QString m_streamKey;
    QString m_ip;
    uint m_id;

    QNetworkReply::NetworkError m_error;
    bool parse ( const QVariant& data );
    bool parse ( const QByteArray& data );

private slots:
    void parseData();
    void error ( QNetworkReply::NetworkError error );
};

}

#endif // STREAMKEY_P_H
