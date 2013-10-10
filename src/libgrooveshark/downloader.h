#ifndef DOWNLOADER_H
#define DOWNLOADER_H

#include <QSharedPointer>
#include <QNetworkReply>

namespace GrooveShark
{

class DownloaderPrivate;

class Downloader : public QNetworkAccessManager
{
    Q_OBJECT
//    Q_PROPERTY( QString result READ result CONSTANT )
//    Q_PROPERTY( bool prefetchEnabled READ prefetchEnabled CONSTANT )
//    Q_PROPERTY( QString serviceVersion READ serviceVersion CONSTANT )
//    Q_PROPERTY( QString session READ session CONSTANT )
    Q_PROPERTY( QString errorString READ errorString CONSTANT )

public:
    Downloader( QString path, QString fileName, uint id, QString token, QObject* parent = 0 );
    virtual ~Downloader();

//    QString result() const;
//    bool prefetchEnabled() const;
//    QString serviceVersion() const;
//    QString session() const;
    QString errorString() const;

    void stopDownload();

private:
    Q_DISABLE_COPY( Downloader )
    DownloaderPrivate* const d;
    friend class DownloaderPrivate;

signals:
    /**Gets emitted when the data is ready to read*/
    void finished();
    /**Gets emitted when an parse error ocurred*/
    void parseError();
    /**Gets emitted when an request error ocurred*/
    void requestError( QNetworkReply::NetworkError error );

    void downloadCompleted(bool);
    void progress(const qint64 &bytesReceived, const qint64 &bytesTotal);
};

typedef QSharedPointer<Downloader> DownloaderPtr;

}

Q_DECLARE_METATYPE( GrooveShark::DownloaderPtr )

#endif // DOWNLOADER_H
