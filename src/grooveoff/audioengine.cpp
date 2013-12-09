/*
 * GrooveOff - Offline Grooveshark.com music
 * Copyright (C) 2013  Giuseppe Calà <Giuseppe.Cala-1973@poste.it>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "audioengine.h"
#include "audioengine_p.h"

#include <QMetaType>
#include <QDir>
#include <QDebug>

void AudioEnginePrivate::onStateChanged( Phonon::State newState, Phonon::State oldState )
{
}

AudioEngine* AudioEnginePrivate::s_instance = 0;

AudioEngine* AudioEngine::instance()
{
    return AudioEnginePrivate::s_instance;
}

AudioEngine::AudioEngine()
    : QObject(), d_ptr(new AudioEnginePrivate(this))
{
    Q_D( AudioEngine );
    d->s_instance = this;
    qRegisterMetaType< AudioErrorCode >("AudioErrorCode");
    qRegisterMetaType< AudioState >("AudioState");

    d->mediaObject = new Phonon::MediaObject( this );
    connect( d->mediaObject, SIGNAL( stateChanged( Phonon::State, Phonon::State ) ), d_func(), SLOT( onStateChanged( Phonon::State, Phonon::State ) ) );
    connect( d->mediaObject, SIGNAL( tick( qint64 ) ), SLOT( timerTriggered( qint64 ) ) );
    connect( d->mediaObject, SIGNAL( aboutToFinish() ), SLOT( onAboutToFinish() ) );
}

AudioEngine::~AudioEngine()
{
    delete d_ptr;

}

void AudioEngine::timerTriggered( qint64 time )
{
    Q_D( AudioEngine );
}

void AudioEngine::onAboutToFinish()
{
//     tDebug( LOGVERBOSE ) << Q_FUNC_INFO;
//     d_func()->expectStop = true;
}

void AudioEngine::playItem(PlaylistItemPtr track)
{
    qDebug() << "entrato";
    Q_D( AudioEngine );
    qDebug() << "Q_D fatto";
    d->currentTrack = track;
    qDebug() << " uguale";
    d->mediaObject->setCurrentSource(d->currentTrack->path() + QDir::separator() + d->currentTrack->fileName());
    qDebug() << "set current";
    d->mediaObject->play();
    qDebug() << "play";
}


#include "audioengine.moc"
#include "audioengine_p.moc"

