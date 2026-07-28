/*
    GrooveOff - Music streaming from torrents
    Copyright (C) 2026  Giuseppe Calà <jiveaxe@gmail.com>

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
#include "Settings.h"

#include <QCheckBox>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QSettings>
#include <QSpinBox>
#include <QStandardPaths>
#include <QtTest>

/**
  * @class ConfigDialogTest
  * @short That what the dialog shows and what the rest of the code reads are
  *        the same thing.
  *
  * The bug this is here for does not look like a bug: an option whose default
  * in the dialog has drifted apart from the default its reader uses works
  * perfectly until somebody opens the settings, at which point the value
  * silently becomes something else. Nothing in the build catches it, and the
  * report that comes back is "it changed on its own".
  *
  * So the dialog is opened with nothing stored and asked what it shows, and
  * the answer is compared against Settings.h — the same constants the session,
  * the track and the player read.
  */
class ConfigDialogTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void init();

    void showsTheSameDefaultsTheCodeReads();
    void writesWhatWasChanged();
    void readsBackWhatWasWritten();
    void restoresTheDefaults();
    void greysTheProxyOutWhenThereIsNone();

private:
    template <typename T>
    static T* widget( QDialog *dialog, const QLatin1String &name )
    {
        T *found = dialog->findChild<T *>( QString( name ) );
        Q_ASSERT( found );
        return found;
    }
};

void ConfigDialogTest::initTestCase()
{
    QStandardPaths::setTestModeEnabled( true );
    QCoreApplication::setOrganizationName( QStringLiteral( "gcala" ) );
    QCoreApplication::setApplicationName( QStringLiteral( "grooveoff-configtest" ) );
}

void ConfigDialogTest::init()
{
    // Each function starts from a settings file that says nothing, except the
    // two that are meant to read what the one before them wrote.
    if( QTest::currentTestFunction() == QLatin1String( "readsBackWhatWasWritten" ) )
        return;

    QSettings settings;
    settings.clear();
    settings.sync();
}

void ConfigDialogTest::showsTheSameDefaultsTheCodeReads()
{
    ConfigDialog dialog;

    QCOMPARE( widget<QCheckBox>( &dialog, Settings::restorePlayback )->isChecked(), Settings::restorePlaybackDefault );
    QCOMPARE( widget<QCheckBox>( &dialog, Settings::downloadAhead )->isChecked(), Settings::downloadAheadDefault );
    QCOMPARE( widget<QSpinBox>( &dialog, Settings::bufferSeconds )->value(), Settings::bufferSecondsDefault );

    QCOMPARE( widget<QSpinBox>( &dialog, Settings::cacheSize )->value(), Settings::cacheSizeDefault );
    QCOMPARE( widget<QCheckBox>( &dialog, Settings::emptyCacheOnExit )->isChecked(), Settings::emptyCacheOnExitDefault );

    QCOMPARE( widget<QSpinBox>( &dialog, Settings::listenPort )->value(), Settings::listenPortDefault );
    QCOMPARE( widget<QSpinBox>( &dialog, Settings::streamPort )->value(), Settings::streamPortDefault );
    QCOMPARE( widget<QSpinBox>( &dialog, Settings::downloadRateLimit )->value(), Settings::downloadRateLimitDefault );

    QCOMPARE( widget<QCheckBox>( &dialog, Settings::sharing )->isChecked(), Settings::sharingDefault );
    QCOMPARE( widget<QSpinBox>( &dialog, Settings::uploadSlots )->value(), Settings::uploadSlotsDefault );
    QCOMPARE( widget<QSpinBox>( &dialog, Settings::uploadRateLimit )->value(), Settings::uploadRateLimitDefault );

    QCOMPARE( widget<QComboBox>( &dialog, Settings::proxyType )->currentIndex(), 0 );
    QVERIFY( widget<QLineEdit>( &dialog, Settings::proxyHost )->text().isEmpty() );
    QCOMPARE( widget<QSpinBox>( &dialog, Settings::proxyPort )->value(), Settings::proxyPortDefault );

    // Zero is a value here, not an empty field: no ceiling and no fixed port
    // both mean something, and a spin box that showed nothing would hide it.
    QVERIFY( !widget<QSpinBox>( &dialog, Settings::cacheSize )->specialValueText().isEmpty() );
    QVERIFY( !widget<QSpinBox>( &dialog, Settings::streamPort )->specialValueText().isEmpty() );
    QVERIFY( !widget<QSpinBox>( &dialog, Settings::downloadRateLimit )->specialValueText().isEmpty() );

    // Sharing is the other way round: the checkbox is what says yes or no, so
    // the count beside it never has to mean nobody and starts at one.
    QCOMPARE( widget<QSpinBox>( &dialog, Settings::uploadSlots )->minimum(), 1 );
}

void ConfigDialogTest::writesWhatWasChanged()
{
    ConfigDialog dialog;

    widget<QCheckBox>( &dialog, Settings::restorePlayback )->setChecked( false );
    widget<QCheckBox>( &dialog, Settings::downloadAhead )->setChecked( false );
    widget<QSpinBox>( &dialog, Settings::bufferSeconds )->setValue( 25 );
    widget<QSpinBox>( &dialog, Settings::cacheSize )->setValue( 512 );
    widget<QCheckBox>( &dialog, Settings::emptyCacheOnExit )->setChecked( true );
    widget<QSpinBox>( &dialog, Settings::listenPort )->setValue( 51413 );
    widget<QSpinBox>( &dialog, Settings::streamPort )->setValue( 8123 );
    widget<QSpinBox>( &dialog, Settings::downloadRateLimit )->setValue( 300 );
    widget<QCheckBox>( &dialog, Settings::sharing )->setChecked( true );
    widget<QSpinBox>( &dialog, Settings::uploadSlots )->setValue( 2 );
    widget<QSpinBox>( &dialog, Settings::uploadRateLimit )->setValue( 30 );
    widget<QComboBox>( &dialog, Settings::proxyType )->setCurrentIndex( 1 );
    widget<QLineEdit>( &dialog, Settings::proxyHost )->setText( QStringLiteral( "127.0.0.1" ) );
    widget<QSpinBox>( &dialog, Settings::proxyPort )->setValue( 9050 );

    QPushButton *apply = widget<QPushButton>( &dialog, QLatin1String( "applyButton" ) );
    QVERIFY2( apply->isEnabled(), "changing something did not enable Apply" );
    apply->click();

    QSettings settings;
    QCOMPARE( settings.value( Settings::restorePlayback ).toBool(), false );
    QCOMPARE( settings.value( Settings::downloadAhead ).toBool(), false );
    QCOMPARE( settings.value( Settings::bufferSeconds ).toInt(), 25 );
    QCOMPARE( settings.value( Settings::cacheSize ).toInt(), 512 );
    QCOMPARE( settings.value( Settings::emptyCacheOnExit ).toBool(), true );
    QCOMPARE( settings.value( Settings::listenPort ).toInt(), 51413 );
    QCOMPARE( settings.value( Settings::streamPort ).toInt(), 8123 );
    QCOMPARE( settings.value( Settings::downloadRateLimit ).toInt(), 300 );
    QCOMPARE( settings.value( Settings::sharing ).toBool(), true );
    QCOMPARE( settings.value( Settings::uploadSlots ).toInt(), 2 );
    QCOMPARE( settings.value( Settings::uploadRateLimit ).toInt(), 30 );
    QCOMPARE( settings.value( Settings::proxyType ).toString(), QString( Settings::proxyTypeSocks5 ) );
    QCOMPARE( settings.value( Settings::proxyHost ).toString(), QStringLiteral( "127.0.0.1" ) );
    QCOMPARE( settings.value( Settings::proxyPort ).toInt(), 9050 );

    QVERIFY( !apply->isEnabled() );
}

void ConfigDialogTest::readsBackWhatWasWritten()
{
    ConfigDialog dialog;

    QCOMPARE( widget<QCheckBox>( &dialog, Settings::downloadAhead )->isChecked(), false );
    QCOMPARE( widget<QSpinBox>( &dialog, Settings::bufferSeconds )->value(), 25 );
    QCOMPARE( widget<QSpinBox>( &dialog, Settings::cacheSize )->value(), 512 );
    QCOMPARE( widget<QComboBox>( &dialog, Settings::proxyType )->currentIndex(), 1 );
    QCOMPARE( widget<QLineEdit>( &dialog, Settings::proxyHost )->text(), QStringLiteral( "127.0.0.1" ) );
    QCOMPARE( widget<QCheckBox>( &dialog, Settings::sharing )->isChecked(), true );
    QCOMPARE( widget<QSpinBox>( &dialog, Settings::uploadSlots )->value(), 2 );

    // Nothing was touched, so there is nothing to apply.
    QVERIFY( !widget<QPushButton>( &dialog, QLatin1String( "applyButton" ) )->isEnabled() );
}

void ConfigDialogTest::restoresTheDefaults()
{
    QSettings settings;
    settings.setValue( Settings::cacheSize, 128 );
    settings.setValue( Settings::downloadAhead, false );
    settings.sync();

    ConfigDialog dialog;
    QCOMPARE( widget<QSpinBox>( &dialog, Settings::cacheSize )->value(), 128 );

    widget<QPushButton>( &dialog, QLatin1String( "restoreButton" ) )->click();

    QCOMPARE( widget<QSpinBox>( &dialog, Settings::cacheSize )->value(), Settings::cacheSizeDefault );
    QCOMPARE( widget<QCheckBox>( &dialog, Settings::downloadAhead )->isChecked(), Settings::downloadAheadDefault );

    // Restoring fills the fields in; it is Apply that writes them.
    QCOMPARE( settings.value( Settings::cacheSize ).toInt(), 128 );
}

void ConfigDialogTest::greysTheProxyOutWhenThereIsNone()
{
    ConfigDialog dialog;

    QComboBox *type = widget<QComboBox>( &dialog, Settings::proxyType );
    QLineEdit *host = widget<QLineEdit>( &dialog, Settings::proxyHost );
    QSpinBox  *port = widget<QSpinBox>( &dialog, Settings::proxyPort );

    QCOMPARE( type->currentIndex(), 0 );
    QVERIFY( !host->isEnabled() );
    QVERIFY( !port->isEnabled() );

    type->setCurrentIndex( 1 );
    QVERIFY( host->isEnabled() );
    QVERIFY( port->isEnabled() );

    type->setCurrentIndex( 0 );
    QVERIFY( !host->isEnabled() );
    QVERIFY( !port->isEnabled() );

    // The same rule under the sharing checkbox: both numbers mean nothing
    // while it is off, and a field that stays lit invites setting it.
    // slotBox and not slots: "slots" is a Qt keyword, and a variable by that
    // name is eaten by the preprocessor.
    QCheckBox *share  = widget<QCheckBox>( &dialog, Settings::sharing );
    QSpinBox *slotBox = widget<QSpinBox>( &dialog, Settings::uploadSlots );
    QSpinBox *limit   = widget<QSpinBox>( &dialog, Settings::uploadRateLimit );

    QVERIFY( !share->isChecked() );
    QVERIFY( !slotBox->isEnabled() );
    QVERIFY( !limit->isEnabled() );

    share->setChecked( true );
    QVERIFY( slotBox->isEnabled() );
    QVERIFY( limit->isEnabled() );
}

QTEST_MAIN( ConfigDialogTest )

#include "ConfigDialogTest.moc"
