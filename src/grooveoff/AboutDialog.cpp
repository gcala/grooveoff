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


#include "AboutDialog.h"
#include "ui_AboutDialog.h"
#include "Utility.h"

// version include
#include <../config-version.h>

#include <QDesktopServices>
#include <QUrl>

/*!
  \class AboudDialog
  \inheaderfile AboutDialog.h
  \brief The application About dialog.
*/
AboutDialog::AboutDialog( QWidget *parent )
    : QDialog( parent )
    , ui( new Ui::AboutDialog )
{
    ui->setupUi( this );
    
    setupGraphicElements();
    setupTextElements();
    setupConnections();

    setMinimumSize( 620,400 );
}

/*!
  \brief About dialog destructor.
*/
AboutDialog::~AboutDialog()
{

}

void AboutDialog::setupConnections()
{
    connect( ui->closeButton, &QPushButton::clicked, this, &AboutDialog::close );
    connect( ui->gpl3Button, &QPushButton::clicked, this, &AboutDialog::openGplPage );
    connect( ui->donateButton, &QPushButton::clicked, this, &AboutDialog::openDonatePage );
}

void AboutDialog::setupGraphicElements()
{
    ui->closeButton->setIcon( QIcon::fromTheme( QLatin1String( "dialog-close "), QIcon( QLatin1String( ":/resources/dialog-close.png" ) ) ) );
    
    ui->logoLabel->setPixmap( QPixmap( QLatin1String( ":/resources/grooveoff.png" ) ) );
    ui->logoLabel->setScaledContents( true );
    
    ui->gpl3Button->setIcon( QIcon( QLatin1String( ":/resources/gplv3-127x51.png" ) ) );
    ui->gpl3Button->setIconSize( QSize( 96,38 ) );
    
    ui->donateButton->setIcon( QIcon( ":/resources/btn_donate_LG.gif" ) );
    ui->donateButton->setIconSize( QSize( 96,27 ) );
}

void AboutDialog::setupTextElements()
{
    ui->titleLabel->setText( tr( "GrooveOff" ) );
    
    ui->versionLabel->setText( tr("Version ") + GROOVEOFF_VERSION );
    
    ui->descriptionLabel->setWordWrap( true );
    ui->descriptionLabel->setTextFormat( Qt::RichText );
    ui->descriptionLabel->setText( tr( "Music streaming from torrents." ) + QLatin1String( "<br><br>" ) +
                                   tr( "Paste a magnet link or open a .torrent file and it becomes a playlist." )
                                       + QLatin1String( "<br><br>" ) +
                                   QLatin1String( "<b>" ) + tr( "Author:" ) + QStringLiteral( "</b> Giuseppe Calà &lt;<a href=\"mailto:gcala@mailbox.org\">gcala@mailbox.org</a>&gt;<br><br>" ) +
                                   QLatin1String( "<b>" ) + tr( "License:" ) + QLatin1String( "</b> GPLv3+<br><br>" ) +
                                   QLatin1String( "<b>" ) + tr( "Disclaimer:" ) + QLatin1String( "</b> " ) +
                                   tr( "GrooveOff provides no content and is not affiliated with any tracker, "
                                       "indexer or torrent site. The end user is solely responsible for the "
                                       "sources used and for complying with the law where they are. The author "
                                       "accepts no liability for any use made of this application. Support the "
                                       "artists you like by buying their music." ) );
}



/*!
  \brief Open gpl license with web browser
*/
void AboutDialog::openGplPage()
{
    QDesktopServices::openUrl( QUrl( QLatin1String( "http://www.gnu.org/licenses/gpl.html" ) ) );
}

/*!
  \brief Open donation page with web browser
*/
void AboutDialog::openDonatePage()
{
    QDesktopServices::openUrl( QUrl( Utility::donateUrl ) );
}
