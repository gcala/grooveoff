/*
    GrooveOff - Offline Grooveshark.com music
    Copyright (C) 2013-2014  Giuseppe Calà <jiveaxe@gmail.com>

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


#include "Mpris2PluginPlayerAdaptor.h"
#include "AudioEngine.h"
#include "Playlist.h"
#include "Utility.h"

#include <QFile>
#include <QDBusMetaType>
#include <QDBusArgument>
#include <QDBusMessage>
#include <QDBusConnection>

static QByteArray idFromPlaylistItem( const PlaylistItemPtr item )
{
    return QByteArray( "/org/grooveoff/tid_" ) +
           QByteArray::number( item->song()->songID(), 16 ).rightJustified( 8, '0' );
}

Mpris2PluginPlayerAdaptor::Mpris2PluginPlayerAdaptor( QObject *parent )
    : QDBusAbstractAdaptor( parent )
{
    m_engine = The::audioEngine();
    connect( m_engine, SIGNAL( sourceChanged() ),
                       SLOT( emitPropertiesChanged() ) );
    connect( m_engine, SIGNAL( stateChanged( Phonon::State ) ),
                       SLOT( emitPropertiesChanged() ) );
    connect( m_engine, SIGNAL( volumeChanged( int ) ),
                       SLOT( emitPropertiesChanged() ) );
    connect( m_engine, SIGNAL( seeked( qint64,bool ) ),
                       SLOT( onSeeked( qint64, bool ) ) );
    connect( m_engine, SIGNAL( seekableChanged( bool ) ),
                       SLOT( emitPropertiesChanged() ) );
    connect( The::playlist(), SIGNAL( playlistChanged() ),
                              SLOT( emitPropertiesChanged() ) );
    syncProperties();
}

Mpris2PluginPlayerAdaptor::~Mpris2PluginPlayerAdaptor()
{}

bool Mpris2PluginPlayerAdaptor::canControl() const
{
    return true;
}

bool Mpris2PluginPlayerAdaptor::canGoNext() const
{
    return m_engine->canGoNext();
}

bool Mpris2PluginPlayerAdaptor::canGoPrevious() const
{
    return m_engine->canGoPrevious();
}

bool Mpris2PluginPlayerAdaptor::canPause() const
{
    return (m_engine->state() == Phonon::PausedState || m_engine->state() == Phonon::PlayingState);
}
bool Mpris2PluginPlayerAdaptor::canPlay() const
{
    return The::playlist()->count() != 0;
}

bool Mpris2PluginPlayerAdaptor::canSeek() const
{
    return m_engine->isSeekable();
}

QString Mpris2PluginPlayerAdaptor::loopStatus() const
{
    return "None";
}

void Mpris2PluginPlayerAdaptor::setLoopStatus( const QString &value )
{
    Q_UNUSED( value )
}

double Mpris2PluginPlayerAdaptor::maximumRate() const
{
    return 1.0;
}

QVariantMap Mpris2PluginPlayerAdaptor::metadata() const
{
    if( !m_engine->currentTrack() )
        return QVariantMap();

    PlaylistItemPtr track = m_engine->currentTrack();

    QVariantMap map;
    map["mpris:length"] = qMax( m_engine->currentTrackTotalTime() * 1000 , qint64( 0 ) );

    const QString coverpath = Utility::coversCachePath + track->song()->coverArtFilename();
    map["mpris:artUrl"] = QUrl::fromLocalFile( coverpath ).toString();

    map["xesam:album"] = track->song()->albumName();
    map["xesam:artist"] = QStringList( track->song()->artistName() );
    map["xesam:title"] = track->song()->songName();

    QByteArray playingTrackFileId = idFromPlaylistItem( track );
    map["mpris:trackid"] = QVariant::fromValue< QDBusObjectPath >( QDBusObjectPath( playingTrackFileId.constData() ) );

    map["xesam:url"] =  QUrl::fromLocalFile( track->path() + track->fileName() ).toString();
    return map;
}

double Mpris2PluginPlayerAdaptor::minimumRate() const
{
    return 1.0;
}

QString Mpris2PluginPlayerAdaptor::playbackStatus() const
{
    if( m_engine->state() == Phonon::PlayingState )
        return "Playing";
    else if ( m_engine->state() == Phonon::PausedState )
        return "Paused";
    return "Stopped";
}

double Mpris2PluginPlayerAdaptor::rate() const
{
    return 1.0;
}

void Mpris2PluginPlayerAdaptor::setRate( double value )
{
    Q_UNUSED( value )
}

bool Mpris2PluginPlayerAdaptor::shuffle() const
{
    return false;
}

void Mpris2PluginPlayerAdaptor::setShuffle( bool value )
{
    Q_UNUSED( value )
}

double Mpris2PluginPlayerAdaptor::volume() const
{
    return static_cast< double >( m_engine->volume() ) / 100.0;
}

void Mpris2PluginPlayerAdaptor::Mpris2PluginPlayerAdaptor::setVolume( double value )
{
    value = qBound( 0.0, value, 1.0 );
    m_engine->setVolume( value * 100 );
}

void Mpris2PluginPlayerAdaptor::Next()
{
    m_engine->next();
}

void Mpris2PluginPlayerAdaptor::OpenUri( const QString &uri )
{
}

void Mpris2PluginPlayerAdaptor::Pause()
{
    m_engine->pause();
}

void Mpris2PluginPlayerAdaptor::Play()
{
    m_engine->play();
}

void Mpris2PluginPlayerAdaptor::PlayPause()
{
    if( m_engine->state() == Phonon::StoppedState )
        m_engine->play();
    else if( m_engine->state() == Phonon::PausedState || m_engine->state() == Phonon::PlayingState )
        m_engine->playPause();
}

void Mpris2PluginPlayerAdaptor::Previous()
{
    m_engine->previous();
}

void Mpris2PluginPlayerAdaptor::Seek( qlonglong Offset )
{
    if ( !canSeek() )
        return;

    qlonglong seekTime = position() + Offset;
    if ( seekTime < 0 )
        m_engine->seek( 0 );
    else if ( seekTime > m_engine->currentTrackTotalTime() * 1000 )
        Next();
    // seekTime is in microseconds, but we work internally in milliseconds
    else
        m_engine->seek( (qint64) ( seekTime / 1000 ) );
}

qlonglong Mpris2PluginPlayerAdaptor::position() const
{
    return (qlonglong) ( m_engine->currentTime() * 1000 ); // prima era m_engine->currentTime()*1000
}

void Mpris2PluginPlayerAdaptor::SetPosition( const QDBusObjectPath &TrackId, qlonglong Position )
{
    if ( !canSeek() )
        return;

    QByteArray currentTrackId = idFromPlaylistItem( m_engine->currentTrack() );

    if ( TrackId.path().toLatin1() != currentTrackId ) {
        qWarning( "Mpris2PluginPlayerAdaptor: SetPosition() called with an invalid trackId" );
        return;
    }

    if ( ( Position < 0 ) || ( Position > m_engine->currentTrackTotalTime() * 1000 )  )
        return;

    m_engine->seek( (qint64) ( Position / 1000 ) );
}

void Mpris2PluginPlayerAdaptor::Stop()
{
    m_engine->stop();
}

void Mpris2PluginPlayerAdaptor::emitPropertiesChanged()
{
    QList<QByteArray> changedProps;
    if( m_props["CanGoNext"] != canGoNext() )
        changedProps << "CanGoNext";
    if( m_props["CanGoPrevious"] != canGoPrevious() )
        changedProps << "CanGoPrevious";
    if( m_props["CanPause"] != canPause() )
        changedProps << "CanPause";
    if( m_props["CanPlay"] != canPlay() )
        changedProps << "CanPlay";
    if( m_props["CanSeek"] != canSeek() )
        changedProps << "CanSeek";
    if( m_props["LoopStatus"] != loopStatus() )
        changedProps << "LoopStatus";
    if( m_props["MaximumRate"] != maximumRate() )
        changedProps << "MaximumRate";
    if( m_props["MinimumRate"] != minimumRate() )
        changedProps << "MinimumRate";
    if( m_props["PlaybackStatus"] != playbackStatus() )
        changedProps << "PlaybackStatus";
    if( m_props["Rate"] != rate() )
        changedProps << "Rate";
    if( m_props["Shuffle"] != shuffle() )
        changedProps << "Shuffle";
    if( m_props["Volume"] != volume() )
        changedProps << "Volume";
    if( m_props["Metadata"] != metadata() )
        changedProps << "Metadata";

    if( changedProps.isEmpty() )
        return;

    syncProperties();

    QVariantMap map;
    foreach( QByteArray name, changedProps )
        map.insert( name, m_props.value( name ) );

    QDBusMessage msg = QDBusMessage::createSignal("/org/mpris/MediaPlayer2",
                                                  "org.freedesktop.DBus.Properties",
                                                  "PropertiesChanged");
    msg << "org.mpris.MediaPlayer2.Player";
    msg << map;
    msg << QStringList();
    QDBusConnection::sessionBus().send( msg );
}

void Mpris2PluginPlayerAdaptor::onSeeked( qint64 elapsed, bool userSeek )
{
    if ( userSeek )
        emit Seeked( elapsed * 1000 );
}

void Mpris2PluginPlayerAdaptor::syncProperties()
{
    m_props["CanGoNext"] = canGoNext();
    m_props["CanGoPrevious"] = canGoPrevious();
    m_props["CanPause"] = canPause();
    m_props["CanPlay"] = canPlay();
    m_props["CanSeek"] = canSeek();
    m_props["LoopStatus"] = loopStatus();
    m_props["MaximumRate"] = maximumRate();
    m_props["MinimumRate"] = minimumRate();
    m_props["PlaybackStatus"] = playbackStatus();
    m_props["Rate"] = rate();
    m_props["Shuffle"] = shuffle();
    m_props["Volume"] = volume();
    m_props["Metadata"] = metadata();
}
