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
#include "Settings.h"

#include "../torrent/CacheManager.h"

#include <QComboBox>
#include <QLineEdit>
#include <QLocale>
#include <QMessageBox>
#include <QSettings>

namespace {
    /** the order of the entries of the proxy combo, and of nothing else */
    enum ProxyRow {
        NoProxy = 0,
        Socks5Proxy = 1,
        HttpProxy = 2
    };
}

/*!
  \brief ConfigDialog: this is the ConfigDialog constructor.
  \param parent: The Parent Widget
*/
ConfigDialog::ConfigDialog( QWidget *parent )
    : QDialog( parent )
    , ui( new Ui::ConfigDialog )
    , configChanged( false )
{
    ui->setupUi( this );
    setupConnections();
    setupGraphicElements();
    setupSettingCategories();

    loadSettings();

    ui->applyButton->setEnabled( false );
    // be sure that settings opens with the first page
    ui->stackedWidget->setCurrentIndex( 0 );
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

    connect( ui->proxyType, &QComboBox::currentIndexChanged, this, &ConfigDialog::onConfigChanged );
    connect( ui->proxyHost, &QLineEdit::textEdited, this, &ConfigDialog::onConfigChanged );

    connect( ui->cancelButton, &QPushButton::clicked, this, &ConfigDialog::close );
    connect( ui->restoreButton, &QPushButton::clicked, this, &ConfigDialog::restoreDefaults );
    connect( ui->applyButton, &QPushButton::clicked, this, &ConfigDialog::saveSettings );
    connect( ui->okButton, &QPushButton::clicked, this, &ConfigDialog::okClicked );
    connect( ui->emptyCacheButton, &QPushButton::clicked, this, &ConfigDialog::emptyCacheNow );
    connect( ui->contentsWidget, &QListWidget::currentRowChanged, this, &ConfigDialog::switchPage );
}

void ConfigDialog::setupGraphicElements()
{
    ui->cancelButton->setIcon( QIcon::fromTheme( QLatin1String( "dialog-cancel" ), QIcon( QLatin1String( ":/resources/dialog-cancel.png " ) ) ) );
    ui->applyButton->setIcon( QIcon::fromTheme( QLatin1String( "dialog-ok-apply" ),
                                                QIcon::fromTheme( QLatin1String( "dialog-apply" ), QIcon( QLatin1String( ":/resources/dialog-ok-apply.png" ) ) ) ) );

    ui->okButton->setIcon( QIcon::fromTheme( QLatin1String( "dialog-ok" ), QIcon( ":/resources/dialog-ok.png" ) ) );
    ui->restoreButton->setIcon( QIcon::fromTheme( QLatin1String( "document-revert" ), QIcon( ":/resources/document-revert.png" ) ) );

    ui->playbackPageIcon->setPixmap( QPixmap( QLatin1String( ":/resources/grooveoff.png" ) ) );
    ui->playbackPageIcon->setScaledContents( true );

    ui->cachePageIcon->setPixmap( QIcon::fromTheme( QLatin1String( "drive-harddisk" ),
                                                    QIcon( QLatin1String( ":/resources/preferences-system-performance.png" ) ) ).pixmap( 22, 22 ) );
    ui->cachePageIcon->setScaledContents( true );

    ui->networkPageIcon->setPixmap( QIcon::fromTheme( QLatin1String( "network-workgroup" ),
                                                      QIcon( QLatin1String( ":/resources/preferences-system-performance.png" ) ) ).pixmap( 22, 22 ) );
    ui->networkPageIcon->setScaledContents( true );
}

void ConfigDialog::setupSettingCategories()
{
    const QString playbackString = tr( "Playback" );
    const QString cacheString    = tr( "Cache" );
    const QString networkString  = tr( "Network" );

    QFont boldFont;
    boldFont.setBold( true );
    QFontMetrics fmText( boldFont );

    // adapt column width to translated strings
    int maxWidth = qMax( fmText.horizontalAdvance( playbackString ),
                         qMax( fmText.horizontalAdvance( cacheString ),
                               fmText.horizontalAdvance( networkString ) ) );

    ui->contentsWidget->setFixedWidth( maxWidth + 20 /*margin*/ );

    // empty icon name is default for application own icon
    SettingsItem *playbackItem = new SettingsItem( playbackString, "", this );
    SettingsItem *cacheItem    = new SettingsItem( cacheString, QLatin1String( "drive-harddisk" ), this );
    SettingsItem *networkItem  = new SettingsItem( networkString, QLatin1String( "network-workgroup" ), this );

    const QList<SettingsItem *> items = { playbackItem, cacheItem, networkItem };

    QListWidgetItem *first = nullptr;
    for( SettingsItem *item : items ) {
        QListWidgetItem *widget = new QListWidgetItem();
        ui->contentsWidget->addItem( widget );
        ui->contentsWidget->setItemWidget( widget, item );
        widget->setSizeHint( QSize( maxWidth, 80 ) );

        if( !first )
            first = widget;
    }

    ui->contentsWidget->setCurrentItem( first );
}

/*!
  \brief restoreDefaults: restore default settings
  \return void
*/
void ConfigDialog::restoreDefaults()
{
    configChanged = true;
    ui->applyButton->setEnabled( true );

    ui->restorePlayback->setChecked( Settings::restorePlaybackDefault );
    ui->downloadAhead->setChecked( Settings::downloadAheadDefault );
    ui->bufferSeconds->setValue( Settings::bufferSecondsDefault );

    ui->cacheSize->setValue( Settings::cacheSizeDefault );
    ui->emptyCacheOnExit->setChecked( Settings::emptyCacheOnExitDefault );

    ui->listenPort->setValue( Settings::listenPortDefault );
    ui->streamPort->setValue( Settings::streamPortDefault );
    ui->downloadRateLimit->setValue( Settings::downloadRateLimitDefault );

    ui->sharing->setChecked( Settings::sharingDefault );
    ui->uploadSlots->setValue( Settings::uploadSlotsDefault );
    ui->uploadRateLimit->setValue( Settings::uploadRateLimitDefault );

    ui->proxyType->setCurrentIndex( NoProxy );
    ui->proxyHost->clear();
    ui->proxyPort->setValue( Settings::proxyPortDefault );

    updateEnabledState();
}

/*!
  \brief save settings
*/
void ConfigDialog::saveSettings()
{
    configChanged = false;
    ui->applyButton->setEnabled( false );

    QSettings settings;

    settings.setValue( Settings::restorePlayback, ui->restorePlayback->isChecked() );
    settings.setValue( Settings::downloadAhead, ui->downloadAhead->isChecked() );
    settings.setValue( Settings::bufferSeconds, ui->bufferSeconds->value() );

    settings.setValue( Settings::cacheSize, ui->cacheSize->value() );
    settings.setValue( Settings::emptyCacheOnExit, ui->emptyCacheOnExit->isChecked() );

    settings.setValue( Settings::listenPort, ui->listenPort->value() );
    settings.setValue( Settings::streamPort, ui->streamPort->value() );
    settings.setValue( Settings::downloadRateLimit, ui->downloadRateLimit->value() );

    settings.setValue( Settings::sharing, ui->sharing->isChecked() );
    settings.setValue( Settings::uploadSlots, ui->uploadSlots->value() );
    settings.setValue( Settings::uploadRateLimit, ui->uploadRateLimit->value() );

    QString proxyType = Settings::proxyTypeNone;
    if( ui->proxyType->currentIndex() == Socks5Proxy )
        proxyType = Settings::proxyTypeSocks5;
    else if( ui->proxyType->currentIndex() == HttpProxy )
        proxyType = Settings::proxyTypeHttp;

    settings.setValue( Settings::proxyType, proxyType );
    settings.setValue( Settings::proxyHost, ui->proxyHost->text().trimmed() );
    settings.setValue( Settings::proxyPort, ui->proxyPort->value() );

    // A cache ceiling that has just come down means something has to go, and it
    // has to go now rather than at the next check: the user lowered it because
    // they want the space back.
    The::cacheManager()->enforceLimit();
    showCacheUsage();
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

    updateEnabledState();
}

void ConfigDialog::updateEnabledState()
{
    const bool proxied = ui->proxyType->currentIndex() != NoProxy;

    ui->proxyHostLabel->setEnabled( proxied );
    ui->proxyHost->setEnabled( proxied );
    ui->proxyPortLabel->setEnabled( proxied );
    ui->proxyPort->setEnabled( proxied );

    ui->downloadAheadHint->setEnabled( ui->downloadAhead->isChecked() );

    // Neither number means anything with the sharing off, and greyed rather
    // than hidden they say what turning it on is going to do.
    const bool sharing = ui->sharing->isChecked();
    ui->sharingHint->setEnabled( sharing );
    ui->uploadSlotsLabel->setEnabled( sharing );
    ui->uploadSlots->setEnabled( sharing );
    ui->uploadRateLabel->setEnabled( sharing );
    ui->uploadRateLimit->setEnabled( sharing );
}

/*!
  \brief emptyCacheNow: the bytes go, the catalogue stays

  Worth confirming even though nothing is lost for good, because "empty the
  cache" reads like something irreversible; the message says what it really
  costs, which is fetching those tracks again.
*/
void ConfigDialog::emptyCacheNow()
{
    const int answer = QMessageBox::question( this,
                                              tr( "Empty the cache" ),
                                              tr( "Delete the downloaded data of every torrent?" )
                                                  + QLatin1String( "\n\n" )
                                                  + tr( "Your playlists are not touched: the tracks will "
                                                        "be fetched again the next time you play them." ),
                                              QMessageBox::Yes | QMessageBox::No,
                                              QMessageBox::No );

    if( answer != QMessageBox::Yes )
        return;

    The::cacheManager()->emptyCache();
    showCacheUsage();
}

void ConfigDialog::showCacheUsage()
{
    const qint64 used = The::cacheManager()->usage();

    ui->cacheUsage->setText( tr( "In the cache now: %1" ).arg( QLocale().formattedDataSize( used ) ) );
}

/*!
  \brief loadSettings: load settings
  \return void
*/
void ConfigDialog::loadSettings()
{
    QSettings settings;

    ui->restorePlayback->setChecked( settings.value( Settings::restorePlayback, Settings::restorePlaybackDefault ).toBool() );
    ui->downloadAhead->setChecked( settings.value( Settings::downloadAhead, Settings::downloadAheadDefault ).toBool() );
    ui->bufferSeconds->setValue( settings.value( Settings::bufferSeconds, Settings::bufferSecondsDefault ).toInt() );

    ui->cacheSize->setValue( settings.value( Settings::cacheSize, Settings::cacheSizeDefault ).toInt() );
    ui->emptyCacheOnExit->setChecked( settings.value( Settings::emptyCacheOnExit, Settings::emptyCacheOnExitDefault ).toBool() );

    ui->listenPort->setValue( settings.value( Settings::listenPort, Settings::listenPortDefault ).toInt() );
    ui->streamPort->setValue( settings.value( Settings::streamPort, Settings::streamPortDefault ).toInt() );
    ui->downloadRateLimit->setValue( settings.value( Settings::downloadRateLimit, Settings::downloadRateLimitDefault ).toInt() );

    ui->sharing->setChecked( settings.value( Settings::sharing, Settings::sharingDefault ).toBool() );
    ui->uploadSlots->setValue( settings.value( Settings::uploadSlots, Settings::uploadSlotsDefault ).toInt() );
    ui->uploadRateLimit->setValue( settings.value( Settings::uploadRateLimit, Settings::uploadRateLimitDefault ).toInt() );

    const QString proxyType = settings.value( Settings::proxyType, Settings::proxyTypeNone ).toString();
    if( proxyType == Settings::proxyTypeSocks5 )
        ui->proxyType->setCurrentIndex( Socks5Proxy );
    else if( proxyType == Settings::proxyTypeHttp )
        ui->proxyType->setCurrentIndex( HttpProxy );
    else
        ui->proxyType->setCurrentIndex( NoProxy );

    ui->proxyHost->setText( settings.value( Settings::proxyHost ).toString() );
    ui->proxyPort->setValue( settings.value( Settings::proxyPort, Settings::proxyPortDefault ).toInt() );

    updateEnabledState();
    showCacheUsage();
}
