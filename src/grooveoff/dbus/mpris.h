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


#ifndef MPRIS_H
#define MPRIS_H

#include <QObject>

namespace mpris {

class Mpris1;
class Mpris2;

class Mpris : public QObject {
  Q_OBJECT

public:
  Mpris(QObject* parent = 0);

signals:
//   void RaiseMainWindow();

private:
  Mpris1* mpris1_;
  Mpris2* mpris2_;
};

} // namespace mpris

#endif // MPRIS_H
