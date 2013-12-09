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

#ifndef AUDIOENGINEPRIVATE_H
#define AUDIOENGINEPRIVATE_H

#include "playlistitem.h"

#include <phonon/MediaObject>

class AudioEnginePrivate : public QObject
{
    Q_OBJECT
public:
    AudioEnginePrivate( AudioEngine* q )
        : q_ptr ( q )
    {
    }
    AudioEngine* q_ptr;
    Q_DECLARE_PUBLIC ( AudioEngine )

public slots:
    void onStateChanged( Phonon::State newState, Phonon::State oldState );

private:
    //class AudioEngine* const q;

    Phonon::MediaObject* mediaObject;

    static AudioEngine* s_instance;
    PlaylistItemPtr currentTrack;
};

#endif // AUDIOENGINEPRIVATE_H
