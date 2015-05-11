/*
    GrooveOff - Offline Grooveshark.com music
    Copyright (C) 2013-2015  Giuseppe Calà <jiveaxe@gmail.com>

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

// this code was "borrowed" from ktp-kded-integration-module

#ifndef NAMINGSCHEMA_H
#define NAMINGSCHEMA_H

#include <QLineEdit>

class NamingSchema : public QLineEdit    //krazy:exclude=qclasses
{
     Q_OBJECT

public:
    NamingSchema(QWidget *parent = 0);
    virtual ~NamingSchema();

    void setLocalizedTagNames(QStringList tagNames);

protected:
    void dropEvent(QDropEvent *event);
    void mousePressEvent(QMouseEvent *event);

private:
    QStringList m_localizedTagNames;
};

#endif // NAMINGSCHEMA_H
