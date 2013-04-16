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


#include "grooveoff/aboutdialog.h"
#include "ui_aboutdialog.h"

// version include
#include <config-version.h>

#include <QDesktopServices>
#include <QUrl>

/*!
  \brief AboutDialog: this is the AboutDialog constructor.
  \param parent: The Parent Widget
*/
AboutDialog::AboutDialog(QWidget *parent) :
    QDialog(parent),
    ui_(new Ui::AboutDialog)
{
    ui_->setupUi(this);
    ui_->closeButton->setIcon(QIcon::fromTheme(QLatin1String("dialog-close"), QIcon(QLatin1String(":/resources/dialog-close.png"))));
    connect(ui_->closeButton, SIGNAL(clicked()), this, SLOT(close()));
    ui_->logoLabel->setPixmap(QPixmap(QLatin1String(":/resources/grooveoff.png")));
    ui_->logoLabel->setScaledContents(true);
    ui_->gpl3Button->setIcon(QIcon(QLatin1String(":/resources/gplv3-127x51.png")));
    ui_->gpl3Button->setIconSize(QSize(96,38));
    ui_->titleLabel->setText(trUtf8("GrooveOff"));
    ui_->versionLabel->setText(trUtf8("Version ") + GROOVEOFF_VERSION);
    ui_->descriptionLabel->setWordWrap(true);
    ui_->descriptionLabel->setTextFormat(Qt::RichText);
    ui_->descriptionLabel->setText(trUtf8("Offline Grooveshark.com music.") + QLatin1String("<br><br>") +
                                  trUtf8("GrooveOff can access the huge Grooveshark database through its <a href=\"http://developers.grooveshark.com/docs/public_api/v3/\">public api</a>.") + QLatin1String("<br><br>") +
                                  trUtf8("<b>Author</b>: ") + QString("Giuseppe Calà &lt;<a href=\"mailto:jiveaxe@gmail.com\">jiveaxe@gmail.com</a>&gt;<br><br>") +
                                  trUtf8("<b>License</b>: ") + QLatin1String("GPLv3+<br><br>") +
                                  QLatin1String("<b>") + trUtf8("Disclaimer:") + QLatin1String("</b> ") +
                                  trUtf8("GrooveOff is not affiliated with Grooveshark™ and has not been reviewed or officially approved by Grooveshark.com. The author is not responsible for any violations this application does to Grooveshark's TOS. The author is not related to Grooveshark™ in any way! Support the Artists You Like by Buying Their Music."));
    ui_->donateButton->setIcon(QIcon(":/resources/btn_donate_LG.gif"));
    ui_->donateButton->setIconSize(QSize(96,27));

    connect(ui_->gpl3Button, SIGNAL(clicked()), this, SLOT(openGplPage()));
    connect(ui_->donateButton, SIGNAL(clicked()), this, SLOT(openDonatePage()));

    setMinimumSize(620,400);
}

/*!
  \brief AboutDialog: this is the AboutDialog destructor.
*/
AboutDialog::~AboutDialog()
{

}

/*!
  \brief openGplPage: open GPL web page
  \return void
*/
void AboutDialog::openGplPage()
{
    QDesktopServices::openUrl(QUrl(QLatin1String("http://www.gnu.org/licenses/gpl.html")));
}

/*!
  \brief openDonatePage: open web page for donations
  \return void
*/
void AboutDialog::openDonatePage()
{
    QDesktopServices::openUrl(QUrl(QLatin1String("https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=WJNETV7GLTKDG&item_name=") + trUtf8("Donation to Grooveoff's author")));
}


#include "aboutdialog.moc"
