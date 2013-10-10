#ifndef DOWNLOADER_P_H
#define DOWNLOADER_P_H

#include "downloader.h"
#include "streamkey.h"

class QFile;

namespace GrooveShark
{

class DownloaderPrivate : public QNetworkAccessManager
{
    Q_OBJECT

public:
    DownloaderPrivate ( Downloader* qq, QString path, QString fileName, uint id, QString token, QObject* parent = 0 );

//    QString result() const;
//    bool prefetchEnabled() const;
//    QString serviceVersion() const;
//    QString session() const;
    QString errorString() const;

    void stopDownload();

private:
//    QNetworkReply* m_reply;
    Downloader* const q;

//    QString m_result;
//    bool m_prefetchEnabled;
//    QString m_serviceVersion;
//    QString m_session;
    QString m_errorString;

    QString m_path;
    QString m_fileName;
    uint m_id;

    QNetworkReply::NetworkError m_error;

    //---------------------
    StreamKeyPtr streamKey_;
    QNetworkReply *reply_;
    QFile *file_;
    QNetworkRequest mainRequest_;
    bool aborted_;

private slots:
    void error ( QNetworkReply::NetworkError error );
    void streamKeyRetrieved();
    void onDownloadProgress(qint64,qint64);
    void onFinished(QNetworkReply*);
    void onReadyRead();
    void onReplyFinished();
};

}

#endif // DOWNLOADER_P_H

