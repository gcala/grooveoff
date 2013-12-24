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
public:
    Downloader( QString path, QString fileName, uint id, QString token, QObject* parent = 0 );
    virtual ~Downloader();

    QString errorString() const;
    void stopDownload();

private:
    Q_DISABLE_COPY( Downloader )
    DownloaderPrivate* const d;
    friend class DownloaderPrivate;

signals:
    /**Gets emitted when the data is ready to read*/
    void finished();
    /**Gets emitted when a parse error ocurred*/
    void parseError();
    /**Gets emitted when a request error ocurred*/
    void requestError( QNetworkReply::NetworkError error );

    void downloadCompleted(bool);
    void progress(const qint64 &bytesReceived, const qint64 &bytesTotal);
};

typedef QSharedPointer<Downloader> DownloaderPtr;

}

Q_DECLARE_METATYPE( GrooveShark::DownloaderPtr )

#endif // DOWNLOADER_H
