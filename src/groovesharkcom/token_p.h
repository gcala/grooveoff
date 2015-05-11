#ifndef TOKEN_P_H
#define TOKEN_P_H

#include "token.h"

namespace GroovesharkCom
{

class TokenPrivate : public QObject
{
    Q_OBJECT

public:
    TokenPrivate ( Token* qq, QNetworkReply* reply, QObject* parent = 0 );
    TokenPrivate ( Token* qq, const QVariant& variant, QObject* parent = 0 );

    QString content() const;
    bool isEmpty() const;
    bool prefetchEnabled() const;
    QString serviceVersion() const;
    QString session() const;
    QString errorString() const;

private:
    QNetworkReply* m_reply;
    Token* const q;

    QString m_content;
    bool m_prefetchEnabled;
    QString m_serviceVersion;
    QString m_session;
    QString m_errorString;

    QNetworkReply::NetworkError m_error;
    bool parse ( const QVariant& data );
    bool parse ( const QByteArray& data );

private slots:
    void parseData();
    void error ( QNetworkReply::NetworkError error );
};

}

#endif // TOKEN_P_H
