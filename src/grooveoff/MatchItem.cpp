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


#include "MatchItem.h"
#include "ui_MatchItem.h"
#include "Utility.h"

#include <QGraphicsDropShadowEffect>
#include <QFile>
#include <QDir>
#include <QDebug>

using namespace GrooveShark;

MatchItem::MatchItem(const PlaylistItemPtr &playlistItem, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::MatchItem),
    m_playlistItem(playlistItem)
{
    ui->setupUi(this);
    setupUi();

    connect(ui->downloadButton, SIGNAL(buttonClicked()), SLOT(downloadSlot()));
    connect(m_playlistItem.data(), SIGNAL(reloadCover()), this, SLOT(loadCover()));
    connect(m_playlistItem.data(), SIGNAL(reloadIcon()), this, SLOT(setDownloadIcon()));
}

MatchItem::~MatchItem()
{
}

void MatchItem::setupUi()
{
    ui->coverLabel->setScaledContents(true);
    ui->coverLabel->setFixedSize(QSize(Utility::coverSize,Utility::coverSize));

    loadCover();

    ui->coverLabel->setToolTip(m_playlistItem->song()->songName() + " - " + m_playlistItem->song()->artistName());

    QGraphicsDropShadowEffect *coverShadow = new QGraphicsDropShadowEffect(this);
    coverShadow->setBlurRadius(10.0);
    coverShadow->setColor(palette().color(QPalette::Shadow));
    coverShadow->setOffset(0.0);

    ui->coverLabel->setGraphicsEffect(coverShadow);

    ui->titleLabel->setFont(Utility::font(QFont::Bold));
    ui->titleLabel->setText(m_playlistItem->song()->songName());
    ui->titleLabel->setToolTip(m_playlistItem->song()->songName());
    ui->titleLabel->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred); // fix hidden label

    ui->artist_albumLabel->setText(m_playlistItem->song()->artistName() + " - " + m_playlistItem->song()->albumName());
    ui->artist_albumLabel->setToolTip(m_playlistItem->song()->songName());
    ui->artist_albumLabel->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred); // fix hidden label

    ui->downloadButton->setType(IconButton::Download);
    ui->downloadButton->setFixedSize(QSize(Utility::buttonSize,Utility::buttonSize));

    setDownloadIcon();
    
    ui->mainLayout->setContentsMargins(4,4,4,4);
    ui->downloadLayout->setContentsMargins(0,4,0,4);
}

void MatchItem::loadCover()
{
    if(!m_playlistItem->song()->coverArtFilename().isEmpty()
        && QFile::exists(Utility::coversCachePath + m_playlistItem->song()->coverArtFilename())
        && m_playlistItem->song()->coverArtFilename() != "0")
        ui->coverLabel->setPixmap(QPixmap(Utility::coversCachePath + m_playlistItem->song()->coverArtFilename()));
    else
        ui->coverLabel->setPixmap(QIcon::fromTheme(QLatin1String("media-optical"),
                                  QIcon(QLatin1String(":/resources/media-optical.png"))).pixmap(Utility::coverSize));
}

void MatchItem::downloadSlot()
{
    emit download(m_playlistItem);
}

void MatchItem::setDownloadIcon()
{
    QString schema = Utility::namingSchema;
    schema.replace(QLatin1String("%title"), m_playlistItem->song()->songName(), Qt::CaseInsensitive);
    schema.replace(QLatin1String("%artist"), m_playlistItem->song()->artistName(), Qt::CaseInsensitive);
    schema.replace(QLatin1String("%album"), m_playlistItem->song()->albumName(), Qt::CaseInsensitive);
    schema.replace(QLatin1String("%track"), QString::number(m_playlistItem->song()->trackNum()), Qt::CaseInsensitive);
    
    ui->downloadButton->setExists(QFile::exists(Utility::destinationPath + QDir::separator() + schema + ".mp3"));
}

