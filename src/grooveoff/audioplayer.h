/*
    GrooveOff - Offline Grooveshark.com music
    Copyright (C) 2013  Giuseppe Calà <jiveaxe@gmail.com>

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


#ifndef AUDIOPLAYER_H
#define AUDIOPLAYER_H

#include "grooveoff/downloaditem.h"

#include <QWidget>
#include <phonon/MediaObject>
#include <phonon/AudioOutput>

namespace Ui {
class AudioPlayer;
}

class QLabel;
class QPushButton;

class AudioPlayer : public QWidget
{
    Q_OBJECT

public:
    explicit AudioPlayer(QWidget *parent = 0);
    virtual ~AudioPlayer();

    GrooveOff::DownloadState currentItemDownloadState() { return item->downloadState(); }

public slots:
    void playItem(DownloadItem *i);
    void pauseResumePlaying();

private slots:
    void tick(qint64 time);
    void aboutToFinish();
    void stopPlaying();
    void removeItem(DownloadItem *i);
    void stateChanged(Phonon::State newState, Phonon::State oldState);

private:
    Ui::AudioPlayer *ui_;
    Phonon::MediaObject *mediaObject_;
    Phonon::MediaObject *metaInformationResolver_;
    Phonon::AudioOutput *audioOutput_;
    DownloadItem *item;
    QList<Phonon::MediaSource> audioSources_;
    QString currentSongFileName_;
    bool updateState_;

    //Methods
    void setupUi();
    void setupActions();
};

#endif // AUDIOPLAYER_H
