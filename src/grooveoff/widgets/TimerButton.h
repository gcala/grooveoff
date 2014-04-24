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


#ifndef TIMERBUTTON_H
#define TIMERBUTTON_H

#include <QPushButton>


class TimerButton : public QPushButton
{
    Q_OBJECT
public:
    explicit TimerButton(QWidget *parent = 0);
    virtual ~TimerButton();

    bool isCountdownStarted() { return countdownStarted_;}

    void startCountdown();
    void stopCountdown();

signals:
    void countdownFinished();

private slots:
    void decreaseCounter();

private:
    int counter_;
    QTimer *timer_;
    bool countdownStarted_;

    void drawPixmap();
};

#endif // TIMERBUTTON_H
