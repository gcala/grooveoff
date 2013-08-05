#include "song_p.h"
#include <qjson/parser.h>
#include <QDebug>

using namespace GrooveShark;

SongPrivate::SongPrivate ( Song* qq, QNetworkReply* reply, QObject* parent ) :
    QObject ( parent ),
    m_reply ( reply ),
    q ( qq ),
    m_error ( QNetworkReply::NoError )
{
    QObject::connect ( m_reply, SIGNAL ( finished() ), this, SLOT ( parseData() ) );
    QObject::connect ( m_reply, SIGNAL ( error ( QNetworkReply::NetworkError ) ), this, SLOT ( error ( QNetworkReply::NetworkError ) ) );
}

SongPrivate::SongPrivate ( Song* qq, const QVariant& variant, QObject* parent ) : QObject ( parent ), m_reply ( 0 ), q ( qq )
{
    parse ( variant );
}


uint SongPrivate::albumID() const
{
    return m_albumID;
}

QString SongPrivate::albumName() const
{
    return m_albumName;
}

QString SongPrivate::artistCoverArtFilename() const
{
    return m_artistCoverArtFilename;
}

uint SongPrivate::artistID() const
{
    return m_artistID;
}

QString SongPrivate::artistName() const
{
    return m_artistName;
}

QString SongPrivate::avgDuration() const
{
    return m_avgDuration;
}

QString SongPrivate::avgRating() const
{
    return m_avgRating;
}

QString SongPrivate::coverArtFilename() const
{
    return m_coverArtFilename;
}

QString SongPrivate::estimateDuration() const
{
    return m_estimateDuration;
}

qulonglong SongPrivate::flags() const
{
    return m_flags;
}

uint SongPrivate::genreID() const
{
    return m_genreID;
}

bool SongPrivate::isLowBitrateAvailable() const
{
    return m_isLowBitrateAvailable;
}

bool SongPrivate::isVerified() const
{
    return m_isVerified;
}

qulonglong SongPrivate::popularity() const
{
    return m_popularity;
}

qulonglong SongPrivate::popularityIndex() const
{
    return m_popularityIndex;
}

qulonglong SongPrivate::rawScore() const
{
    return m_rawScore;
}

double SongPrivate::score() const
{
    return m_score;
}

uint SongPrivate::songID() const
{
    return m_songID;
}

QString SongPrivate::songName() const
{
    return m_songName;
}

QString SongPrivate::tsAdded() const
{
    return m_tsAdded;
}

uint SongPrivate::trackNum() const
{
    return m_trackNum;
}

uint SongPrivate::year() const
{
    return m_year;
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
    m_albumName = v.toString();

    v = songMap.value( QLatin1String( "ArtistCoverArtFilename" ) );
    if( !v.canConvert( QVariant::String ) )
        qDebug() << "'ArtistCoverArtFilename' field is invalid";
    m_artistCoverArtFilename = v.toString();

    v = songMap.value( QLatin1String( "ArtistID" ) );
    if( !v.canConvert( QVariant::UInt ) )
        qDebug() << "'ArtistID' field is invalid";
    m_artistID = v.toUInt();

    v = songMap.value( QLatin1String( "ArtistName" ) );
    if( !v.canConvert( QVariant::String ) )
        qDebug() << "'ArtistName' field is invalid";
    m_artistName = v.toString();

    v = songMap.value( QLatin1String( "AvgDuration" ) );
    if( !v.canConvert( QVariant::String ) )
        qDebug() << "'AvgDuration' field is invalid";
    m_avgDuration = v.toString();

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

    v = songMap.value( QLatin1String( "GenreID" ) );
    if( !v.canConvert( QVariant::UInt ) )
        qDebug() << "'GenreID' field is invalid";
    m_genreID = v.toUInt();

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

    v = songMap.value( QLatin1String( "PopularityIndex" ) );
    if( !v.canConvert( QVariant::ULongLong ) )
        qDebug() << "'PopularityIndex' field is invalid";
    m_popularityIndex = v.toULongLong();

    v = songMap.value( QLatin1String( "RawScore" ) );
    if( !v.canConvert( QVariant::ULongLong ) )
        qDebug() << "'RawScore' field is invalid";
    m_rawScore = v.toULongLong();

    v = songMap.value( QLatin1String( "Score" ) );
    if( !v.canConvert( QVariant::Double ) )
        qDebug() << "'Score' field is invalid";
    m_score = v.toDouble();

    v = songMap.value( QLatin1String( "SongID" ) );
    if( !v.canConvert( QVariant::UInt ) )
        qDebug() << "'SongID' field is invalid";
    m_songID = v.toUInt();

    v = songMap.value( QLatin1String( "SongName" ) );
    if( !v.canConvert( QVariant::String ) )
        qDebug() << "'SongName' field is invalid";
    m_songName = v.toString();

    v = songMap.value( QLatin1String( "TSAdded" ) );
    if( !v.canConvert( QVariant::String ) )
        qDebug() << "'TSAdded' field is invalid";
    m_tsAdded = v.toString();

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
    QJson::Parser parser;
    bool ok;
    QVariant variant = parser.parse ( data, &ok );
    if ( ok )
    {
        if ( !parse ( variant ) ) return false;
        return true;
    }
    else
    {
        return false;
    }
}

void SongPrivate::parseData()
{
    //parse and send signal
    if ( m_reply->error() == QNetworkReply::NoError )
    {
        if ( parse ( m_reply->readAll() ) )
        {
            emit q->finished();
        }
        else
        {
            emit q->parseError();
        }
    }
    m_reply->deleteLater();
}

void SongPrivate::error ( QNetworkReply::NetworkError error )
{
    this->m_error = error;
    emit q->requestError ( error );
}

Song::Song ( QNetworkReply* reply, QObject* parent ) : QObject ( parent ), d ( new SongPrivate ( this, reply ) )
{

}

Song::Song ( const QVariant& variant, QObject* parent ) : QObject ( parent ), d ( new SongPrivate ( this, variant ) )
{

}

Song::~Song()
{
    delete d;
}

uint Song::albumID() const
{
    return d->albumID();
}

QString Song::albumName() const
{
    return d->albumName();
}

QString Song::artistCoverArtFilename() const
{
    return d->artistCoverArtFilename();
}

uint Song::artistID() const
{
    return d->artistID();
}

QString Song::artistName() const
{
    return d->artistName();
}

QString Song::avgDuration() const
{
    return d->avgDuration();
}

QString Song::avgRating() const
{
    return d->avgRating();
}

QString Song::coverArtFilename() const
{
    return d->coverArtFilename();
}

QString Song::estimateDuration() const
{
    return d->estimateDuration();
}

qulonglong Song::flags() const
{
    return d->flags();
}

uint Song::genreID() const
{
    return d->genreID();
}

bool Song::isLowBitrateAvailable() const
{
    return d->isLowBitrateAvailable();
}

bool Song::isVerified() const
{
    return d->isVerified();
}

qulonglong Song::popularity() const
{
    return d->popularity();
}

qulonglong Song::popularityIndex() const
{
    return d->popularityIndex();
}

qulonglong Song::rawScore() const
{
    return d->rawScore();
}

double Song::score() const
{
    return d->score();
}

uint Song::songID() const
{
    return d->songID();
}

QString Song::songName() const
{
    return d->songName();
}

QString Song::tsAdded() const
{
    return d->tsAdded();
}

uint Song::trackNum() const
{
    return d->trackNum();
}

uint Song::year() const
{
    return d->year();
}

QString Song::errorString() const
{
    return d->errorString();
}

#include "song_p.moc"
#include "song.moc"
