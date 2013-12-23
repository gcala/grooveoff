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


#include "configdialog.h"
#include "ui_configdialog.h"
#include "settingsitem.h"

#include <QSettings>

/*!
  \brief ConfigDialog: this is the ConfigDialog constructor.
  \param parent: The Parent Widget
*/
ConfigDialog::ConfigDialog(QWidget *parent):
    QDialog(parent),
    ui_(new Ui::ConfigDialog)
{
    ui_->setupUi(this);
    setupUi();

    // be sure that settings opens with general page
    ui_->stackedWidget->setCurrentIndex(0);

    // just started so this flag is false
    configChanged = false;

    ui_->historySize->setMinimum(1);
    ui_->historySize->setMaximum(10);

    // connect all checkboxes to the same slot
    QList<QCheckBox *> checkboxList = this->findChildren<QCheckBox *>();
    foreach(QCheckBox *cb, checkboxList) {
        connect(cb, SIGNAL(toggled(bool)), this, SLOT(cfgChanged()));
    }

    // connect all spinboxes to the same slot
    QList<QSpinBox *> spinboxList = this->findChildren<QSpinBox *>();
    foreach(QSpinBox *sb, spinboxList) {
        connect(sb, SIGNAL(valueChanged(int)), this, SLOT(cfgChanged()));
    }

    connect(ui_->cancelButton, SIGNAL(clicked()), this, SLOT(close()));
    connect(ui_->restoreButton, SIGNAL(clicked()), this, SLOT(restoreDefaults()));
    connect(ui_->applyButton, SIGNAL(clicked()), this, SLOT(saveSettings()));
    connect(ui_->okButton, SIGNAL(clicked()), this, SLOT(okClicked()));
    connect(ui_->contentsWidget, SIGNAL(currentRowChanged(int)), this, SLOT(switchPage(int)));

    loadSettings();

    ui_->applyButton->setEnabled(false);
}

/*!
  \brief setupUi: setup ui elements
  \return void
*/
void ConfigDialog::setupUi()
{
    ui_->cancelButton->setIcon(QIcon::fromTheme(QLatin1String("dialog-cancel"), QIcon(QLatin1String(":/resources/dialog-cancel.png"))));

    if(QIcon::hasThemeIcon(QLatin1String("dialog-ok-apply")))
        ui_->applyButton->setIcon(QIcon::fromTheme(QLatin1String("dialog-ok-apply")));
    else
        ui_->applyButton->setIcon(QIcon::fromTheme(QLatin1String("dialog-apply"), QIcon(QLatin1String(":/resources/dialog-ok-apply.png"))));

    ui_->okButton->setIcon(QIcon::fromTheme(QLatin1String("dialog-ok"), QIcon(":/resources/dialog-ok.png")));

    ui_->restoreButton->setIcon(QIcon::fromTheme(QLatin1String("document-revert"), QIcon(":/resources/document-revert.png")));

    QString generalString = trUtf8("General");
    QString performanceString = trUtf8("Performance");

    QFont boldFont;
    boldFont.setBold(true);
    QFontMetrics fmText(boldFont);

    // avoid that strings translation deform gui
    int maxWidth = 0;
    if(fmText.width(generalString) > maxWidth)
        maxWidth = fmText.width(generalString);
    if(fmText.width(performanceString) > maxWidth)
        maxWidth = fmText.width(performanceString);

    ui_->contentsWidget->setFixedWidth(maxWidth + 20);

    SettingsItem *generalItem = new SettingsItem(generalString, "", this); // empty icon name is default for application own icon
    SettingsItem *performanceItem = new SettingsItem(performanceString, QLatin1String("preferences-system-performance"), this);

    QListWidgetItem *generalWidget = new QListWidgetItem();
    ui_->contentsWidget->addItem(generalWidget);
    ui_->contentsWidget->setItemWidget(generalWidget, generalItem);
    generalWidget->setSizeHint(QSize(maxWidth,80));

    QListWidgetItem *performanceWidget = new QListWidgetItem();
    ui_->contentsWidget->addItem(performanceWidget);
    ui_->contentsWidget->setItemWidget(performanceWidget, performanceItem);
    performanceWidget->setSizeHint(QSize(maxWidth,80));

    ui_->generalPageIcon->setPixmap(QPixmap(QLatin1String(":/resources/grooveoff.png")));
    ui_->generalPageIcon->setScaledContents(true);

    if(QIcon::hasThemeIcon(QLatin1String("preferences-system-performance")))
        ui_->performancePageIcon->setPixmap(QIcon::fromTheme(QLatin1String("preferences-system-performance")).pixmap(22,22));
    else
        ui_->performancePageIcon->setPixmap(QIcon::fromTheme(QLatin1String("software-update-available"), QIcon(QLatin1String(":/resources/preferences-system-performance.png"))).pixmap(22,22));

    ui_->performancePageIcon->setScaledContents(true);

    ui_->contentsWidget->setCurrentItem(generalWidget);
}

/*!
  \brief restoreDefaults: restore default settings
  \return void
*/
void ConfigDialog::restoreDefaults()
{
    configChanged = true;
    ui_->applyButton->setEnabled(true);
    ui_->saveSearches->setChecked(false);
    ui_->saveSession->setChecked(false);
    ui_->historySize->setEnabled(false);
    ui_->saveDestination->setChecked(false);
    ui_->label_3->setEnabled(false);
    ui_->loadCovers->setChecked(true);
    ui_->numResults->setValue(0);
    ui_->maxDownloads->setValue(5);
}

/*!
  \brief saveSettings: save settings
  \return void
*/
void ConfigDialog::saveSettings()
{
    configChanged = false;
    ui_->applyButton->setEnabled(false);
    QSettings settings;
    settings.setValue(QLatin1String("saveSearches"), ui_->saveSearches->isChecked());
    settings.setValue(QLatin1String("saveSession"), ui_->saveSession->isChecked());
    settings.setValue(QLatin1String("historySize"), ui_->historySize->value());
    settings.setValue(QLatin1String("loadCovers"), ui_->loadCovers->isChecked());
    settings.setValue(QLatin1String("numResults"), ui_->numResults->value());
    settings.setValue(QLatin1String("maxDownloads"), ui_->maxDownloads->value());
    settings.setValue(QLatin1String("saveDestination"), ui_->saveDestination->isChecked());
}

/*!
  \brief okClicked: ok button clicked
  \return void
*/
void ConfigDialog::okClicked()
{
    if(configChanged)
        saveSettings();
    this->close();
}

/*!
  \brief switchPage: change page
  \param page: page index
  \return void
*/
void ConfigDialog::switchPage(int page)
{
    ui_->stackedWidget->setCurrentIndex(page);
}

/*!
  \brief cfgChanged: config changed slot
  \return void
*/
void ConfigDialog::cfgChanged()
{
    configChanged = true;
    ui_->applyButton->setEnabled(true);
    if(ui_->saveSearches->isChecked()) {
        ui_->historySize->setEnabled(true);
        ui_->label_3->setEnabled(true);
    } else {
        ui_->historySize->setEnabled(false);
        ui_->label_3->setEnabled(false);
    }
}

/*!
  \brief loadSettings: load settings
  \return void
*/
void ConfigDialog::loadSettings()
{
    QSettings settings;
    ui_->saveSession->setChecked(settings.value(QLatin1String("saveSession"),false).toBool());
    ui_->saveSearches->setChecked(settings.value(QLatin1String("saveSearches"),false).toBool());
    if(!ui_->saveSearches->isChecked()) {
        ui_->historySize->setEnabled(false);
        ui_->label_3->setEnabled(false);
    }
    ui_->historySize->setValue(settings.value(QLatin1String("historySize"), 0).toInt());
    ui_->loadCovers->setChecked(settings.value(QLatin1String("loadCovers"), true).toBool());
    ui_->numResults->setValue(settings.value(QLatin1String("numResults"), 0).toInt());
    ui_->maxDownloads->setValue(settings.value(QLatin1String("maxDownloads"), 5).toInt());
    ui_->saveDestination->setChecked(settings.value(QLatin1String("saveDestination"), false).toBool());
}

