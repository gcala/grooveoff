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


#ifndef PLAYERWIDGET_H
#define PLAYERWIDGET_H

#include "downloaditem.h"
#include "grooveoffnamespace.h"

#include <QWidget>
#include <phonon/MediaObject>
#include <phonon/AudioOutput>

namespace Ui {
class PlayerWidget;
}

class QLabel;
class QPushButton;

class PlayerWidget : public QWidget
{
    Q_OBJECT

public:
    explicit PlayerWidget(QWidget *parent = 0);
    virtual ~PlayerWidget();

    void showElapsedTimerLabel(bool);

    GrooveOff::TimerState getTimerState() { return timerState; }
    void setTimerState(GrooveOff::TimerState state) { timerState = state; }

    void showMessage(const QString &message);

public slots:
    void pauseResumePlaying();
    void play(QString);

signals:
    void cambioStato(Phonon::State newState, QString source);

private slots:
    void tick(qint64 elapsedTime);
    void aboutToFinish();
    void stopPlaying();
    void stateChanged(Phonon::State newState, Phonon::State oldState);
    void toggleTimeLabel();
    void removeFromPlaylist();
    void sourceChanged(Phonon::MediaSource);
    void playPrevious();
    void playNext();

private:
    Ui::PlayerWidget *ui_;
    Phonon::MediaObject *mediaObject_;
    Phonon::MediaObject *metaInformationResolver_;
    Phonon::AudioOutput *audioOutput_;
    bool updateState_;
    GrooveOff::TimerState timerState;
    int oldIndex_;
    Phonon::MediaSource oldSource_;

    //Methods
    void setupUi();
    void setupLabels(int);
    void setupActions();
    int currentIndex(const QString & file);
};

#endif // PLAYERWIDGET_H
