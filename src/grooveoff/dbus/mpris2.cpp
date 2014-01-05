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


#include "mpris2.h"
#include "AudioEngine.h"
#include "Playlist.h"
#include "Utility.h"

#include <algorithm>

#include "mpris_common.h"
#include "mpris1.h"

#include "mpris2_player.h"
#include "mpris2_root.h"

#include <QApplication>
#include <QDBusConnection>
#include <QtConcurrentRun>

// Use these to convert between time units
const qint64 kNsecPerSec  = 1000000000ll;
const qint64 kUsecPerSec  = 1000000ll;
const qint64 kNsecPerUsec = 1000ll;

namespace mpris {

const char* Mpris2::kMprisObjectPath = "/org/mpris/MediaPlayer2";
const char* Mpris2::kServiceName = "org.mpris.MediaPlayer2.grooveoff";
const char* Mpris2::kFreedesktopPath = "org.freedesktop.DBus.Properties";

Mpris2::Mpris2(Mpris1* mpris1, QObject* parent)
    : QObject(parent),
      mpris1_(mpris1)
{
    new Mpris2Root(this);
    new Mpris2Player(this);

    if (!QDBusConnection::sessionBus().registerService(kServiceName)) {
        qWarning() << "Failed to register" << QString(kServiceName) << "on the session bus";
        return;
    }

    QDBusConnection::sessionBus().registerObject(kMprisObjectPath, this);

    connect(The::audioEngine(), SIGNAL(stateChanged(Phonon::State, Phonon::State)), SLOT(EngineStateChanged(Phonon::State, Phonon::State)));
    connect(The::audioEngine(), SIGNAL(volumeChanged(int)), SLOT(VolumeChanged(int)));
    connect(The::audioEngine(), SIGNAL(seeked(qint64)), SIGNAL(Seeked(qint64)));

    connect(The::audioEngine(), SIGNAL(sourceChanged()), SLOT(CurrentSongChanged()));
}

void Mpris2::EngineStateChanged(Phonon::State newState, Phonon::State oldState)
{
    if(newState != Phonon::PlayingState && newState != Phonon::PausedState) {
        last_metadata_= QVariantMap();
        EmitNotification("Metadata");
    }

    EmitNotification( "PlaybackStatus", PlaybackStatus(newState));
    EmitNotification( "CanGoNext" );
    EmitNotification( "CanGoPrevious" );
    EmitNotification( "CanPause" );
}

void Mpris2::VolumeChanged(int)
{
    EmitNotification("Volume");
}

void Mpris2::EmitNotification(const QString& name, const QVariant& val)
{
    QDBusMessage msg = QDBusMessage::createSignal(
            kMprisObjectPath, kFreedesktopPath, "PropertiesChanged");
    QVariantMap map;
    map.insert(name, val);
    QVariantList args = QVariantList()
                        << "org.mpris.MediaPlayer2.Player"
                        << map
                        << QStringList();
    msg.setArguments(args);
    QDBusConnection::sessionBus().send(msg);
}

void Mpris2::EmitNotification(const QString& name)
{
    QVariant value;
    if      (name == "PlaybackStatus")  value = PlaybackStatus();
//     else if (name == "LoopStatus")      value = LoopStatus();
//     else if (name == "Shuffle")         value = Shuffle();
    else if (name == "Metadata")        value = Metadata();
    else if (name == "Volume")          value = Volume();
    else if (name == "Position")        value = Position();
    else if (name == "CanGoNext")       value = CanGoNext();
    else if (name == "CanGoPrevious")   value = CanGoPrevious();
    else if (name == "CanPause")        value = CanPause();

    if (value.isValid())
        EmitNotification(name, value);
}

//------------------Root Interface--------------------------//

bool Mpris2::CanQuit() const
{
    return true;
}

bool Mpris2::CanRaise() const
{
    return true;
}

bool Mpris2::HasTrackList() const
{
    return true;
}

QString Mpris2::Identity() const
{
    return QCoreApplication::applicationName();
}

QString Mpris2::DesktopEntryAbsolutePath() const
{
    QStringList xdg_data_dirs = QString(getenv("XDG_DATA_DIRS")).split(":");
    xdg_data_dirs.append("/usr/local/share/");
    xdg_data_dirs.append("/usr/share/");

    foreach (const QString& directory, xdg_data_dirs) {
        QString path = QString("%1/applications/%2.desktop").
                    arg(directory, QApplication::applicationName().toLower());
        if (QFile::exists(path))
        return path;
    }
    return QString();
}

QString Mpris2::DesktopEntry() const
{
    return QApplication::applicationName().toLower();
}

QStringList Mpris2::SupportedUriSchemes() const
{
    static QStringList res = QStringList()
                            << "file"
                            << "http"
                            << "cdda"
                            << "smb"
                            << "sftp";
    return res;
}

QStringList Mpris2::SupportedMimeTypes() const
{
    static QStringList res = QStringList()
                            << "application/ogg"
                            << "application/x-ogg"
                            << "application/x-ogm-audio"
                            << "audio/aac"
                            << "audio/mp4"
                            << "audio/mpeg"
                            << "audio/mpegurl"
                            << "audio/ogg"
                            << "audio/vnd.rn-realaudio"
                            << "audio/vorbis"
                            << "audio/x-flac"
                            << "audio/x-mp3"
                            << "audio/x-mpeg"
                            << "audio/x-mpegurl"
                            << "audio/x-ms-wma"
                            << "audio/x-musepack"
                            << "audio/x-oggflac"
                            << "audio/x-pn-realaudio"
                            << "audio/x-scpls"
                            << "audio/x-speex"
                            << "audio/x-vorbis"
                            << "audio/x-vorbis+ogg"
                            << "audio/x-wav"
                            << "video/x-ms-asf"
                            << "x-content/audio-player";
    return res;
}

void Mpris2::Raise()
{
//   emit RaiseMainWindow();
}

void Mpris2::Quit()
{
//     qApp->quit();
}

QString Mpris2::PlaybackStatus() const
{
    return PlaybackStatus(The::audioEngine()->state());
}

QString Mpris2::PlaybackStatus(Phonon::State state) const
{
    switch (state) {
        case Phonon::PlayingState: return "Playing";
        case Phonon::PausedState:  return "Paused";
        default:                   return "Stopped";
    }
}

double Mpris2::Rate() const
{
    return 1.0;
}

void Mpris2::SetRate(double rate)
{
    if(rate == 0) {
        if (mpris1_->player()) {
        mpris1_->player()->Pause();
        }
    }
}

QVariantMap Mpris2::Metadata() const
{
    return last_metadata_;
}

QString Mpris2::current_track_id() const
{
    return QString("/org/mpris/MediaPlayer2/Track/%1").arg(
            QString::number(The::audioEngine()->currentTrack()->song()->songID()));
}

// We send Metadata change notification as soon as the process of
// changing song starts...
void Mpris2::CurrentSongChanged()
{
    PlaylistItemPtr track = The::audioEngine()->currentTrack();

    last_metadata_ = QVariantMap();
    AddMetadata("xesam:url", track->path() + track->fileName(), &last_metadata_);
    AddMetadata("xesam:title", track->song()->songName(), &last_metadata_);
    AddMetadataAsList("xesam:artist", track->song()->artistName(), &last_metadata_);
    AddMetadata("xesam:album", track->song()->albumName(), &last_metadata_);
    AddMetadata("mpris:length", static_cast<qlonglong>(The::audioEngine()->currentTrackTotalTime()) * 1000000, &last_metadata_);
    if (track->song()->avgRating().toDouble() != -1.0) {
        AddMetadata("xesam:userRating", track->song()->avgRating().toDouble(), &last_metadata_);
    }

    using mpris::AddMetadata;
    AddMetadata("mpris:trackid", current_track_id(), &last_metadata_);

    const QString coverpath = Utility::coversCachePath + track->song()->coverArtFilename();

    if (!coverpath.isEmpty()) {
        AddMetadata("mpris:artUrl", QString( QUrl::fromLocalFile( coverpath ).toEncoded() ), &last_metadata_);
    }

    EmitNotification("Metadata", last_metadata_);
}

double Mpris2::Volume() const
{
    if (mpris1_->player()) {
        return double(mpris1_->player()->VolumeGet()) / 100;
    } else {
        return 0.0;
    }
}

void Mpris2::SetVolume(double value)
{
    The::audioEngine()->setVolume(value * 100);
}

double Mpris2::MaximumRate() const {
    return 1.0;
}

double Mpris2::MinimumRate() const {
    return 1.0;
}

bool Mpris2::CanGoNext() const {
    return The::audioEngine()->canGoNext();
}

bool Mpris2::CanGoPrevious() const {
    return The::audioEngine()->canGoPrevious();
}

bool Mpris2::CanPlay() const {
    return The::audioEngine()->currentTrack() ||  !The::playlist()->count();
}

// This one's a bit different than MPRIS 1 - we want this to be true even when
// the song is already paused or stopped.
bool Mpris2::CanPause() const
{
  return The::audioEngine()->currentTrack();
}

bool Mpris2::CanSeek() const
{
//   if (mpris1_->player()) {
//     return mpris1_->player()->GetCaps() & CAN_SEEK;
//   } else {
//     return true;
//   }
    /* to be improved */
    return true;
}

bool Mpris2::CanControl() const
{
  return true;
}

void Mpris2::Next()
{
    The::audioEngine()->next();
}

void Mpris2::Previous() {
    The::audioEngine()->previous();
}

void Mpris2::Pause()
{
    The::audioEngine()->pause();
}

void Mpris2::PlayPause()
{
    The::audioEngine()->playPause();
}

void Mpris2::Stop()
{
    The::audioEngine()->stop();
}

void Mpris2::Play()
{
    The::audioEngine()->play();
}

void Mpris2::Seek(qlonglong offset)
{
/*
  if(CanSeek()) {
      qDebug() << "Valore di seek" << The::audioEngine()->currentTime() / kNsecPerSec + offset / kUsecPerSec;
      The::audioEngine()->seek(The::audioEngine()->currentTime() / kNsecPerSec +
                    offset / kUsecPerSec);
//     app_->player()->SeekTo(app_->player()->engine()->position_nanosec() / kNsecPerSec +
//                     offset / kUsecPerSec);
  }
  */
    if ( !CanSeek() )
        return;

    qlonglong seekTime = Position() + offset;
    if ( seekTime < 0 )
        The::audioEngine()->seek( 0 );
    else if ( seekTime > The::audioEngine()->currentTrackTotalTime()*1000 )
        Next();
    // seekTime is in microseconds, but we work internally in milliseconds
    else
        The::audioEngine()->seek( (qint64) ( seekTime / 1000 ) );
}

qlonglong Mpris2::Position() const
{
    return The::audioEngine()->currentTime() * 1000;
//     return The::audioEngine()->currentTime() / kNsecPerUsec;
//   return app_->player()->engine()->position_nanosec() / kNsecPerUsec;
}

void Mpris2::SetPosition(const QDBusObjectPath& trackId, qlonglong offset)
{
    if (CanSeek() && trackId.path() == QString("/org/mpris/MediaPlayer2/Track/%1").arg(
        QString::number(The::audioEngine()->currentTrack()->song()->songID())) && offset >= 0) {
            offset *= kNsecPerUsec;

            if(offset < The::audioEngine()->currentTrackTotalTime()) {
                The::audioEngine()->seek(offset / kNsecPerSec);
            }
    }
}

void Mpris2::OpenUri(const QString& uri)
{

}

TrackIds Mpris2::Tracks() const
{
    //TODO
    return TrackIds();
}

bool Mpris2::CanEditTracks() const
{
    return false;
}

TrackMetadata Mpris2::GetTracksMetadata(const TrackIds &tracks) const
{
    //TODO
    return TrackMetadata();
}

void Mpris2::AddTrack(const QString &uri, const QDBusObjectPath &afterTrack, bool setAsCurrent)
{
    //TODO
}

void Mpris2::RemoveTrack(const QDBusObjectPath &trackId)
{
    //TODO
}

void Mpris2::GoTo(const QDBusObjectPath &trackId)
{
    //TODO
}

} // namespace mpris
