/*
    GrooveOff - Offline Grooveshark.com music
    Copyright (C) 2013-2015  Giuseppe Calà <jiveaxe@gmail.com>

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


#ifndef MPRIS1PLUGINPLAYERADAPTOR_H
#define MPRIS1PLUGINPLAYERADAPTOR_H

#include <QObject>
#include <QVariantMap>

class AudioEngine;

struct PlayerStatus
{
    int state;  // 0 = Playing, 1 = Paused, 2 = Stopped.
    int random; // 0 = Playing linearly, 1 = Playing randomly.
    int repeat; // 0 = Go to the next element once the current has finished playing, 1 = Repeat the current element
    int repeatPlayList; // 0 = Stop playing once the last element has been played, 1 = Never give up playing
};

Q_DECLARE_METATYPE( PlayerStatus );

class Mpris1PluginPlayerAdaptor : public QObject
{
    Q_OBJECT
    Q_CLASSINFO( "D-Bus Interface", "org.freedesktop.MediaPlayer" )

public:
    Mpris1PluginPlayerAdaptor( QObject *parent = 0 );
    ~Mpris1PluginPlayerAdaptor();


    enum PlayerCaps
    {
        NONE                  = 0,
        CAN_GO_NEXT           = 1 << 0,
        CAN_GO_PREV           = 1 << 1,
        CAN_PAUSE             = 1 << 2,
        CAN_PLAY              = 1 << 3,
        CAN_SEEK              = 1 << 4,
        CAN_PROVIDE_METADATA  = 1 << 5,
        CAN_HAS_TRACKLIST     = 1 << 6
    };


public Q_SLOTS:
    void Next();
    void Prev();
    void Pause();
    void Stop();
    void Play();
    void Repeat( bool ok );
    PlayerStatus GetStatus();
    QVariantMap GetMetadata();
    int GetCaps();
    void VolumeSet( double volume );
    double VolumeGet();
    void PositionSet( int position );
    int PositionGet();

Q_SIGNALS:
    void CapsChange( int );
    void TrackChange( QVariantMap );
    void StatusChange( PlayerStatus );

private Q_SLOTS:
    void updateCaps();
    void updateTrack();
    void updateStatus();

private:
    AudioEngine *m_engine;
};

#endif
