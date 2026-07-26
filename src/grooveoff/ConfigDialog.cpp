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
ConfigDialog::ConfigDialog( QWidget *parent )
    : QDialog( parent )
    , ui( new Ui::ConfigDialog )
    , configChanged(false)
{
    ui->setupUi( this );
    setupConnections();
    setupGraphicElements();
    setupSettingCategories();
    setupTagElements();

    loadSettings();

    ui->applyButton->setEnabled( false );
    // be sure that settings opens with general page
    ui->stackedWidget->setCurrentIndex( 0 );
    
    ui->historySize->setMinimum( 1 );
    ui->historySize->setMaximum( 10 );
}

void ConfigDialog::setupConnections()
{
    // connect all checkboxes to the same slot
    const QList< QCheckBox * > checkboxList = this->findChildren< QCheckBox * >();
    for( QCheckBox *checkbox : checkboxList ) {
        connect( checkbox, &QCheckBox::toggled, this, &ConfigDialog::onConfigChanged );
    }

    // connect all spinboxes to the same slot
    const QList< QSpinBox * > spinboxList = this->findChildren< QSpinBox * >();
    for( QSpinBox *spinbox : spinboxList ) {
        connect( spinbox, &QSpinBox::valueChanged, this, &ConfigDialog::onConfigChanged );
    }

    connect( ui->cancelButton, &QPushButton::clicked, this, &ConfigDialog::close );
    connect( ui->restoreButton, &QPushButton::clicked, this, &ConfigDialog::restoreDefaults );
    connect( ui->applyButton, &QPushButton::clicked, this, &ConfigDialog::saveSettings );
    connect( ui->okButton, &QPushButton::clicked, this, &ConfigDialog::okClicked );
    connect( ui->contentsWidget, &QListWidget::currentRowChanged, this, &ConfigDialog::switchPage );
}

void ConfigDialog::setupGraphicElements()
{
    ui->cancelButton->setIcon( QIcon::fromTheme( QLatin1String( "dialog-cancel" ), QIcon( QLatin1String( ":/resources/dialog-cancel.png " ) ) ) );
    ui->applyButton->setIcon( QIcon::fromTheme( QLatin1String( "dialog-ok-apply" ),
                                                QIcon::fromTheme( QLatin1String( "dialog-apply" ), QIcon( QLatin1String( ":/resources/dialog-ok-apply.png" ) ) ) ) );

    ui->okButton->setIcon( QIcon::fromTheme( QLatin1String( "dialog-ok" ), QIcon( ":/resources/dialog-ok.png" ) ) );
    ui->restoreButton->setIcon( QIcon::fromTheme( QLatin1String( "document-revert" ), QIcon( ":/resources/document-revert.png" ) ) );
    
    ui->generalPageIcon->setPixmap( QPixmap( QLatin1String( ":/resources/grooveoff.png" ) ) );
    ui->generalPageIcon->setScaledContents(true);

    ui->performancePageIcon->setPixmap( QIcon::fromTheme( QLatin1String( "preferences-system-performance" ), 
                                                          QIcon::fromTheme( QLatin1String( "software-update-available" ), QIcon( QLatin1String( ":/resources/preferences-system-performance.png" ) ) ) ).pixmap( 22,22 ) );
    ui->performancePageIcon->setScaledContents( true );
}

void ConfigDialog::setupSettingCategories()
{
    QString generalString = tr( "General" );
    QString performanceString = tr( "Performance" );

    QFont boldFont;
    boldFont.setBold( true );
    QFontMetrics fmText( boldFont );

    // adapat column width to translated strings
    int maxWidth = qMax(fmText.horizontalAdvance( generalString ), fmText.horizontalAdvance( performanceString ));

    ui->contentsWidget->setFixedWidth( maxWidth + 20 /*margin*/ );
    
    SettingsItem *generalItem = new SettingsItem( generalString, "", this ); // empty icon name is default for application own icon
    SettingsItem *performanceItem = new SettingsItem( performanceString, QLatin1String( "preferences-system-performance" ), this );

    QListWidgetItem *generalWidget = new QListWidgetItem();
    ui->contentsWidget->addItem( generalWidget );
    ui->contentsWidget->setItemWidget( generalWidget, generalItem );
    generalWidget->setSizeHint( QSize( maxWidth,80 ) );

    QListWidgetItem *performanceWidget = new QListWidgetItem();
    ui->contentsWidget->addItem( performanceWidget );
    ui->contentsWidget->setItemWidget( performanceWidget, performanceItem );
    performanceWidget->setSizeHint( QSize(maxWidth,80 ) );

    ui->contentsWidget->setCurrentItem( generalWidget );
}


void ConfigDialog::setupTagElements()
{
    m_tagNames << QLatin1String( "%title" ) 
               << QLatin1String( "%artist" )
               << QLatin1String( "%album" )
               << QLatin1String( "%track" );
    // xgettext: no-c-format
    m_localizedTagNames << tr( "%title" )
                        // xgettext: no-c-format
                        << tr( "%artist" )
                        // xgettext: no-c-format
                        << tr( "%album" )
                        // xgettext: no-c-format
                        << tr( "%track" );
    // xgettext: no-c-format
    m_localizedTimeTagName = tr("%time" );

    QStringList itemsIcons;
    itemsIcons << QLatin1String( "view-media-lyrics" )   //%title
               << QLatin1String( "view-media-artist" )   //%artist
               << QLatin1String( "view-media-playlist" ) //%album
               << QLatin1String( "media-optical" );      //%track

}


/*!
  \brief restoreDefaults: restore default settings
  \return void
*/
void ConfigDialog::restoreDefaults()
{
    configChanged = true;
    ui->applyButton->setEnabled( true );
    ui->saveSearches->setChecked( false );
    ui->saveSession->setChecked( true );
    ui->historySize->setEnabled( false );
    ui->saveDestination->setChecked( true );
    ui->label_3->setEnabled( false );
    ui->loadCovers->setChecked( true );
    ui->emptyCache->setChecked( false );
    ui->saveAborted->setChecked( false );
    ui->numResults->setValue( 0 );
    ui->maxDownloads->setValue( 5 );
}

/*!
  \brief save settings
*/
void ConfigDialog::saveSettings()
{
    configChanged = false;
    ui->applyButton->setEnabled( false );
    QSettings settings;
    settings.setValue( QLatin1String( "saveSearches" ), ui->saveSearches->isChecked() );
    settings.setValue( QLatin1String( "saveSession" ), ui->saveSession->isChecked() );
    if( ui->saveSession->isChecked() )
        settings.setValue( QLatin1String( "saveAborted" ), ui->saveAborted->isChecked() );
    else
        settings.setValue( QLatin1String("saveAborted" ), false );
    settings.setValue( QLatin1String( "historySize" ), ui->historySize->value() );
    settings.setValue( QLatin1String( "loadCovers" ), ui->loadCovers->isChecked() );
    if( ui->loadCovers->isChecked() )
        settings.setValue( QLatin1String("emptyCache"), ui->emptyCache->isChecked() );
    else
        settings.setValue( QLatin1String( "emptyCache" ), true );
    settings.setValue( QLatin1String( "numResults" ), ui->numResults->value() );
    settings.setValue( QLatin1String( "maxDownloads" ), ui->maxDownloads->value() );
    settings.setValue( QLatin1String( "saveDestination" ), ui->saveDestination->isChecked() );
}

/*!
  \brief ok button clicked
*/
void ConfigDialog::okClicked()
{
    if( configChanged )
        saveSettings();
    close();
}

/*!
  \brief switchPage: change page
*/
void ConfigDialog::switchPage( int page )
{
    ui->stackedWidget->setCurrentIndex( page );
}

/*!
  \brief config changed slot
*/
void ConfigDialog::onConfigChanged()
{
    configChanged = true;
    ui->applyButton->setEnabled( true );
    if( ui->saveSearches->isChecked() ) {
        ui->historySize->setEnabled( true );
        ui->label_3->setEnabled( true );
    } else {
        ui->historySize->setEnabled( false );
        ui->label_3->setEnabled( false );
    }
    
    ui->emptyCache->setEnabled( ui->loadCovers->isChecked() );
    ui->saveAborted->setEnabled( ui->saveSession->isChecked() );
}

/*!
  \brief loadSettings: load settings
  \return void
*/
void ConfigDialog::loadSettings()
{
    QSettings settings;
    ui->saveSession->setChecked( settings.value(QLatin1String( "saveSession" ), true ).toBool() );
    
    if( ui->saveSession->isChecked() ) {
        ui->saveAborted->setEnabled( true );
        ui->saveAborted->setChecked( settings.value( QLatin1String( "saveAborted" ), false ).toBool() );
    } else
        ui->saveAborted->setEnabled( false );
    
    ui->saveSearches->setChecked( settings.value( QLatin1String( "saveSearches" ),false ).toBool() );
    
    if( !ui->saveSearches->isChecked() ) {
        ui->historySize->setEnabled( false );
        ui->label_3->setEnabled( false );
    }
    
    ui->historySize->setValue( settings.value( QLatin1String( "historySize" ), 0 ).toInt() );
    ui->loadCovers->setChecked( settings.value( QLatin1String( "loadCovers" ), true ).toBool() );
    
    if( ui->loadCovers->isChecked() ) {
        ui->emptyCache->setEnabled( true );
        ui->emptyCache->setChecked( settings.value( QLatin1String( "emptyCache" ), false ).toBool() );
    } else
        ui->emptyCache->setEnabled( false );
    
    ui->numResults->setValue( settings.value( QLatin1String( "numResults" ), 0 ).toInt() );
    ui->maxDownloads->setValue( settings.value( QLatin1String( "maxDownloads" ), 5 ).toInt() );
    ui->saveDestination->setChecked( settings.value( QLatin1String( "saveDestination" ), true ).toBool() );

    //Naming Schema
}
