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
    explicit ConfigDialog( QWidget *parent = 0 );

private Q_SLOTS:
    void restoreDefaults();
    void saveSettings();
    void okClicked();
    void switchPage( int );
    void onConfigChanged();

private:
    Ui::ConfigDialog *ui;
    QStringList m_tagNames;
    QStringList m_localizedTagNames;
    QString m_localizedTimeTagName;

    bool configChanged;
    void loadSettings();
    void setupConnections();
    void setupTagElements();
    void setupGraphicElements();
    void setupSettingCategories();
};

#endif // CONFIGDIALOG_H
