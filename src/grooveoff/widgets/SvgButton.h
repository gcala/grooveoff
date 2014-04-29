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
        QImage play[3], pause[2], next[3], previous[3], search[3], browse[2], batch[2], download[2], redownload[2], stop[2], trash[2], remove[2], clock[2], aborted[2], warning[2];
    } m_icon;

    bool m_isEnabled;
    bool m_isPlaying;
    bool m_fileExists;
    
    QImage icon();
};

#endif // BUTTONFACTORY_H
