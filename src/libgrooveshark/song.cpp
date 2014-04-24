#include "song_p.h"

#if QT_VERSION >= QT_VERSION_CHECK( 5, 0, 0 )
#include <QJsonParseError>
#include <QJsonDocument>
#else
#include <qjson/parser.h>
#endif

#include <QDebug>
#include <QMetaProperty>

using namespace GrooveShark;

SongPrivate::SongPrivate ( Song* qq, const QVariant& variant, bool fromPlaylist, QObject* parent ) :
    QObject ( parent ),
    m_fromPlaylist(fromPlaylist),
    q ( qq )
{
    parse ( variant );
}

SongPrivate::SongPrivate ( Song* qq, bool fromPlaylist, QObject* parent ) :
    QObject ( parent ),
    q ( qq ),
    m_fromPlaylist(fromPlaylist)
{
}

quint32 SongPrivate::albumID() const
{
    return m_albumID;
}

void SongPrivate::setAlbumID(quint32 albumID)
{
    m_albumID = albumID;
}

QString SongPrivate::albumName() const
{
    return m_albumName;
}

void SongPrivate::setAlbumName(QString albumName)
{
    m_albumName = albumName;
}

QString SongPrivate::artistCoverArtFilename() const
{
    return m_artistCoverArtFilename;
}

void SongPrivate::setArtistCoverArtFilename(QString artistCoverArtFilename)
{
    m_artistCoverArtFilename = m_artistCoverArtFilename;
}

quint32 SongPrivate::artistID() const
{
    return m_artistID;
}

void SongPrivate::setArtistID(quint32 artistID)
{
    m_artistID = artistID;
}

QString SongPrivate::artistName() const
{
    return m_artistName;
}

void SongPrivate::setArtistName(QString artistName)
{
    m_artistName = artistName;
}

QString SongPrivate::avgDuration() const
{
    return m_avgDuration;
}

void SongPrivate::setAvgDuration(QString avgDuration)
{
    m_avgDuration = avgDuration;
}

QString SongPrivate::avgRating() const
{
    return m_avgRating;
}

void SongPrivate::setAvgRating(QString avgRating)
{
    m_avgRating = avgRating;
}

QString SongPrivate::coverArtFilename() const
{
    return m_coverArtFilename;
}

void SongPrivate::setCoverArtFilename(QString coverArtFilename)
{
    m_coverArtFilename = coverArtFilename;
}

QString SongPrivate::estimateDuration() const
{
    return m_estimateDuration;
}

void SongPrivate::setEstimateDuration(QString estimateDuration)
{
    m_estimateDuration = estimateDuration;
}

quint64 SongPrivate::flags() const
{
    return m_flags;
}

void SongPrivate::setFlags(quint64 flags)
{
    m_flags = flags;
}

quint32 SongPrivate::genreID() const
{
    return m_genreID;
}

void SongPrivate::setGenreID(quint32 genreID)
{
    m_genreID = genreID;
}

bool SongPrivate::isLowBitrateAvailable() const
{
    return m_isLowBitrateAvailable;
}

void SongPrivate::setIsLowBitrateAvailable(bool isLowBitrateAvailable)
{
    m_isLowBitrateAvailable = isLowBitrateAvailable;
}

bool SongPrivate::isVerified() const
{
    return m_isVerified;
}

void SongPrivate::setIsVerified(bool isVerified)
{
    m_isVerified = isVerified;
}

quint64 SongPrivate::popularity() const
{
    return m_popularity;
}

void SongPrivate::setPopularity(quint64 popularity)
{
    m_popularity = popularity;
}

quint64 SongPrivate::popularityIndex() const
{
    return m_popularityIndex;
}

void SongPrivate::setPopularityIndex(quint64 popularityIndex)
{
    m_popularityIndex = popularityIndex;
}

quint64 SongPrivate::rawScore() const
{
    return m_rawScore;
}

void SongPrivate::setRawScore(quint64 rawScore)
{
    m_rawScore = rawScore;
}

double SongPrivate::score() const
{
    return m_score;
}

void SongPrivate::setScore(double score)
{
    m_score = score;
}

quint32 SongPrivate::songID() const
{
    return m_songID;
}

void SongPrivate::setSongID(quint32 songID)
{
    m_songID = songID;
}

QString SongPrivate::songName() const
{
    return m_songName;
}

void SongPrivate::setSongName(QString songName)
{
    m_songName = songName;
}

QString SongPrivate::tsAdded() const
{
    return m_tsAdded;
}

void SongPrivate::setTsAdded(QString tsAdded)
{
    m_tsAdded = tsAdded;
}

quint32 SongPrivate::trackNum() const
{
    return m_trackNum;
}

void SongPrivate::setTrackNum(quint32 trackNum)
{
    m_trackNum = trackNum;
}

quint32 SongPrivate::year() const
{
    return m_year;
}

void SongPrivate::setYear(quint32 year)
{
    m_year = year;
}

QString SongPrivate::errorString() const
{
    return m_errorString;
}

bool SongPrivate::parse( const QVariant& data )
{
    if( !data.canConvert( QVariant::Map ) ){
        m_errorString = "  received data is not valid";
        return false;
    }

    QVariantMap songMap = data.toMap();
    if(songMap.keys().contains("fault")) {
        m_errorString = songMap.value("fault").toMap().value("message").toString();
        return false;
    }

    QVariant v = songMap.value( QLatin1String( "AlbumID" ) );
    if( !v.canConvert( QVariant::UInt ) )
        qDebug() << "'AlbumID' field is invalid";
    m_albumID = v.toUInt();

    v = songMap.value( QLatin1String( "AlbumName" ) );
    if( !v.canConvert( QVariant::String ) )
        qDebug() << "'AlbumName' field is invalid";
    m_albumName = v.toString().replace('/','-');
    // if AlbumName is empty string
    if(m_albumName.trimmed().isEmpty())
        m_albumName = QLatin1String("unknown");

    if(!m_fromPlaylist) {
        v = songMap.value( QLatin1String( "ArtistCoverArtFilename" ) );
        if( !v.canConvert( QVariant::String ) )
            qDebug() << "'ArtistCoverArtFilename' field is invalid";
        m_artistCoverArtFilename = v.toString();
    }

    v = songMap.value( QLatin1String( "ArtistID" ) );
    if( !v.canConvert( QVariant::UInt ) )
        qDebug() << "'ArtistID' field is invalid";
    m_artistID = v.toUInt();

    v = songMap.value( QLatin1String( "ArtistName" ) );
    if( !v.canConvert( QVariant::String ) )
        qDebug() << "'ArtistName' field is invalid";
    m_artistName = v.toString().replace('/','-');

    if(!m_fromPlaylist) {
        v = songMap.value( QLatin1String( "AvgDuration" ) );
        if( !v.canConvert( QVariant::String ) )
            qDebug() << "'AvgDuration' field is invalid";
        m_avgDuration = v.toString();
    }

    v = songMap.value( QLatin1String( "AvgRating" ) );
    if( !v.canConvert( QVariant::String ) )
        qDebug() << "'AvgRating' field is invalid";
    m_avgRating = v.toString();

    v = songMap.value( QLatin1String( "CoverArtFilename" ) );
    if( !v.canConvert( QVariant::String ) )
        qDebug() << "'CoverArtFilename' field is invalid";
    m_coverArtFilename = v.toString();

    v = songMap.value( QLatin1String( "EstimateDuration" ) );
    if( !v.canConvert( QVariant::String ) )
        qDebug() << "'EstimateDuration' field is invalid";
    m_estimateDuration = v.toString();

    v = songMap.value( QLatin1String( "Flags" ) );
    if( !v.canConvert( QVariant::ULongLong ) )
        qDebug() << "'Flags' field is invalid";
    m_flags = v.toULongLong();

    v = songMap.value( QLatin1String( "IsLowBitrateAvailable" ) );
    if( !v.canConvert( QVariant::Bool ) )
        qDebug() << "'IsLowBitrateAvailable' field is invalid";
    m_isLowBitrateAvailable = v.toBool();

    v = songMap.value( QLatin1String( "IsVerified" ) );
    if( !v.canConvert( QVariant::Bool ) )
        qDebug() << "'IsVerified' field is invalid";
    m_isVerified = v.toBool();

    v = songMap.value( QLatin1String( "Popularity" ) );
    if( !v.canConvert( QVariant::ULongLong ) )
        qDebug() << "'Popularity' field is invalid";
    m_popularity = v.toULongLong();

    if(!m_fromPlaylist) {
        v = songMap.value( QLatin1String( "PopularityIndex" ) );
        if( !v.canConvert( QVariant::ULongLong ) )
            qDebug() << "'PopularityIndex' field is invalid";
        m_popularityIndex = v.toULongLong();
    }

    if(!m_fromPlaylist) {
        v = songMap.value( QLatin1String( "RawScore" ) );
        if( !v.canConvert( QVariant::ULongLong ) )
            qDebug() << "'RawScore' field is invalid";
        m_rawScore = v.toULongLong();
    }

    if(!m_fromPlaylist) {
        v = songMap.value( QLatin1String( "Score" ) );
        if( !v.canConvert( QVariant::Double ) )
            qDebug() << "'Score' field is invalid";
        m_score = v.toDouble();
    }

    v = songMap.value( QLatin1String( "SongID" ) );
    if( !v.canConvert( QVariant::UInt ) )
        qDebug() << "'SongID' field is invalid";
    m_songID = v.toUInt();

    v = songMap.value( QLatin1String( m_fromPlaylist ? "Name" : "SongName" ) );
    if( !v.canConvert( QVariant::String ) )
        qDebug() << "'SongName' field is invalid";
    m_songName = v.toString().replace('/','-');

    if(!m_fromPlaylist) {
        v = songMap.value( QLatin1String( "TSAdded" ) );
        if( !v.canConvert( QVariant::String ) )
            qDebug() << "'TSAdded' field is invalid";
        m_tsAdded = v.toString();
    }

    v = songMap.value( QLatin1String( "TrackNum" ) );
    if( !v.canConvert( QVariant::UInt ) )
        qDebug() << "'TrackNum' field is invalid";
    m_trackNum = v.toUInt();

    v = songMap.value( QLatin1String( "Year" ) );
    if( !v.canConvert( QVariant::UInt ) )
        qDebug() << "'Year' field is invalid";
    m_year = v.toUInt();

    return true;
}

bool SongPrivate::parse ( const QByteArray& data )
{
    bool ok;
    QVariant variant;

#if QT_VERSION < QT_VERSION_CHECK( 5, 0, 0 )
    QJson::Parser parser;
    variant = parser.parse ( data, &ok );
#else
    QJsonParseError *err = new QJsonParseError();
    QJsonDocument doc = QJsonDocument::fromJson(data, err);
    if (err->error != 0) {
        qDebug() << err->errorString();
        ok = false;
    } else {
        variant = doc.toVariant();
        ok = true;
    }
#endif

    if( ok )
    {
        ok = ( parse( variant ) );
    }
    return ok;
}

Song::Song ( const QVariant& variant, bool fromPlaylist, QObject* parent ) :
    QObject ( parent ),
    d ( new SongPrivate ( this, variant, fromPlaylist ) )
{

}

Song::Song ( bool fromPlaylist, QObject* parent )
    : QObject ( parent )
    , d ( new SongPrivate ( this, fromPlaylist ) )
{

}

Song::~Song()
{
    delete d;
}

quint32 Song::albumID() const
{
    return d->albumID();
}

void Song::setAlbumID(quint32 albumID)
{
    d->setAlbumID(albumID);
}

QString Song::albumName() const
{
    return d->albumName();
}

void Song::setAlbumName(QString albumName)
{
    d->setAlbumName(albumName);
}

QString Song::artistCoverArtFilename() const
{
    return d->artistCoverArtFilename();
}

void Song::setArtistCoverArtFilename(QString artistCoverArtFilename)
{
    d->setArtistCoverArtFilename(artistCoverArtFilename);
}

quint32 Song::artistID() const
{
    return d->artistID();
}

void Song::setArtistID(quint32 artistID)
{
    d->setArtistID(artistID);
}

QString Song::artistName() const
{
    return d->artistName();
}

void Song::setArtistName(QString artistName)
{
    d->setArtistName(artistName);
}

QString Song::avgDuration() const
{
    return d->avgDuration();
}

void Song::setAvgDuration(QString avgDuration)
{
    d->setAvgDuration(avgDuration);
}

QString Song::avgRating() const
{
    return d->avgRating();
}

void Song::setAvgRating(QString avgRating)
{
    d->setAvgRating(avgRating);
}

QString Song::coverArtFilename() const
{
    return d->coverArtFilename();
}

void Song::setCoverArtFilename(QString coverArtFilename)
{
    d->setCoverArtFilename(coverArtFilename);
}

QString Song::estimateDuration() const
{
    return d->estimateDuration();
}

void Song::setEstimateDuration(QString estimateDuration)
{
    d->setEstimateDuration(estimateDuration);
}

quint64 Song::flags() const
{
    return d->flags();
}

void Song::setFlags(quint64 flags)
{
    d->setFlags(flags);
}

quint32 Song::genreID() const
{
    return d->genreID();
}

void Song::setGenreID(quint32 genreID)
{
    d->setGenreID(genreID);
}

bool Song::isLowBitrateAvailable() const
{
    return d->isLowBitrateAvailable();
}

void Song::setIsLowBitrateAvailable(bool isLowBitrateAvailable)
{
    d->setIsLowBitrateAvailable(isLowBitrateAvailable);
}

bool Song::isVerified() const
{
    return d->isVerified();
}

void Song::setIsVerified(bool isVerified)
{
    d->setIsVerified(isVerified);
}

quint64 Song::popularity() const
{
    return d->popularity();
}

void Song::setPopularity(quint64 popularity)
{
    d->setPopularity(popularity);
}

quint64 Song::popularityIndex() const
{
    return d->popularityIndex();
}

void Song::setPopularityIndex(quint64 popularityIndex)
{
    d->setPopularityIndex(popularityIndex);
}

quint64 Song::rawScore() const
{
    return d->rawScore();
}

void Song::setRawScore(quint64 rawScore)
{
    d->setRawScore(rawScore);
}

double Song::score() const
{
    return d->score();
}

void Song::setScore(double score)
{
    d->setScore(score);
}

quint32 Song::songID() const
{
    return d->songID();
}

void Song::setSongID(quint32 songID)
{
    d->setSongID(songID);
}

QString Song::songName() const
{
    return d->songName();
}

void Song::setSongName(QString songName)
{
    d->setSongName(songName);
}

QString Song::tsAdded() const
{
    return d->tsAdded();
}

void Song::setTsAdded(QString tsAdded)
{
    d->setTsAdded(tsAdded);
}

quint32 Song::trackNum() const
{
    return d->trackNum();
}

void Song::setTrackNum(quint32 trackNum)
{
    d->setTrackNum(trackNum);
}

quint32 Song::year() const
{
    return d->year();
}

void Song::setYear(quint32 year)
{
    d->setYear(year);
}

QString Song::errorString() const
{
    return d->errorString();
}

QDataStream& operator<<( QDataStream& dataStream, const SongPtr song )
{
    for(int i=0; i< song->metaObject()->propertyCount(); ++i) {
        if(song->metaObject()->property(i).isStored(song.data())) {
            dataStream << song->metaObject()->property(i).read(song.data());
        }
    }
    return dataStream;
}

QDataStream & operator>>(QDataStream & dataStream, SongPtr song) {
    QVariant var;
    for(int i = 0; i < song->metaObject()->propertyCount(); ++i) {
        if(song->metaObject()->property(i).isStored(song.data())) {
            dataStream >> var;
            song->metaObject()->property(i).write(song.data(), var);
        }
    }
    return dataStream;
}
