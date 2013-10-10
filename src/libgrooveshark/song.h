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
    Q_PROPERTY( QString albumName READ albumName CONSTANT )
    Q_PROPERTY( QString songName READ songName CONSTANT )
    Q_PROPERTY( QString artistName READ artistName CONSTANT )
    Q_PROPERTY( QString coverArtFilename READ coverArtFilename CONSTANT )
    Q_PROPERTY( QString artistCoverArtFilename READ artistCoverArtFilename CONSTANT )
    Q_PROPERTY( uint albumID READ albumID CONSTANT )
    Q_PROPERTY( uint songID READ songID CONSTANT )
    Q_PROPERTY( uint artistID READ artistID CONSTANT )
    Q_PROPERTY( uint genreID READ genreID CONSTANT )
    Q_PROPERTY( QString avgDuration READ avgDuration CONSTANT )
    Q_PROPERTY( QString avgRating READ avgRating CONSTANT )
    Q_PROPERTY( QString estimateDuration READ estimateDuration CONSTANT )
    Q_PROPERTY( qulonglong flags READ flags CONSTANT )
    Q_PROPERTY( bool isLowBitrateAvailable READ isLowBitrateAvailable CONSTANT )
    Q_PROPERTY( bool isVerified READ isVerified CONSTANT )
    Q_PROPERTY( qulonglong popularity READ popularity CONSTANT )
    Q_PROPERTY( qulonglong popularityIndex READ popularityIndex CONSTANT )
    Q_PROPERTY( qulonglong rawScore READ rawScore CONSTANT )
    Q_PROPERTY( double score READ score CONSTANT )
    Q_PROPERTY( QString tsAdded READ tsAdded CONSTANT )
    Q_PROPERTY( uint trackNum READ trackNum CONSTANT )
    Q_PROPERTY( uint year READ year CONSTANT )
    Q_PROPERTY( QString errorString READ errorString CONSTANT )

public:
    Song( QNetworkReply* reply, QObject* parent = 0 );
    Song( const QVariant& variant, QObject* parent = 0 );
    virtual ~Song();

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
    Q_DISABLE_COPY( Song )
    SongPrivate* const d;
    friend class SongPrivate;

signals:
    /**Gets emitted when the data is ready to read*/
    void finished();
    /**Gets emitted when an parse error ocurred*/
    void parseError();
    /**Gets emitted when an request error ocurred*/
    void requestError( QNetworkReply::NetworkError error );
};

typedef QSharedPointer<Song> SongPtr;

}

Q_DECLARE_METATYPE( GrooveShark::SongPtr )

#endif // SONG_H
