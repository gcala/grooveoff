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


#include "playanimationwidget.h"

#include <QPainter>
#include <QTime>

#define NUM_BARS 4
#define BAR_WIDTH 3
#define TICK 1
#define MIN_HEIGHT 3
#define SPEED 15

PlayAnimationWidget::PlayAnimationWidget(QWidget *parent) :
    QWidget(parent),
    animating_(false)
{
    connect(&timer_, SIGNAL(timeout()), SLOT(repaint()));

    QTime time = QTime::currentTime();
    qsrand((uint)time.msec());

    for(int i = 0; i < NUM_BARS; i++) {
        currentHeights_.append(MIN_HEIGHT);
    }
}

void PlayAnimationWidget::startAnimation()
{
    for(int i = 0; i < NUM_BARS; i++) {
        maxHeights_.append( randomHeight() );
        currentHeights_.append(MIN_HEIGHT);
        ticks_.append(TICK);
    }

    animating_ = true;
    timer_.start(SPEED);
}

void PlayAnimationWidget::stopAnimation()
{
    for(int i = 0; i < NUM_BARS; i++) {
        currentHeights_[i] = MIN_HEIGHT;
    }
    animating_ = false;
}

void PlayAnimationWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    QPainter painter(this);
//    painter.setRenderHint(QPainter::Antialiasing);
    QPalette systemPalette;
    painter.setPen(QPen(systemPalette.color(QPalette::Highlight), 1, Qt::SolidLine));
    painter.setBrush(systemPalette.color(QPalette::Highlight));

    // calculating spacer
    int emptySpace = rect().width() - BAR_WIDTH * NUM_BARS;
    int spacer = emptySpace / (NUM_BARS + 1);

    if(!animating_) {
        for(int i = 0; i < NUM_BARS; i++) {
            painter.drawRect(spacer*(i+1) + BAR_WIDTH*i, this->rect().height() - currentHeights_.at(i), BAR_WIDTH, this->rect().height());
        }
        return;
    }

    for(int i = 0; i < NUM_BARS; i++) {
        if(currentHeights_.at(i) > maxHeights_.at(i)) {
            ticks_[i] = -ticks_.at(i);
            currentHeights_[i] += ticks_.at(i) * 2;
        } else if(currentHeights_.at(i) < MIN_HEIGHT) {
            ticks_[i] = -ticks_.at(i);
            currentHeights_[i] += ticks_.at(i) * 2;
            maxHeights_[i] = randomHeight();
        }

        painter.drawRect(spacer * ( i + 1 ) + BAR_WIDTH * i, this->rect().height() - currentHeights_.at(i), BAR_WIDTH, this->rect().height());
        currentHeights_[i] += ticks_.at(i);
    }
}

int PlayAnimationWidget::randomHeight()
{
    return qrand() % ( rect().height() - MIN_HEIGHT ) + MIN_HEIGHT;
}

#include "playanimationwidget.moc"
