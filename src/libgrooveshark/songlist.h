#ifndef SONGLIST_H
#define SONGLIST_H

#include "song.h"

#include <QNetworkReply>
#include <QSharedPointer>
#include <QList>

namespace GrooveShark
{

class SongListPrivate;

class SongList : public QObject
{
    Q_OBJECT
    Q_PROPERTY( QVariant songs READ songs CONSTANT )
public:
    SongList( QNetworkReply* reply, QObject* parent = 0 );
    virtual ~SongList();
    int count() const;
    SongPtr item ( int row );
    QVariant songs() const;
    QString errorString() const;

private:
    Q_DISABLE_COPY( SongList )
    SongListPrivate* const d;
    friend class SongListPrivate;

signals:
    /**Gets emitted when the data is ready to read*/
    void finished();
    /**Gets emitted when a parse error ocurred*/
    void parseError();
    /**Gets emitted when a request error ocurred*/
    void requestError( QNetworkReply::NetworkError error );
};

typedef QSharedPointer<SongList> SongListPtr;

}

Q_DECLARE_METATYPE( GrooveShark::SongListPtr )

#endif // SONGLIST_H
