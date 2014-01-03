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


#include "SettingsItem.h"
#include "ui_SettingsItem.h"

/*!
  \brief SettingsItem: this is the SettingsItem constructor
  \param name: setting name
  \param icon: setting icon
  \param parent: The Parent Widget
*/
SettingsItem::SettingsItem(const QString &name, const QString &icon, QWidget *parent) :
    QWidget(parent),
    ui_(new Ui::SettingsItem),
    settingName_(name),
    settingIcon_(icon)
{
    ui_->setupUi(this);
    if(settingIcon_.isEmpty()) {
        ui_->settingIconLabel->setPixmap(QPixmap(QLatin1String(":/resources/grooveoff.png")));
        ui_->settingIconLabel->setScaledContents(true);
    }
    else if(QIcon::hasThemeIcon(settingIcon_))
        ui_->settingIconLabel->setPixmap(QIcon::fromTheme(settingIcon_).pixmap(48,48));
    else
        ui_->settingIconLabel->setPixmap(QIcon::fromTheme(QLatin1String("software-update-available"), QIcon(QLatin1String(":/resources/" + settingIcon_.toLatin1() + ".png"))).pixmap(48,48));

    ui_->settingNameLabel->setText(settingName_);
}


