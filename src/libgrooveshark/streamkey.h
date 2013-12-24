#ifndef STREAMKEY_H
#define STREAMKEY_H

#include <QSharedPointer>
#include <QVariant>
#include <QNetworkReply>

namespace GrooveShark
{

class StreamKeyPrivate;

class StreamKey : public QObject
{
    Q_OBJECT
    Q_PROPERTY( QString streamKey READ streamKey CONSTANT )
    Q_PROPERTY( QString ip READ ip CONSTANT )
    Q_PROPERTY( uint id READ id CONSTANT )

public:
    StreamKey( uint id, QString token, QObject* parent = 0 );
    virtual ~StreamKey();

    QString errorString() const;
    QString streamKey() const;
    QString ip() const;
    uint id() const;
    void abort();

private:
    Q_DISABLE_COPY( StreamKey )
    StreamKeyPrivate* const d;
    friend class StreamKeyPrivate;

signals:
    /**Gets emitted when the data is ready to read*/
    void finished();
    /**Gets emitted when an parse error ocurred*/
    void parseError();
    /**Gets emitted when an request error ocurred*/
    void requestError( QNetworkReply::NetworkError error );
};

typedef QSharedPointer<StreamKey> StreamKeyPtr;

}

Q_DECLARE_METATYPE( GrooveShark::StreamKeyPtr )

#endif // STREAMKEY_H
