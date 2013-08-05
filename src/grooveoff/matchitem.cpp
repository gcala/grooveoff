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


#include "grooveoff/matchitem.h"
#include "grooveoff/utility.h"
#include "ui_matchitem.h"
#include <QGraphicsDropShadowEffect>
#include <QFile>
#include <QDir>
#include <QDebug>

MatchItem::MatchItem(const QSharedPointer<SongObject> &song, QWidget *parent) :
    QWidget(parent),
    ui_(new Ui::MatchItem),
    song_(song)
{
    ui_->setupUi(this);
    setupUi();

    connect(ui_->downloadButton, SIGNAL(clicked()), SLOT(downloadSlot()));
    connect(song_.data(), SIGNAL(reloadCover()), this, SLOT(loadCover()));
    connect(song_.data(), SIGNAL(reloadIcon()), this, SLOT(setDownloadIcon()));
}

MatchItem::~MatchItem()
{
}

void MatchItem::setupUi()
{
    ui_->coverLabel->setScaledContents(true);
    ui_->coverLabel->setFixedSize(QSize(Utility::coverSize,Utility::coverSize));

    loadCover();

    ui_->coverLabel->setToolTip(song_.data()->title() + " - " + song_.data()->artist());

    QGraphicsDropShadowEffect *coverShadow = new QGraphicsDropShadowEffect(this);
    coverShadow->setBlurRadius(10.0);
    coverShadow->setColor(palette().color(QPalette::Shadow));
    coverShadow->setOffset(0.0);

    ui_->coverLabel->setGraphicsEffect(coverShadow);

    ui_->titleLabel->setFont(Utility::font(QFont::Bold));
    ui_->titleLabel->setText(song_.data()->title());
    ui_->titleLabel->setToolTip(song_.data()->title());
    ui_->titleLabel->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred); // fix hidden label

    ui_->artist_albumLabel->setText(song_.data()->artist() + " - " + song_.data()->album());
    ui_->artist_albumLabel->setToolTip(song_.data()->title());
    ui_->artist_albumLabel->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred); // fix hidden label

    ui_->downloadButton->setFixedSize(QSize(Utility::buttonSize,Utility::buttonSize));
    ui_->downloadButton->setIconSize(QSize(16,16));

    setDownloadIcon();
}

void MatchItem::loadCover()
{
    if(!song_.data()->coverName().isEmpty() && QFile::exists(Utility::coversCachePath + song_.data()->coverName()))
        ui_->coverLabel->setPixmap(QPixmap(Utility::coversCachePath + song_.data()->coverName()));
    else
        ui_->coverLabel->setPixmap(QIcon::fromTheme(QLatin1String("media-optical"),
                                   QIcon(QLatin1String(":/resources/media-optical.png"))).pixmap(Utility::coverSize));
}

void MatchItem::downloadSlot()
{
    emit download(song_);
}

void MatchItem::setDownloadIcon()
{
    if(QFile::exists(Utility::downloadPath + QDir::separator() + song_.data()->title() + " - " + song_.data()->artist() + ".mp3")) {
        ui_->downloadButton->setIcon(QIcon::fromTheme(QLatin1String("view-refresh"),
                                     QIcon(QLatin1String(":/resources/view-refresh.png"))));
    } else {
        if(QIcon::hasThemeIcon(QLatin1String("download")))
            ui_->downloadButton->setIcon(QIcon::fromTheme(QLatin1String("download")));
        else
            ui_->downloadButton->setIcon(QIcon::fromTheme(QLatin1String("document-save"),
                                         QIcon(QLatin1String(":/resources/download.png"))));
    }
}

#include "matchitem.moc"
