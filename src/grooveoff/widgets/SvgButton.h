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


#ifndef BUTTONFACTORY_H
#define BUTTONFACTORY_H

#include "IconButton.h"

class SvgButton : public IconButton
{
    Q_OBJECT
public:
    SvgButton( QWidget *parent = 0 );
    void setButtonEnabled(bool);
    
    inline bool playing() const { return m_isPlaying; }
    void setPlaying( bool playing );
    
    void setExists( bool ok );

protected:
    void enterEvent( QEvent * );
    void leaveEvent( QEvent * );
    void mousePressEvent( QMouseEvent * );
    void mouseReleaseEvent( QMouseEvent * );
    void reloadContent( const QSize &sz );

private slots:
    void clicked();

signals:
    void buttonClicked();

private:
    struct
    {
        QImage play[3];
        QImage pause[2];
        QImage next[3];
        QImage previous[3];
        QImage search[3];
        QImage browse[2];
        QImage batch[2];
        QImage download[2];
        QImage redownload[2];
        QImage stop[2];
        QImage trash[2];
        QImage remove[2];
        QImage clock[2];
        QImage aborted[2];
        QImage warning[2];
        QImage settings[2];
        QImage offline[2];
        QImage online[2];
    } m_icon;

    bool m_isEnabled;
    bool m_isPlaying;
    bool m_fileExists;
    
    QImage icon();
};

#endif // BUTTONFACTORY_H
