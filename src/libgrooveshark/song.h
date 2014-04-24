#ifndef SONG_H
#define SONG_H

#include <QSharedPointer>
#include <QVariant>
#include <QNetworkReply>

namespace GrooveShark
{

class SongPrivate;

class Song : public QObject
{
    Q_OBJECT
    Q_PROPERTY( QString albumName READ albumName WRITE setAlbumName )
    Q_PROPERTY( QString songName READ songName WRITE setSongName )
    Q_PROPERTY( QString artistName READ artistName WRITE setArtistName )
    Q_PROPERTY( QString coverArtFilename READ coverArtFilename WRITE setCoverArtFilename )
    Q_PROPERTY( QString artistCoverArtFilename READ artistCoverArtFilename WRITE setArtistCoverArtFilename )
    Q_PROPERTY( quint32 albumID READ albumID WRITE setAlbumID )
    Q_PROPERTY( quint32 songID READ songID WRITE setSongID )
    Q_PROPERTY( quint32 artistID READ artistID WRITE setArtistID )
    Q_PROPERTY( quint32 genreID READ genreID WRITE setGenreID )
    Q_PROPERTY( QString avgDuration READ avgDuration WRITE setAvgDuration )
    Q_PROPERTY( QString avgRating READ avgRating WRITE setAvgRating )
    Q_PROPERTY( QString estimateDuration READ estimateDuration WRITE setEstimateDuration )
    Q_PROPERTY( quint64 flags READ flags WRITE setFlags )
    Q_PROPERTY( bool isLowBitrateAvailable READ isLowBitrateAvailable WRITE setIsLowBitrateAvailable )
    Q_PROPERTY( bool isVerified READ isVerified WRITE setIsVerified )
    Q_PROPERTY( quint64 popularity READ popularity WRITE setPopularity )
    Q_PROPERTY( quint64 popularityIndex READ popularityIndex WRITE setPopularityIndex )
    Q_PROPERTY( quint64 rawScore READ rawScore WRITE setRawScore )
    Q_PROPERTY( double score READ score WRITE setScore )
    Q_PROPERTY( QString tsAdded READ tsAdded WRITE setTsAdded )
    Q_PROPERTY( quint32 trackNum READ trackNum WRITE setTrackNum )
    Q_PROPERTY( quint32 year READ year WRITE setYear )

public:
    Song( const QVariant& variant, bool fromPlaylist, QObject* parent = 0 );
    Song( bool fromPlaylist = false, QObject* parent = 0 );
    Song( const Song & song, QObject *parent = 0);
    virtual ~Song();

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
    Q_DISABLE_COPY( Song )
    SongPrivate* const d;
    friend class SongPrivate;
};

typedef QSharedPointer<Song> SongPtr;

}

QDataStream& operator<<( QDataStream& dataStream, const GrooveShark::SongPtr song );
QDataStream& operator>>( QDataStream& dataStream, GrooveShark::SongPtr song ); // deprecated: throw( UserException )

Q_DECLARE_METATYPE( GrooveShark::SongPtr )

#endif // SONG_H
