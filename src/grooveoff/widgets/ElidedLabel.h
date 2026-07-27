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


// Thanks to Jason G
// http://gedgedev.blogspot.it/2010/12/elided-labels-in-qt.html

#ifndef ELIDEDLABEL_H
#define ELIDEDLABEL_H

#include <QLabel>

class QGraphicsDropShadowEffect;


class ElidedLabel : public QLabel
{
    Q_OBJECT

public:
    explicit ElidedLabel(QWidget* parent = 0, Qt::WindowFlags f = Qt::WindowFlags());
    explicit ElidedLabel(const QString& text, QWidget* parent = 0, Qt::WindowFlags f = Qt::WindowFlags());
    explicit ElidedLabel(const QString &txt, Qt::TextElideMode elideMode = Qt::ElideRight, QWidget * parent = 0, Qt::WindowFlags f = Qt::WindowFlags());

    //! Set the elide mode used for displaying text.
    void setElideMode(Qt::TextElideMode elideMode) {
        m_elideMode = elideMode;
        updateGeometry();
    }

    //! Get the elide mode currently used to display text.
    Qt::TextElideMode elideMode() const { return m_elideMode; }

    // QLabel overrides
    void setText(const QString &);

    //! An elided label is one line: its height cannot depend on its width.
    bool hasHeightForWidth() const override;

    // set shadow flag
    void enableShadow(bool ok);

protected:
    virtual void paintEvent(QPaintEvent* );

    virtual void changeEvent ( QEvent * event );


private:
    Qt::TextElideMode m_elideMode;
    bool m_drawShadow;
    QGraphicsDropShadowEffect *m_shadowEffect;
};

#endif // ELIDEDLABEL_H
