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

#ifndef SESSIONITEM_H
#define SESSIONITEM_H

#include "GrooveOffNamespace.h"

#include <QWidget>

namespace Ui {
class SessionItem;
}

class SessionItem : public QWidget
{
    Q_OBJECT

public:
    explicit SessionItem(const QString &name, int num, QWidget *parent = 0);
    ~SessionItem();
    QString sessionName() const;
    void updateNumTrack(int);

    void deleteButtonEnabled(bool ok);

protected:
    virtual void leaveEvent ( QEvent * event );
    virtual void enterEvent ( QEvent * event );

Q_SIGNALS:
    void remove(const QString &);

private Q_SLOTS:
    void multiFuncBtnClicked();
    void removeSession();

private:
    Ui::SessionItem *ui;
    GrooveOff::DownloadState m_state;
    bool m_deleteButtonEnabled;

    void setupUi();
    void setupConnections();
    void stateChanged();
};

#endif // SESSIONITEM_H
