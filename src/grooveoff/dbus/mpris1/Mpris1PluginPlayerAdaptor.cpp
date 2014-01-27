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


#include "Mpris1PluginPlayerAdaptor.h"
#include "AudioEngine.h"
#include "PlaylistItem.h"
#include "Utility.h"

#include <QFile>
#include <QDBusMetaType>
#include <QDBusArgument>
#include <phonon/phononnamespace.h>


//register << operator
QDBusArgument &operator << ( QDBusArgument &arg, const PlayerStatus &status )
{
    arg.beginStructure();
    arg << status.state;
    arg << status.random;
    arg << status.repeat;
    arg << status.repeatPlayList;
    arg.endStructure();
    return arg;
}

//register >> operator
const QDBusArgument &operator >> ( const QDBusArgument &arg, PlayerStatus &status )
{
    arg.beginStructure();
    arg >> status.state;
    arg >> status.random;
    arg >> status.repeat;
    arg >> status.repeatPlayList;
    arg.endStructure();
    return arg;
}

Mpris1PluginPlayerAdaptor::Mpris1PluginPlayerAdaptor( QObject *parent )
    : QObject( parent )
{
    qDBusRegisterMetaType< PlayerStatus > ();
    m_engine = The::audioEngine();
    connect( m_engine, SIGNAL( stateChanged ( Phonon::State ) ),
                       SLOT( updateCaps() ) );
    connect( m_engine, SIGNAL( stateChanged ( Phonon::State ) ),
                       SLOT( updateStatus() ) );
    connect( m_engine, SIGNAL( sourceChanged() ),
                       SLOT( updateTrack() ) );
}

Mpris1PluginPlayerAdaptor::~Mpris1PluginPlayerAdaptor()
{}

void Mpris1PluginPlayerAdaptor::Next()
{
    m_engine->next();
}

void Mpris1PluginPlayerAdaptor::Prev()
{
    m_engine->previous();
}

void Mpris1PluginPlayerAdaptor::Pause()
{
    m_engine->pause();
}

void Mpris1PluginPlayerAdaptor::Stop()
{
    m_engine->stop();
}

void Mpris1PluginPlayerAdaptor::Play()
{
    m_engine->play();
}

void Mpris1PluginPlayerAdaptor::Repeat( bool ok )
{
    Q_UNUSED( ok )
}

PlayerStatus Mpris1PluginPlayerAdaptor::GetStatus()
{
    PlayerStatus st;
    switch ( m_engine->state() )
    {
    case Phonon::StoppedState:
    case Phonon::ErrorState:
        st.state = 2;
        break;
    case Phonon::PlayingState:
    case Phonon::BufferingState:
        st.state = 0;
        break;
    case Phonon::PausedState:
        st.state = 1;
    };

    return st;
}

QVariantMap Mpris1PluginPlayerAdaptor::GetMetadata()
{
    QVariantMap map;
    PlaylistItemPtr track = m_engine->currentTrack();

    map.insert( QLatin1String( "location" ), "file://" + track->path() + track->fileName() );
    map.insert( QLatin1String( "arturl" ), Utility::coversCachePath + track->song()->coverArtFilename() );
    map.insert( QLatin1String( "title" ), track->song()->songName() );
    map.insert( QLatin1String( "artist" ), track->song()->artistName() );
    map.insert( QLatin1String( "album" ), track->song()->albumName() );
    map.insert( QLatin1String( "time" ), ( quint32 ) m_engine->currentTrackTotalTime() / 1000 );
    map.insert( QLatin1String( "mtime" ), ( quint32 ) m_engine->currentTrackTotalTime() );
    map.insert( QLatin1String( "year" ), track->song()->year() );
    return map;
}

int Mpris1PluginPlayerAdaptor::GetCaps()
{
    int caps = NONE;
    if (GetStatus().state == 0)
        caps |= CAN_PAUSE;
    else
        caps |= CAN_PLAY;
    if ( ( GetStatus().state < 2 ) && ( m_engine->currentTrackTotalTime() > 0 ) )
        caps |= CAN_SEEK;
    caps |= CAN_GO_NEXT;
    caps |= CAN_GO_PREV;
    caps |= CAN_PROVIDE_METADATA;
    return caps;
}

void Mpris1PluginPlayerAdaptor::VolumeSet( double volume )
{
    m_engine->setVolume( volume * 100 );
}

double Mpris1PluginPlayerAdaptor::VolumeGet()
{
    return static_cast< double >( m_engine->volume() ) / 100.0;
}

void Mpris1PluginPlayerAdaptor::PositionSet( int position )
{
    m_engine->seek( position );
}

int Mpris1PluginPlayerAdaptor::PositionGet()
{
    return m_engine->currentTime();
}

void Mpris1PluginPlayerAdaptor::updateCaps()
{
    emit CapsChange( GetCaps() );
}

void Mpris1PluginPlayerAdaptor::updateTrack()
{
    emit TrackChange( GetMetadata() );
}

void Mpris1PluginPlayerAdaptor::updateStatus()
{
    emit StatusChange( GetStatus() );
}
