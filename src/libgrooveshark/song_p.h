#ifndef SONG_P_H
#define SONG_P_H

#include "song.h"

namespace GrooveShark
{

class SongPrivate : public QObject
{
    Q_OBJECT

public:
    SongPrivate ( Song* qq, const QVariant& variant, bool fromPlaylist = false, QObject* parent = 0 );
    SongPrivate ( Song* qq, bool fromPlaylist = false, QObject* parent = 0 );

    quint32 albumID() const;
    void setAlbumID(quint32);
    QString albumName() const;
    void setAlbumName(QString);
    QString artistCoverArtFilename() const;
    void setArtistCoverArtFilename(QString);
    quint32 artistID() const;
    void setArtistID(quint32);
    QString artistName() const;
    void setArtistName(QString);
    QString avgDuration() const;
    void setAvgDuration(QString);
    QString avgRating() const;
    void setAvgRating(QString);
    QString coverArtFilename() const;
    void setCoverArtFilename(QString);
    QString estimateDuration() const;
    void setEstimateDuration(QString);
    quint64 flags() const;
    void setFlags(quint64);
    quint32 genreID() const;
    void setGenreID(quint32);
    bool isLowBitrateAvailable() const;
    void setIsLowBitrateAvailable(bool);
    bool isVerified() const;
    void setIsVerified(bool);
    quint64 popularity() const;
    void setPopularity(quint64);
    quint64 popularityIndex() const;
    void setPopularityIndex(quint64);
    quint64 rawScore() const;
    void setRawScore(quint64);
    double score() const;
    void setScore(double);
    quint32 songID() const;
    void setSongID(quint32);
    QString songName() const;
    void setSongName(QString);
    QString tsAdded() const;
    void setTsAdded(QString);
    quint32 trackNum() const;
    void setTrackNum(quint32);
    quint32 year() const;
    void setYear(quint32);

    QString errorString() const;

private:
    Song* const q;

    quint32 m_albumID;
    QString m_albumName;
    QString m_artistCoverArtFilename;
    quint32 m_artistID;
    QString m_artistName;
    QString m_avgDuration;
    QString m_avgRating;
    QString m_coverArtFilename;
    QString m_estimateDuration;
    quint64 m_flags;
    quint32 m_genreID;
    bool m_isLowBitrateAvailable;
    bool m_isVerified;
    quint64 m_popularity;
    quint64 m_popularityIndex;
    quint64 m_rawScore;
    double m_score;
    quint32 m_songID;
    QString m_songName;
    QString m_tsAdded;
    quint32 m_trackNum;
    quint32 m_year;

    QString m_errorString;

    bool parse ( const QVariant& data );
    bool parse ( const QByteArray& data );
    bool m_fromPlaylist;
};

}

#endif // SONG_P_H
