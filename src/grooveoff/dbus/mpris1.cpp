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


#include "mpris1.h"
#include "mpris_common.h"
#include "AudioEngine.h"

#include <QDBusConnection>

#include "mpris_player.h"
#include "mpris_root.h"

// Use these to convert between time units
const qint64 kMsecPerSec  = 1000ll;

namespace mpris {

const char* Mpris1::kDefaultDbusServiceName = "org.mpris.grooveoff";

Mpris1::Mpris1(QObject* parent,
               const QString& dbus_service_name)
    : QObject(parent),
      dbus_service_name_(dbus_service_name),
      root_(NULL),
      player_(NULL)
{
    qDBusRegisterMetaType<DBusStatus>();
    qDBusRegisterMetaType<Version>();

    if (dbus_service_name_.isEmpty()) {
        dbus_service_name_ = kDefaultDbusServiceName;
    }

    if (!QDBusConnection::sessionBus().registerService(dbus_service_name_)) {
        qWarning() << "Failed to register" << dbus_service_name_ << "on the session bus";
        return;
    }

    root_ = new Mpris1Root(this);
    player_ = new Mpris1Player(this);
}

Mpris1::~Mpris1() {
    QDBusConnection::sessionBus().unregisterService(dbus_service_name_);
}

Mpris1Root::Mpris1Root(QObject* parent)
    : QObject(parent)
{
    new MprisRoot(this);
    QDBusConnection::sessionBus().registerObject("/", this);
}

Mpris1Player::Mpris1Player(QObject* parent)
    : QObject(parent)
{
    new MprisPlayer(this);
    QDBusConnection::sessionBus().registerObject("/Player", this);

    connect(The::audioEngine(), SIGNAL(stateChanged(Phonon::State,Phonon::State)),
            SLOT(EngineStateChanged(Phonon::State,Phonon::State)));
}

// we use the state from event and don't try to obtain it from Player
// later because only the event's version is really the current one
void Mpris1Player::EngineStateChanged(Phonon::State newState, Phonon::State oldState)
{
    emit StatusChange(GetStatus(newState));
}

void Mpris1Player::CurrentSongChanged(const PlaylistItemPtr song, const QString& art_uri, const QImage&)
{
    last_metadata_ = Mpris1::GetMetadata(song);

    if (!art_uri.isEmpty()) {
        AddMetadata("arturl", art_uri, &last_metadata_);
    }

    emit TrackChange(last_metadata_);
    emit StatusChange(GetStatus());
}


QString Mpris1Root::Identity()
{
    return QString("%1 %2").arg(
      QCoreApplication::applicationName(),
      QCoreApplication::applicationVersion());
}

Version Mpris1Root::MprisVersion()
{
    Version version;
    version.major = 1;
    version.minor = 0;
    return version;
}

void Mpris1Root::Quit()
{
    qApp->quit();
}

void Mpris1Player::Pause()
{
    The::audioEngine()->pause();
}

void Mpris1Player::Stop()
{
    The::audioEngine()->stop();
}

void Mpris1Player::Prev()
{
    The::audioEngine()->previous();
}

void Mpris1Player::Play()
{
    The::audioEngine()->play();
}

void Mpris1Player::Next()
{
    The::audioEngine()->next();
}

DBusStatus Mpris1Player::GetStatus() const
{
    return GetStatus(The::audioEngine()->state());
}

DBusStatus Mpris1Player::GetStatus(Phonon::State state) const
{
    DBusStatus status;
    switch (state) {
        case Phonon::PlayingState:
        status.play = Phonon::PlayingState;
        break;
        case Phonon::PausedState:
        status.play = Phonon::PausedState;
        break;
        default:
        status.play = Phonon::StoppedState;
        break;
    }
  return status;
}

void Mpris1Player::VolumeSet(int volume)
{
    The::audioEngine()->setVolume(volume);
}

int Mpris1Player::VolumeGet() const
{
    return The::audioEngine()->volume();
}

void Mpris1Player::PositionSet(int pos_msec)
{
//     The::audioEngine()->seek( pos_msec / kMsecPerSec );
    The::audioEngine()->seek( (qint64) (pos_msec / 1000 ) );
}

int Mpris1Player::PositionGet() const
{
    return The::audioEngine()->currentTime() * 1000;
}

QVariantMap Mpris1Player::GetMetadata() const
{
    return last_metadata_;
}

void Mpris1Player::VolumeUp(int change)
{
    VolumeSet(VolumeGet() + change);
}

void Mpris1Player::VolumeDown(int change)
{
    VolumeSet(VolumeGet() - change);
}

void Mpris1Player::Mute()
{
    The::audioEngine()->setMuted(true);
}

QVariantMap Mpris1::GetMetadata(const PlaylistItemPtr item)
{
    QVariantMap ret;

    AddMetadata("location", item->path() + item->fileName(), &ret);
    AddMetadata("title", item->song()->songName(), &ret);
    AddMetadata("artist", item->song()->artistName(), &ret);
    AddMetadata("album", item->song()->albumName(), &ret);
    AddMetadata("year", (qint64)item->song()->year(), &ret);
    if (item->song()->avgRating().toDouble() != -1.0) {
        AddMetadata("rating", item->song()->avgRating().toInt() * 5, &ret);
    }

    return ret;
}

} // namespace mpris


QDBusArgument& operator<< (QDBusArgument& arg, const Version& version)
{
    arg.beginStructure();
    arg << version.major << version.minor;
    arg.endStructure();
    return arg;
}

const QDBusArgument& operator>> (const QDBusArgument& arg, Version& version)
{
    arg.beginStructure();
    arg >> version.major >> version.minor;
    arg.endStructure();
    return arg;
}

QDBusArgument& operator<< (QDBusArgument& arg, const DBusStatus& status)
{
    arg.beginStructure();
    arg << status.play;
    arg.endStructure();
    return arg;
}

const QDBusArgument& operator>> (const QDBusArgument& arg, DBusStatus& status)
{
    arg.beginStructure();
    arg >> status.play;
    arg.endStructure();
    return arg;
}

