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


#ifndef PLAYANIMATIONWIDGET_H
#define PLAYANIMATIONWIDGET_H

#include <QWidget>
#include <QTimer>

class PlayAnimationWidget : public QWidget
{
    Q_OBJECT
public:
    explicit PlayAnimationWidget(QWidget *parent = 0);

    void startAnimation();
    void stopAnimation();

signals:

public slots:

protected:
    void paintEvent(QPaintEvent *event);

private:
    QTimer timer_;
    QVector<int> maxHeights_;
    QVector<int> currentHeights_;
    QVector<int> ticks_;
    bool animating_;

    int randomHeight();
};

#endif // PLAYANIMATIONWIDGET_H
