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

#ifndef VOLUMEDIAL_H
#define VOLUMEDIAL_H

#include <QDial>
#include <QReadWriteLock>
#include <QSvgRenderer>
#include <QHash>


class VolumeDial : public QDial
{
    Q_OBJECT

public:
    VolumeDial( QWidget *parent = 0 );
    ~VolumeDial();
    /**
        Add a list of widgets that should not hide the tooltip on wheelevents, but instead cause
        wheelevents on the dial
        You do NOT have to remove them on deconstruction.
    */
    void addWheelProxies( QList<QWidget*> proxies );
    QSize sizeHint() const;

public slots:
    /**
       Remove an added wheelproxy. The slot is automatically bound to the widgets deconstruction
       signal when added. You don't have to do that.
    */
    void removeWheelProxy( QObject * );
    void setMuted( bool mute );

signals:
    void muteToggled( bool mute );

protected:
    void enterEvent( QEvent * );
    bool eventFilter( QObject *o, QEvent *e );
    void leaveEvent( QEvent * );
    void paintEvent( QPaintEvent * );
    void mouseMoveEvent( QMouseEvent * );
    void mousePressEvent( QMouseEvent * );
    void mouseReleaseEvent( QMouseEvent * );
    void resizeEvent(QResizeEvent *);
    void sliderChange( SliderChange change );
    void timerEvent ( QTimerEvent * );
    friend class MainToolbar;
    void wheelEvent( QWheelEvent * );

private:
    void startFade();
    void stopFade();
    void renderIcons();
    void updateSliderGradient();

private slots:
    void paletteChanged( const QPalette &palette );
    void valueChangedSlot( int );

private:
    QPixmap m_icon[8];
    QPixmap m_sliderGradient;
    int m_formerValue;
    QList<QWidget*> m_wheelProxies;
    struct
    {
        int step;
        int timer;
    } m_anim;
    bool m_isClick, m_isDown, m_muted, m_mouseOver;
    QColor m_highlightColor;

    /**
     * Calculate the luma of a color. Luma is weighted sum of gamma-adjusted
     * R'G'B' components of a color. The result is similar to qGray. The range
     * is from 0.0 (black) to 1.0 (white).
     *
     * KColorUtils::darken(), KColorUtils::lighten() and KColorUtils::shade()
     * operate on the luma of a color.
     *
     * @see http://en.wikipedia.org/wiki/Luma_(video)
     */
    qreal luma(const QColor&);

    /**
     * Calculate the contrast ratio between two colors, according to the
     * W3C/WCAG2.0 algorithm, (Lmax + 0.05)/(Lmin + 0.05), where Lmax and Lmin
     * are the luma values of the lighter color and the darker color,
     * respectively.
     *
     * A contrast ration of 5:1 (result == 5.0) is the minimum for "normal"
     * text to be considered readable (large text can go as low as 3:1). The
     * ratio ranges from 1:1 (result == 1.0) to 21:1 (result == 21.0).
     *
     * @see KColorUtils::luma
     */
    qreal contrastRatio(const QColor&, const QColor&);

    /**
     * Blend two colors into a new color by linear combination.
     * @code
        QColor lighter = KColorUtils::mix(myColor, Qt::white)
     * @endcode
     * @param c1 first color.
     * @param c2 second color.
     * @param bias weight to be used for the mix. @p bias <= 0 gives @p c1,
     * @p bias >= 1 gives @p c2. @p bias == 0.5 gives a 50% blend of @p c1
     * and @p c2.
     */
    QColor mix(const QColor &c1, const QColor &c2,
                            qreal bias = 0.5);
};

#endif  // end include guard
