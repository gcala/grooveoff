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
    
    ui->closeButton->setIcon( QIcon::fromTheme( QLatin1String( "dialog-close "),
                                                QIcon( QLatin1String( ":/resources/dialog-close.png" ) ) 
                                              ) 
                            );
    
    connect( ui->closeButton, SIGNAL(clicked()), 
                              SLOT(close())
           );
    
    ui->logoLabel->setPixmap( QPixmap( QLatin1String( ":/resources/grooveoff.png" ) ) );
    ui->logoLabel->setScaledContents( true );
    
    ui->gpl3Button->setIcon( QIcon( QLatin1String( ":/resources/gplv3-127x51.png" ) ) );
    ui->gpl3Button->setIconSize( QSize( 96,38 ) );
    
    ui->titleLabel->setText( trUtf8( "GrooveOff" ) );
    
    ui->versionLabel->setText( trUtf8("Version ") + GROOVEOFF_VERSION );
    
    ui->descriptionLabel->setWordWrap( true );
    ui->descriptionLabel->setTextFormat( Qt::RichText );
    ui->descriptionLabel->setText(trUtf8( "Offline Grooveshark.com music." ) + QLatin1String( "<br><br>" ) +
                                  trUtf8( "GrooveOff can access the huge Grooveshark database through its <a href=\"http://developers.grooveshark.com/docs/public_api/v3/\">public api</a>." ) + QLatin1String( "<br><br>" ) +
                                  trUtf8( "<b>Author</b>: " ) + QString( "Giuseppe Calà &lt;<a href=\"mailto:jiveaxe@gmail.com\">jiveaxe@gmail.com</a>&gt;<br><br>" ) +
                                  trUtf8( "<b>License</b>: ") + QLatin1String( "GPLv3+<br><br>" ) +
                                  QLatin1String( "<b>" ) + trUtf8( "Disclaimer:" ) + QLatin1String( "</b> " ) +
                                  trUtf8( "GrooveOff is not affiliated with Grooveshark™ and has not been reviewed or officially approved by Grooveshark.com. The author is not responsible for any violations this application does to Grooveshark's TOS. The author is not related to Grooveshark™ in any way! Support the Artists You Like by Buying Their Music." ) );
    
    ui->donateButton->setIcon( QIcon( ":/resources/btn_donate_LG.gif" ) );
    ui->donateButton->setIconSize( QSize( 96,27 ) );

    connect( ui->gpl3Button, SIGNAL(clicked()), 
                             SLOT(openGplPage()) 
           );
    
    connect( ui->donateButton, SIGNAL(clicked()), 
                               SLOT(openDonatePage()) 
           );

    setMinimumSize( 620,400 );
}

/*!
  \brief About dialog destructor.
*/
AboutDialog::~AboutDialog()
{

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
    QDesktopServices::openUrl( QUrl( QLatin1String( "https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=WJNETV7GLTKDG&item_name=" ) + 
                                     trUtf8( "Donation to Grooveoff's author" ) 
                                   )
                             );
}
