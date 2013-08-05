#ifndef SONG_P_H
#define SONG_P_H

#include "song.h"

namespace GrooveShark
{

class SongPrivate : public QObject
{
    Q_OBJECT

public:
    SongPrivate ( Song* qq, QNetworkReply* reply, QObject* parent = 0 );
    SongPrivate ( Song* qq, const QVariant& variant, QObject* parent = 0 );

    uint albumID() const;
    QString albumName() const;
    QString artistCoverArtFilename() const;
    uint artistID() const;
    QString artistName() const;
    QString avgDuration() const;
    QString avgRating() const;
    QString coverArtFilename() const;
    QString estimateDuration() const;
    qulonglong flags() const;
    uint genreID() const;
    bool isLowBitrateAvailable() const;
    bool isVerified() const;
    qulonglong popularity() const;
    qulonglong popularityIndex() const;
    qulonglong rawScore() const;
    double score() const;
    uint songID() const;
    QString songName() const;
    QString tsAdded() const;
    uint trackNum() const;
    uint year() const;
    QString errorString() const;

private:
    QNetworkReply* m_reply;
    Song* const q;

    uint m_albumID;
    QString m_albumName;
    QString m_artistCoverArtFilename;
    uint m_artistID;
    QString m_artistName;
    QString m_avgDuration;
    QString m_avgRating;
    QString m_coverArtFilename;
    QString m_estimateDuration;
    qulonglong m_flags;
    uint m_genreID;
    bool m_isLowBitrateAvailable;
    bool m_isVerified;
    qulonglong m_popularity;
    qulonglong m_popularityIndex;
    qulonglong m_rawScore;
    double m_score;
    uint m_songID;
    QString m_songName;
    QString m_tsAdded;
    uint m_trackNum;
    uint m_year;
    QString m_errorString;

    QNetworkReply::NetworkError m_error;
    bool parse ( const QVariant& data );
    bool parse ( const QByteArray& data );

private slots:
    void parseData();
    void error ( QNetworkReply::NetworkError error );
};

}

#endif // SONG_P_H
