/****************************************************************************************
* Copyright (c) 2009 Thomas Luebking <thomas.luebking@web.de>                          *
*                                                                                      *
* This program is free software; you can redistribute it and/or modify it under        *
* the terms of the GNU General Public License as published by the Free Software        *
* Foundation; either version 2 of the License, or (at your option) any later           *
* version.                                                                             *
*                                                                                      *
* This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
* WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
* PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
*                                                                                      *
* You should have received a copy of the GNU General Public License along with         *
* this program.  If not, see <http://www.gnu.org/licenses/>.                           *
****************************************************************************************/

#ifndef ICONBUTTON_H
#define ICONBUTTON_H

#include <QImage>
#include <QPixmap>
#include <QWidget>
#include <QMenu>

class IconButton : public QWidget
{
    Q_OBJECT

public:
    enum Type { Previous = 0, 
                Next, 
                PlayPause, 
                Search, 
                Browse, 
                Trash, 
                Batch, 
                Download, 
                Remove, 
                Stop, 
                Redownload, 
                Clock, 
                Aborted, 
                Warning, 
                Settings, 
                Offline, 
                Online };

    IconButton( QWidget *parent = 0 );
    virtual QSize sizeHint() const;
    void setIcon( const QImage &img, int steps = 0 );
    void setType( const Type type );
    Type type() const;
    
    void setMenu( QMenu *menu );
    QMenu *menu() const;

signals:
    void clicked();

protected:
    virtual void mousePressEvent( QMouseEvent * );
    virtual void mouseReleaseEvent( QMouseEvent * );
    virtual void paintEvent( QPaintEvent * );
    virtual void resizeEvent(QResizeEvent *);
    virtual void timerEvent ( QTimerEvent * );

    /**
     Reload the content for the given size
     The iconbutton preserves a square size, so sz.width() == sz.height()
    */
    virtual void reloadContent( const QSize &sz ) = 0;

protected slots:
    void svgRetinted();

private:
    void updateIconBuffer();

    bool m_isClick;
    struct
    {
        int step;
        int steps;
        int timer;
    } m_anim;

    struct
    {
        QImage image;
        QPixmap pixmap;
    } m_buffer;

    QImage m_icon, m_oldIcon;
    Type m_type;
    
    QMenu *m_menu;
};


#endif  // end include guard
