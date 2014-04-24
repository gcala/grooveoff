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


#ifndef MPRIS2PLUGINPLAYERADAPTOR_H
#define MPRIS2PLUGINPLAYERADAPTOR_H

#include <QDBusAbstractAdaptor>
#include <QVariantMap>
#include <QDBusObjectPath>
#include <QMap>
#include <QUuid>
#include <phonon/phononnamespace.h>


class AudioEngine;

class Mpris2PluginPlayerAdaptor : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO( "D-Bus Interface", "org.mpris.MediaPlayer2.Player" )

    Q_PROPERTY( bool CanControl READ canControl )
    bool canControl() const;

    Q_PROPERTY( bool CanGoNext READ canGoNext )
    bool canGoNext() const;

    Q_PROPERTY( bool CanGoPrevious READ canGoPrevious )
    bool canGoPrevious() const;

    Q_PROPERTY( bool CanPause READ canPause )
    bool canPause() const;

    Q_PROPERTY( bool CanPlay READ canPlay )
    bool canPlay() const;

    Q_PROPERTY( bool CanSeek READ canSeek )
    bool canSeek() const;

    Q_PROPERTY( QString LoopStatus READ loopStatus WRITE setLoopStatus )
    QString loopStatus() const;
    void setLoopStatus( const QString &value );

    Q_PROPERTY( double MaximumRate READ maximumRate )
    double maximumRate() const;

    Q_PROPERTY( QVariantMap Metadata READ metadata )
    QVariantMap metadata() const;

    Q_PROPERTY( double MinimumRate READ minimumRate )
    double minimumRate() const;

    Q_PROPERTY( QString PlaybackStatus READ playbackStatus )
    QString playbackStatus() const;

    Q_PROPERTY( qlonglong Position READ position )
    qlonglong position() const;

    Q_PROPERTY( double Rate READ rate WRITE setRate )
    double rate() const;
    void setRate( double value );

    Q_PROPERTY( bool Shuffle READ shuffle WRITE setShuffle )
    bool shuffle() const;
    void setShuffle( bool value );

    Q_PROPERTY( double Volume READ volume WRITE setVolume )
    double volume() const;
    void setVolume( double value );

public:
    Mpris2PluginPlayerAdaptor( QObject *parent = 0 );
    virtual ~Mpris2PluginPlayerAdaptor();

public Q_SLOTS:
    void Next();
    void OpenUri( const QString &uri );
    void Pause();
    void Play();
    void PlayPause();
    void Previous();
    void Seek( qlonglong Offset );
    void SetPosition( const QDBusObjectPath &TrackId, qlonglong Position );
    void Stop();

Q_SIGNALS:
    void Seeked( qlonglong Position );

private Q_SLOTS:
    void emitPropertiesChanged();
    void onSeeked( qint64 elapsed, bool userSeek );

private:
    void syncProperties();
    AudioEngine *m_engine;
    QMap<QString, QVariant> m_props;
};

#endif
