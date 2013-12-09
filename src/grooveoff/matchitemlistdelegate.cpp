/*
    GrooveOff - Offline Grooveshark.com music
    Copyright (C) 2013  Giuseppe Calà <jiveaxe@gmail.com>

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


#include "matchitemlistdelegate.h"
#include "matcheslistmodel.h"
#include "roles.h"
#include "utility.h"

#include <QApplication>
#include <QPainter>
#include <QMouseEvent>
#include <QPixmapCache>
#include <QDir>

/*!
  \brief MatchItemListDelegate: this is the MatchItemListDelegate destructor.
  \param parent: The Parent Widget
*/
MatchItemListDelegate::MatchItemListDelegate(QObject *parent) :
    QAbstractItemDelegate(parent)
{
    currentRow_ = -1;
}

/*!
  \brief sizeHint: size hint
  \param option: option used for rendering the item
  \param index: index of the item
  \return size hint
*/
QSize MatchItemListDelegate::sizeHint ( const QStyleOptionViewItem& option, const QModelIndex& index ) const
{
    Q_UNUSED(option);
    Q_UNUSED(index);

    QFontMetrics fmTitle(Utility::font(QFont::Bold));

    //return QSize(Utility::coverSize + Utility::marginSize * 2 , Utility::coverSize + Utility::marginSize * 2);
    return QSize(200, (fmTitle.height()*2 > Utility::coverSize ? fmTitle.height()*2 : Utility::coverSize) + Utility::marginSize * 2);
}

/*!
  \brief paint: size hint
  \param painter: widget painter
  \param option: option used for rendering the item
  \param index: index of the item
  \return void
*/
void MatchItemListDelegate::paint ( QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index ) const
{
   /*
    * ITEM LAYOUT
    +----------------------------------------------------------+
    |                          margin                          |
    +-+------+-+---------------------------------------+-+---+-+
    | |      | |               title                   | |   | |
    |M| icon |M+---------------------------------------+M| B |M| B = PushButton
    | |      | |               artist                  | |   | | M = Margin
    +-+------+-+---------------------------------------+-+---+-+
    |                          margin                          |
    +----------------------------------------------------------+

    marigin | icon | margin | text | margin | button | margin

    */

    if (!index.isValid())
        return;

    QApplication::style()->drawPrimitive(QStyle::PE_PanelItemViewItem, &option, painter, 0);

    // Item height
    // is given by 2 margins (upper and lower) and twice the font metric height
    // the cover size is fixed (40px) so if height*2 is < coverSize the latter will be used
    QFontMetrics fmTitle(Utility::font(QFont::Bold));

    // album cover pixmap
    QPixmap pix;
    if(index.data(SongRoles::CoverPix).value<QPixmap>().isNull()) {
        pix = QIcon::fromTheme(QLatin1String("media-optical"), QIcon(QLatin1String(":/resources/media-optical.png"))).pixmap(Utility::coverSize);
        pix = pix.scaledToWidth(Utility::coverSize, Qt::SmoothTransformation);
    }
    else
        pix = index.data(SongRoles::CoverPix).value<QPixmap>();

    // rect hosting cover pixmap
    QRect coverRect;
    coverRect.setX(option.rect.x() + Utility::marginSize);
    coverRect.setY(option.rect.y() + (option.rect.height() / 2 - Utility::coverSize / 2));
    //coverRect.setY(option.rect.y() + (fmTitle.height()*2 > Utility::coverSize ? (fmTitle.height() - Utility::coverSize / 2) : (option.rect.height() / 2 - Utility::coverSize / 2)));
    coverRect.setWidth(Utility::coverSize);
    coverRect.setHeight(Utility::coverSize);
    painter->drawPixmap(coverRect, pix);

    // rect hosting song title
    QRect titleRect;
    titleRect.setX(option.rect.x() + Utility::marginSize * 2 + Utility::coverSize);
    titleRect.setY(option.rect.y() + Utility::marginSize);
    titleRect.setWidth(option.rect.width() - Utility::marginSize * 4 - Utility::coverSize - Utility::buttonSize);
    titleRect.setHeight((option.rect.height() - Utility::marginSize * 2)/2);

    // we're using bold font so save the painter
    painter->save();
    painter->setFont(Utility::font(QFont::Bold));

    QString title = index.data(Qt::DisplayRole).toString();

    painter->drawText(titleRect, Qt::AlignLeft | Qt::AlignTop, fmTitle.elidedText(title, Qt::ElideRight, titleRect.width()));
    painter->restore();

    QRect albumRect = titleRect;
    albumRect.setY(option.rect.y() + Utility::marginSize + titleRect.height());
    albumRect.setHeight((option.rect.height() - Utility::marginSize * 2) / 2);

    QString artist = index.data(SongRoles::Artist).toString();
    QString album = index.data(SongRoles::Album).toString();
    painter->drawText(albumRect, Qt::AlignLeft | Qt::AlignBottom, option.fontMetrics.elidedText(artist + " - " + album, Qt::ElideRight, albumRect.width()));

    QRect buttonRect;
    buttonRect.setX(option.rect.width() - Utility::buttonSize - Utility::marginSize -2 /* hack */);
    buttonRect.setY(option.rect.y() + (option.rect.height() / 2 - Utility::buttonSize / 2));
    buttonRect.setWidth(Utility::buttonSize);
    buttonRect.setHeight(Utility::buttonSize);

    QStyleOptionButton button;

    if(QFile::exists(path_ + QDir::separator() + index.data(SongRoles::Artist).toString() + " - " + index.data(Qt::DisplayRole).toString() + ".mp3")) {
        button.icon = QIcon::fromTheme(QLatin1String("view-refresh"), QIcon(QLatin1String(":/resources/view-refresh.png")));
    } else {
        if(QIcon::hasThemeIcon(QLatin1String("download")))
            button.icon = QIcon::fromTheme(QLatin1String("download"));
        else
            button.icon = QIcon::fromTheme(QLatin1String("document-save"), QIcon(QLatin1String(":/resources/download.png")));
    }

    button.iconSize = QSize(16,16);
    button.rect = buttonRect;
    button.features |= QStyleOptionButton::Flat;
    //button.state = _state | QStyle::State_Enabled;

    if(currentRow_ == index.row())
        button.state = QStyle::State_Sunken | QStyle::State_Enabled;
    else
        button.state = QStyle::State_Raised | QStyle::State_Enabled;

    QApplication::style()->drawControl(QStyle::CE_PushButton, &button, painter);
}

/*!
  \brief editorEvent: size hint
  \param event: event that triggered the editing
  \param model: model of the item
  \param option: option used for rendering the item
  \param index: index of the item
  \return bool: has handled the event?
*/
bool MatchItemListDelegate::editorEvent(QEvent *event, QAbstractItemModel *model, const QStyleOptionViewItem &option, const QModelIndex &index)
{
    Q_UNUSED(model)

    if( event->type() != QEvent::MouseButtonPress &&
        event->type() != QEvent::MouseButtonRelease ) {
        return true;
    }

    if( event->type() == QEvent::MouseButtonPress)
        currentRow_ = index.row();
    else
        currentRow_ = -1;

    // this rect represents a pushbutton
    QRect buttonRect;
    buttonRect.setX(option.rect.width() - Utility::buttonSize - Utility::marginSize);
    buttonRect.setY(option.rect.y() + (option.rect.height() / 2 - Utility::buttonSize / 2));
    buttonRect.setWidth(Utility::buttonSize);
    buttonRect.setHeight(Utility::buttonSize);

    // if mouse position is outside button rect/button area we use a State_Raised style
    QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
    if( !buttonRect.contains( mouseEvent->pos()) ) {
        return true;
    }

    // mouse pointer is in rect/button area; evaluating button press/release
    if( event->type() == QEvent::MouseButtonRelease) {
        // when MouseButtonRelease emit a signal like clicked()
        emit downloadRequest(index);
    }

    return true;
}

#include "matchitemlistdelegate.moc"
