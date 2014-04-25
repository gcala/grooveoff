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


#ifndef PLAYERWIDGET_H
#define PLAYERWIDGET_H

#include "DownloadItem.h"
#include "GrooveOffNamespace.h"

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

    GrooveOff::TimerState getTimerState() const { return m_timerState; }
    void setTimerState(GrooveOff::TimerState state) { m_timerState = state; }

    void showMessage(const QString &message);

public Q_SLOTS:
    void pauseResumePlaying();
    void reloadPreviousNextButtons();

private Q_SLOTS:
    void tick(qint64 elapsedTime, bool userSeek);
    void stateChanged(Phonon::State state);
    void toggleTimeLabel();
    void sourceChanged();
    void playPrevious();
    void playNext();
    void removedPlayingTrack();
    void switchPage();
    void muteStateChanged( bool mute );

private:
    Ui::PlayerWidget *ui;
    GrooveOff::TimerState m_timerState;
    bool m_playedRemoved;
    QTimer *m_timer;

    //Methods
    void setupUi();
    void setupActions();
};

#endif // PLAYERWIDGET_H
