#ifndef TOKEN_H
#define TOKEN_H

#include <QSharedPointer>
#include <QVariant>
#include <QNetworkReply>

namespace GroovesharkCom
{

class TokenPrivate;

class Token : public QObject
{
    Q_OBJECT
    Q_PROPERTY( QString content READ content CONSTANT )
    Q_PROPERTY( bool prefetchEnabled READ prefetchEnabled CONSTANT )
    Q_PROPERTY( QString serviceVersion READ serviceVersion CONSTANT )
    Q_PROPERTY( QString session READ session CONSTANT )
    Q_PROPERTY( QString errorString READ errorString CONSTANT )

public:
    Token( QNetworkReply* reply, QObject* parent = 0 );
    Token( const QVariant& variant, QObject* parent = 0 );
    virtual ~Token();

    QString content() const;
    bool isEmpty() const;
    bool prefetchEnabled() const;
    QString serviceVersion() const;
    QString session() const;
    QString errorString() const;

private:
    Q_DISABLE_COPY( Token )
    TokenPrivate* const d;
    friend class TokenPrivate;

signals:
    /**Gets emitted when the data is ready to read*/
    void finished();
    /**Gets emitted when a parse error ocurred*/
    void parseError();
    /**Gets emitted when a request error ocurred*/
    void requestError( QNetworkReply::NetworkError error );
};

typedef QSharedPointer<Token> TokenPtr;

}

Q_DECLARE_METATYPE( GroovesharkCom::TokenPtr )

#endif // TOKEN_H
