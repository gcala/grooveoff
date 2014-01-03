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


#include "TimerButton.h"

#include <QPainter>
#include <QTimer>
#include <QDebug>

TimerButton::TimerButton(QWidget *parent) :
    QPushButton(parent)
{
    countdownStarted_ = false;
    timer_ = new QTimer(this);
    connect(timer_, SIGNAL(timeout()), this, SLOT(decreaseCounter()));
}

TimerButton::~TimerButton()
{

}

void TimerButton::decreaseCounter()
{
    counter_--;
    if(counter_ == 0) {
        timer_->stop();
        countdownStarted_ = false;
        emit countdownFinished();
    }

    drawPixmap();
}

void TimerButton::drawPixmap()
{
    QPixmap pix(200,200);
    pix.fill(Qt::transparent);


    QPainterPath path;

    QPoint startPosition;
    switch(counter_) {
    case 4:
        startPosition.setX(100);
        startPosition.setY(50);
        break;
    case 3:
        startPosition.setX(150);
        startPosition.setY(100);
        break;
    case 2:
        startPosition.setX(100);
        startPosition.setY(150);
        break;
    case 1:
        startPosition.setX(50);
        startPosition.setY(100);
        break;
    default:
        startPosition.setX(100);
        startPosition.setY(50);
        break;

    }

    if(counter_ != 0) {
        path.moveTo(startPosition.x(), startPosition.y());
        path.arcTo(50,50,100,100,90-90*(4-counter_),-360+90*(4-counter_));
        path.lineTo(100,0);
        path.arcTo(0,0,200,200,90,360-90*(4-counter_));
        path.lineTo(startPosition.x(), startPosition.y());
    }

    QPainter painter;
    painter.begin(&pix);
    painter.setRenderHint(QPainter::Antialiasing);

    QPalette systemPalette;
    painter.setPen(QPen(systemPalette.color(QPalette::Highlight), 1, Qt::SolidLine));
    painter.setBrush(systemPalette.color(QPalette::Highlight));
    painter.drawPath(path);

    QFont textFont;
    textFont.setPixelSize(160);
    textFont.setBold(true);
    painter.setFont(textFont);
    painter.setPen(QPen(systemPalette.color(QPalette::Text), 1, Qt::SolidLine));
    painter.drawText(QRect(0,0,200,200), Qt::AlignCenter, QString::number(counter_));
    painter.end();

    setIcon(QIcon(pix));
}

void TimerButton::startCountdown()
{
    countdownStarted_ = true;
    counter_ = 4;
    timer_->start(1000);
    drawPixmap();
}

void TimerButton::stopCountdown()
{
    timer_->stop();
    countdownStarted_ = false;
}

