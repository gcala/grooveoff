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


#ifndef MPRIS1_H
#define MPRIS1_H

#include <QDateTime>
#include <QDBusArgument>
#include <QObject>
#include <phonon/phononnamespace.h>
#include <PlaylistItem.h>

struct DBusStatus {    // From Amarok.
    DBusStatus()
        : play(Phonon::StoppedState)
    {}

    int play;            // Playing = 0, Paused = 1, Stopped = 2
};
Q_DECLARE_METATYPE(DBusStatus);

QDBusArgument& operator <<(QDBusArgument& arg, const DBusStatus& status);
const QDBusArgument& operator >>(const QDBusArgument& arg, DBusStatus& status);


struct Version {
    quint16 minor;
    quint16 major;
};
Q_DECLARE_METATYPE(Version);

QDBusArgument& operator <<(QDBusArgument& arg, const Version& version);
const QDBusArgument& operator >>(const QDBusArgument& arg, Version& version);

namespace mpris {

enum DBusCaps {
  NONE                 = 0,
  CAN_GO_NEXT          = 1 << 0,
  CAN_GO_PREV          = 1 << 1,
  CAN_PAUSE            = 1 << 2,
  CAN_PLAY             = 1 << 3,
  CAN_SEEK             = 1 << 4,
  CAN_PROVIDE_METADATA = 1 << 5,
  CAN_HAS_TRACKLIST    = 1 << 6,
};

class Mpris1Root;
class Mpris1Player;
class Mpris1TrackList;


class Mpris1 : public QObject
{
    Q_OBJECT

public:
    Mpris1(QObject* parent = 0,
            const QString& dbus_service_name = QString());
    ~Mpris1();

    static QVariantMap GetMetadata(const PlaylistItemPtr song);

    Mpris1Root* root() const { return root_; }
    Mpris1Player* player() const { return player_; }

private:
    static const char* kDefaultDbusServiceName;

    QString dbus_service_name_;

    Mpris1Root* root_;
    Mpris1Player* player_;
};


class Mpris1Root : public QObject {
  Q_OBJECT

public:
    Mpris1Root(QObject* parent = 0);

    QString Identity();
    void Quit();
    Version MprisVersion();
};


class Mpris1Player : public QObject {
    Q_OBJECT

public:
    Mpris1Player(QObject* parent = 0);

    void Pause();
    void Stop();
    void Prev();
    void Play();
    void Next();

    DBusStatus GetStatus() const;

    // those methods will use engine's state provided as an argument
    DBusStatus GetStatus(Phonon::State state) const;

    void VolumeSet(int volume);
    int VolumeGet() const;
    void PositionSet(int pos_msec);
    int PositionGet() const;
    QVariantMap GetMetadata() const;

    // Amarok extensions
    void VolumeUp(int vol);
    void VolumeDown(int vol);
    void Mute();

public slots:
    void CurrentSongChanged(
        const PlaylistItemPtr song, const QString& art_uri, const QImage&);

signals:
    void CapsChange(int);
    void TrackChange(const QVariantMap&);
    void StatusChange(DBusStatus);

private slots:
    void EngineStateChanged(Phonon::State newState, Phonon::State oldState);

private:
    QVariantMap last_metadata_;
};

} // namespace mpris

#endif // MPRIS1_H
