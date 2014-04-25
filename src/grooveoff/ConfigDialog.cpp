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


#include "ConfigDialog.h"
#include "ui_ConfigDialog.h"
#include "SettingsItem.h"

#include <QSettings>

/*!
  \brief ConfigDialog: this is the ConfigDialog constructor.
  \param parent: The Parent Widget
*/
ConfigDialog::ConfigDialog(QWidget *parent):
    QDialog(parent),
    ui(new Ui::ConfigDialog)
{
    ui->setupUi(this);
    setupUi();

    // be sure that settings opens with general page
    ui->stackedWidget->setCurrentIndex(0);

    // just started so this flag is false
    configChanged = false;

    ui->historySize->setMinimum(1);
    ui->historySize->setMaximum(10);

    // connect all checkboxes to the same slot
    QList<QCheckBox *> checkboxList = this->findChildren<QCheckBox *>();
    foreach(QCheckBox *cb, checkboxList) {
        connect(cb, SIGNAL(toggled(bool)),
                    SLOT(cfgChanged()));
    }

    // connect all spinboxes to the same slot
    QList<QSpinBox *> spinboxList = this->findChildren<QSpinBox *>();
    foreach(QSpinBox *sb, spinboxList) {
        connect(sb, SIGNAL(valueChanged(int)),
                    SLOT(cfgChanged()));
    }

    connect(ui->m_nowPlayingText, SIGNAL(textChanged(QString)),
                                  SLOT(cfgChanged()));

    m_tagNames << QLatin1String("%title") << QLatin1String("%artist") << QLatin1String("%album") << QLatin1String("%track");
    // xgettext: no-c-format
    m_localizedTagNames << trUtf8("%title")
                        // xgettext: no-c-format
                        << trUtf8("%artist")
                        // xgettext: no-c-format
                        << trUtf8("%album")
                        // xgettext: no-c-format
                        << trUtf8("%track");
    // xgettext: no-c-format
    m_localizedTimeTagName = trUtf8("%time");

    QStringList itemsIcons;
    itemsIcons << QLatin1String("view-media-lyrics")   //%title
               << QLatin1String("view-media-artist")   //%artist
               << QLatin1String("view-media-playlist") //%album
               << QLatin1String("mixer-cd");           //%track

    ui->m_tagListWidget->setItemsIcons(itemsIcons);
    ui->m_tagListWidget->setLocalizedTagNames(m_localizedTagNames);
    ui->m_tagListWidget->setupItems(); //populate the list, load items icons and set list's maximum size

    ui->m_nowPlayingText->setLocalizedTagNames(m_localizedTagNames);

    connect(ui->cancelButton, SIGNAL(clicked()),
                              SLOT(close()));
    
    connect(ui->restoreButton, SIGNAL(clicked()),
                               SLOT(restoreDefaults()));
    
    connect(ui->applyButton, SIGNAL(clicked()),
                             SLOT(saveSettings()));
    
    connect(ui->okButton, SIGNAL(clicked()),
                          SLOT(okClicked()));
    
    connect(ui->contentsWidget, SIGNAL(currentRowChanged(int)),
                                SLOT(switchPage(int)));

    loadSettings();

    ui->applyButton->setEnabled(false);
}

/*!
  \brief setupUi: setup ui elements
  \return void
*/
void ConfigDialog::setupUi()
{
    ui->cancelButton->setIcon(QIcon::fromTheme(QLatin1String("dialog-cancel"), QIcon(QLatin1String(":/resources/dialog-cancel.png"))));

    if(QIcon::hasThemeIcon(QLatin1String("dialog-ok-apply")))
        ui->applyButton->setIcon(QIcon::fromTheme(QLatin1String("dialog-ok-apply")));
    else
        ui->applyButton->setIcon(QIcon::fromTheme(QLatin1String("dialog-apply"), QIcon(QLatin1String(":/resources/dialog-ok-apply.png"))));

    ui->okButton->setIcon(QIcon::fromTheme(QLatin1String("dialog-ok"), QIcon(":/resources/dialog-ok.png")));

    ui->restoreButton->setIcon(QIcon::fromTheme(QLatin1String("document-revert"), QIcon(":/resources/document-revert.png")));

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

    ui->contentsWidget->setFixedWidth(maxWidth + 20);

    SettingsItem *generalItem = new SettingsItem(generalString, "", this); // empty icon name is default for application own icon
    SettingsItem *performanceItem = new SettingsItem(performanceString, QLatin1String("preferences-system-performance"), this);

    QListWidgetItem *generalWidget = new QListWidgetItem();
    ui->contentsWidget->addItem(generalWidget);
    ui->contentsWidget->setItemWidget(generalWidget, generalItem);
    generalWidget->setSizeHint(QSize(maxWidth,80));

    QListWidgetItem *performanceWidget = new QListWidgetItem();
    ui->contentsWidget->addItem(performanceWidget);
    ui->contentsWidget->setItemWidget(performanceWidget, performanceItem);
    performanceWidget->setSizeHint(QSize(maxWidth,80));

    ui->generalPageIcon->setPixmap(QPixmap(QLatin1String(":/resources/grooveoff.png")));
    ui->generalPageIcon->setScaledContents(true);

    if(QIcon::hasThemeIcon(QLatin1String("preferences-system-performance")))
        ui->performancePageIcon->setPixmap(QIcon::fromTheme(QLatin1String("preferences-system-performance")).pixmap(22,22));
    else
        ui->performancePageIcon->setPixmap(QIcon::fromTheme(QLatin1String("software-update-available"), QIcon(QLatin1String(":/resources/preferences-system-performance.png"))).pixmap(22,22));

    ui->performancePageIcon->setScaledContents(true);

    ui->contentsWidget->setCurrentItem(generalWidget);
}

/*!
  \brief restoreDefaults: restore default settings
  \return void
*/
void ConfigDialog::restoreDefaults()
{
    configChanged = true;
    ui->applyButton->setEnabled(true);
    ui->saveSearches->setChecked(false);
    ui->saveSession->setChecked(false);
    ui->historySize->setEnabled(false);
    ui->saveDestination->setChecked(false);
    ui->label_3->setEnabled(false);
    ui->loadCovers->setChecked(true);
    ui->emptyCache->setChecked(false);
    ui->numResults->setValue(0);
    ui->maxDownloads->setValue(5);
    ui->m_nowPlayingText->setText(trUtf8("%1 - %2").arg(QLatin1String("%artist")).arg(QLatin1String("%title")));
}

/*!
  \brief saveSettings: save settings
  \return void
*/
void ConfigDialog::saveSettings()
{
    configChanged = false;
    ui->applyButton->setEnabled(false);
    QSettings settings;
    settings.setValue(QLatin1String("saveSearches"), ui->saveSearches->isChecked());
    settings.setValue(QLatin1String("saveSession"), ui->saveSession->isChecked());
    settings.setValue(QLatin1String("historySize"), ui->historySize->value());
    settings.setValue(QLatin1String("loadCovers"), ui->loadCovers->isChecked());
    if(ui->loadCovers->isChecked())
        settings.setValue(QLatin1String("emptyCache"), ui->emptyCache->isChecked());
    else
        settings.setValue(QLatin1String("emptyCache"), true);
    settings.setValue(QLatin1String("numResults"), ui->numResults->value());
    settings.setValue(QLatin1String("maxDownloads"), ui->maxDownloads->value());
    settings.setValue(QLatin1String("saveDestination"), ui->saveDestination->isChecked());

    //we store a nowPlayingText version with untranslated tag names
    QString modifiedNamingSchema = ui->m_nowPlayingText->text();
    for (int i = 0; i < m_tagNames.size(); i++) {
        modifiedNamingSchema.replace(m_localizedTagNames.at(i), m_tagNames.at(i));
    }

    settings.setValue(QLatin1String("namingSchema"), modifiedNamingSchema);
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
    ui->stackedWidget->setCurrentIndex(page);
}

/*!
  \brief cfgChanged: config changed slot
  \return void
*/
void ConfigDialog::cfgChanged()
{
    configChanged = true;
    ui->applyButton->setEnabled(true);
    if(ui->saveSearches->isChecked()) {
        ui->historySize->setEnabled(true);
        ui->label_3->setEnabled(true);
    } else {
        ui->historySize->setEnabled(false);
        ui->label_3->setEnabled(false);
    }
    
    ui->emptyCache->setEnabled(ui->loadCovers->isChecked());
}

/*!
  \brief loadSettings: load settings
  \return void
*/
void ConfigDialog::loadSettings()
{
    QSettings settings;
    ui->saveSession->setChecked(settings.value(QLatin1String("saveSession"),false).toBool());
    ui->saveSearches->setChecked(settings.value(QLatin1String("saveSearches"),false).toBool());
    if(!ui->saveSearches->isChecked()) {
        ui->historySize->setEnabled(false);
        ui->label_3->setEnabled(false);
    }
    ui->historySize->setValue(settings.value(QLatin1String("historySize"), 0).toInt());
    ui->loadCovers->setChecked(settings.value(QLatin1String("loadCovers"), true).toBool());
    if(ui->loadCovers->isChecked()) {
        ui->emptyCache->setEnabled(true);
        ui->emptyCache->setChecked(settings.value(QLatin1String("emptyCache"), false).toBool());
    } else
        ui->emptyCache->setEnabled(false);
    ui->numResults->setValue(settings.value(QLatin1String("numResults"), 0).toInt());
    ui->maxDownloads->setValue(settings.value(QLatin1String("maxDownloads"), 5).toInt());
    ui->saveDestination->setChecked(settings.value(QLatin1String("saveDestination"), false).toBool());

    //Naming Schema
    QString namingSchemaText = settings.value(QLatin1String("namingSchema"),
                                              trUtf8("%1 - %2").arg(QLatin1String("%artist")).arg(QLatin1String("%title"))).toString();

    //in namingSchemaText tag names aren't localized, here they're replaced with the localized ones
    for (int i = 0; i < m_tagNames.size(); i++) {
        namingSchemaText.replace(m_tagNames.at(i), m_localizedTagNames.at(i));
    }

    ui->m_nowPlayingText->setText(namingSchemaText);
}
