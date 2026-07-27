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


#ifndef CONFIGDIALOG_H
#define CONFIGDIALOG_H

#include <QDialog>

namespace Ui {
class ConfigDialog;
}

class ConfigDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ConfigDialog( QWidget *parent = nullptr );

private Q_SLOTS:
    void restoreDefaults();
    void saveSettings();
    void okClicked();
    void switchPage( int );
    void onConfigChanged();

    /** throws the cached bytes away now, without waiting for a restart */
    void emptyCacheNow();

private:
    Ui::ConfigDialog *ui;

    bool configChanged;
    void loadSettings();
    void setupConnections();
    void setupGraphicElements();
    void setupSettingCategories();

    /** greys the proxy fields out when there is no proxy to configure */
    void updateEnabledState();

    /** how much of the disk the cache holds right now, in the label */
    void showCacheUsage();
};

#endif // CONFIGDIALOG_H
