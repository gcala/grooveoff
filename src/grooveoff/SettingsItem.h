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


#ifndef SETTINGSITEM_H
#define SETTINGSITEM_H

#include <QWidget>

namespace Ui {
class SettingsItem;
}

class SettingsItem : public QWidget
{
    Q_OBJECT

public:
    SettingsItem(const QString &name, const QString &icon, QWidget *parent = 0);

private:
    Ui::SettingsItem *ui_;
    QString settingName_;
    QString settingIcon_;
};

#endif // SETTINGSITEM_H
