/*
    GrooveOff - Music streaming from torrents
    Copyright (C) 2026  Giuseppe Calà <jiveaxe@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/


#ifndef TRACKINFO_H
#define TRACKINFO_H

#include <QObject>
#include <QSharedPointer>

/**
  * @class TrackInfo
  * @short One file inside a torrent, as far as the rest of the application cares.
  *
  * Replaces GrooveShark::Song. @c infoHash plus @c fileIndex identify a track
  * everywhere: in the list, in the local stream url, in the cache and in the
  * catalogue. @c magnetUri is kept so that a playlist can reopen its torrent
  * even when neither the .torrent nor the resume file survive.
  *
  * The properties are declared as Q_PROPERTY because the drag and drop of
  * PlaylistItem serialises them through QMetaProperty::isStored(). Persistence,
  * on the other hand, maps them to columns explicitly: see src/db/Catalog.
  */
class TrackInfo : public QObject
{
    Q_OBJECT
    Q_PROPERTY( qint64  id        READ id        WRITE setId )
    Q_PROPERTY( QString infoHash  READ infoHash  WRITE setInfoHash )
    Q_PROPERTY( QString magnetUri READ magnetUri WRITE setMagnetUri )
    Q_PROPERTY( int     fileIndex READ fileIndex WRITE setFileIndex )
    Q_PROPERTY( QString filePath  READ filePath  WRITE setFilePath )
    Q_PROPERTY( qint64  fileSize  READ fileSize  WRITE setFileSize )
    Q_PROPERTY( bool    isAudio   READ isAudio   WRITE setIsAudio )
    Q_PROPERTY( bool    excluded  READ excluded  WRITE setExcluded )
    Q_PROPERTY( QString title     READ title     WRITE setTitle )
    Q_PROPERTY( QString artist    READ artist    WRITE setArtist )
    Q_PROPERTY( QString album     READ album     WRITE setAlbum )
    Q_PROPERTY( quint32 trackNum  READ trackNum  WRITE setTrackNum )
    Q_PROPERTY( quint32 year      READ year      WRITE setYear )
    Q_PROPERTY( qint64  duration  READ duration  WRITE setDuration )

public:
    explicit TrackInfo( QObject *parent = nullptr );
    ~TrackInfo() override;

    /** row id in the track table; 0 until the track has been stored */
    qint64 id() const;
    void setId( qint64 id );

    QString infoHash() const;
    void setInfoHash( const QString &infoHash );

    QString magnetUri() const;
    void setMagnetUri( const QString &magnetUri );

    /** index of the file inside the torrent */
    int fileIndex() const;
    void setFileIndex( int fileIndex );

    /** path of the file inside the torrent */
    QString filePath() const;
    void setFilePath( const QString &filePath );

    qint64 fileSize() const;
    void setFileSize( qint64 fileSize );

    /** true when the extension is a known audio one */
    bool isAudio() const;
    void setIsAudio( bool isAudio );

    /** true when the user explicitly refused this file; sticky across playlists */
    bool excluded() const;
    void setExcluded( bool excluded );

    QString title() const;
    void setTitle( const QString &title );

    QString artist() const;
    void setArtist( const QString &artist );

    QString album() const;
    void setAlbum( const QString &album );

    quint32 trackNum() const;
    void setTrackNum( quint32 trackNum );

    quint32 year() const;
    void setYear( quint32 year );

    /** length in milliseconds, 0 when unknown */
    qint64 duration() const;
    void setDuration( qint64 duration );

    /**
     * The title if tags were read, the file name without extension otherwise.
     * Never empty, so the ui always has something to show.
     */
    QString displayTitle() const;

    /** the file name inside the torrent, without the directories */
    QString fileName() const;

    /** true when the extension of @p filePath is a known audio one */
    static bool isAudioFile( const QString &filePath );

    bool operator==( const TrackInfo &other ) const;

private:
    qint64  m_id;
    QString m_infoHash;
    QString m_magnetUri;
    int     m_fileIndex;
    QString m_filePath;
    qint64  m_fileSize;
    bool    m_isAudio;
    bool    m_excluded;
    QString m_title;
    QString m_artist;
    QString m_album;
    quint32 m_trackNum;
    quint32 m_year;
    qint64  m_duration;
};

typedef QSharedPointer<TrackInfo> TrackInfoPtr;

QDataStream& operator<<( QDataStream& dataStream, const TrackInfoPtr track );
QDataStream& operator>>( QDataStream& dataStream, TrackInfoPtr track );

Q_DECLARE_METATYPE( TrackInfoPtr )

#endif // TRACKINFO_H
