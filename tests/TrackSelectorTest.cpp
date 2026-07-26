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


#include "TrackSelector.h"

#include <QCheckBox>
#include <QLineEdit>
#include <QPushButton>
#include <QTreeWidget>
#include <QtTest>

/**
  * @class TrackSelectorTest
  * @short The rules of the file selector, which are all about what sticks.
  *
  * The distinction this guards is the one the whole thing turns on: leaving a
  * file unticked keeps it out of the playlist and nothing more, while
  * excluding it is a refusal that survives the next visit. Getting them the
  * wrong way round gives the behaviour where you press select all and the
  * three tracks you threw out come back for ever.
  */
class TrackSelectorTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void ticksTheAudioOfANewTorrent();
    void leavesAnExcludedFileAlone();
    void letsAnExcludedFileBackIn();
    void keepsWhatWasChosenBefore();
    void hidesWhatIsNotAudioUntilAsked();
    void selectAllOnlyTouchesWhatIsShown();

private:
    /** a file of a torrent, as the catalogue would hand it over */
    static TrackInfoPtr track( int index, const QString &path, bool audio, bool excluded = false );

    QTreeWidget* listOf( TrackSelector *selector ) const;
};

TrackInfoPtr TrackSelectorTest::track( int index, const QString &path, bool audio, bool excluded )
{
    TrackInfoPtr info( new TrackInfo );
    info->setId( index + 1 );
    info->setInfoHash( QString( 40, QLatin1Char( 'b' ) ) );
    info->setFileIndex( index );
    info->setFilePath( path );
    info->setFileSize( 1000 * ( index + 1 ) );
    info->setIsAudio( audio );
    info->setExcluded( excluded );

    return info;
}

QTreeWidget* TrackSelectorTest::listOf( TrackSelector *selector ) const
{
    return selector->findChild<QTreeWidget *>( QLatin1String( "fileList" ) );
}

void TrackSelectorTest::ticksTheAudioOfANewTorrent()
{
    const QList<TrackInfoPtr> tracks = {
        track( 0, QLatin1String( "album/01.mp3" ), true ),
        track( 1, QLatin1String( "album/cover.jpg" ), false ),
        track( 2, QLatin1String( "album/02.flac" ), true )
    };

    TrackSelector selector( QLatin1String( "album" ), tracks );

    const QList<TrackInfoPtr> selected = selector.selectedTracks();
    QCOMPARE( selected.count(), 2 );
    QCOMPARE( selected.at( 0 )->filePath(), QLatin1String( "album/01.mp3" ) );
    QCOMPARE( selected.at( 1 )->filePath(), QLatin1String( "album/02.flac" ) );
}

void TrackSelectorTest::leavesAnExcludedFileAlone()
{
    const QList<TrackInfoPtr> tracks = {
        track( 0, QLatin1String( "album/01.mp3" ), true ),
        track( 1, QLatin1String( "album/02.mp3" ), true, /*excluded*/ true )
    };

    TrackSelector selector( QLatin1String( "album" ), tracks );

    // Shown, so that it can be taken back, but never ticked on its own: that
    // is what makes a refusal a refusal.
    QCOMPARE( listOf( &selector )->topLevelItemCount(), 2 );
    QVERIFY( !listOf( &selector )->topLevelItem( 1 )->isHidden() );

    const QList<TrackInfoPtr> selected = selector.selectedTracks();
    QCOMPARE( selected.count(), 1 );
    QCOMPARE( selected.first()->filePath(), QLatin1String( "album/01.mp3" ) );

    // And select all must not bring it back either.
    selector.findChild<QPushButton *>( QLatin1String( "selectAllButton" ) )->click();
    QCOMPARE( selector.selectedTracks().count(), 2 );
    QCOMPARE( selector.readmittedTracks().count(), 1 );
}

void TrackSelectorTest::letsAnExcludedFileBackIn()
{
    const QList<TrackInfoPtr> tracks = {
        track( 0, QLatin1String( "album/01.mp3" ), true, /*excluded*/ true )
    };

    TrackSelector selector( QLatin1String( "album" ), tracks );
    QVERIFY( selector.selectedTracks().isEmpty() );

    listOf( &selector )->topLevelItem( 0 )->setCheckState( 0, Qt::Checked );

    QCOMPARE( selector.selectedTracks().count(), 1 );
    QCOMPARE( selector.readmittedTracks().count(), 1 );
}

void TrackSelectorTest::keepsWhatWasChosenBefore()
{
    const QList<TrackInfoPtr> tracks = {
        track( 0, QLatin1String( "album/01.mp3" ), true ),
        track( 1, QLatin1String( "album/02.mp3" ), true ),
        track( 2, QLatin1String( "album/03.mp3" ), true )
    };

    // Reopening a torrent shows the playlist as it stands, not everything
    // ticked again: a track taken out by hand would otherwise come straight
    // back the next time the selector is opened.
    TrackSelector selector( QLatin1String( "album" ), tracks, { 1, 3 } );

    const QList<TrackInfoPtr> selected = selector.selectedTracks();
    QCOMPARE( selected.count(), 2 );
    QCOMPARE( selected.at( 0 )->filePath(), QLatin1String( "album/01.mp3" ) );
    QCOMPARE( selected.at( 1 )->filePath(), QLatin1String( "album/03.mp3" ) );

    // Taken out of the playlist is not refused: nothing was excluded here.
    QVERIFY( selector.readmittedTracks().isEmpty() );
}

void TrackSelectorTest::hidesWhatIsNotAudioUntilAsked()
{
    const QList<TrackInfoPtr> tracks = {
        track( 0, QLatin1String( "album/01.mp3" ), true ),
        track( 1, QLatin1String( "album/notes.txt" ), false )
    };

    TrackSelector selector( QLatin1String( "album" ), tracks );
    QVERIFY( listOf( &selector )->topLevelItem( 1 )->isHidden() );

    selector.findChild<QCheckBox *>( QLatin1String( "showAllFiles" ) )->setChecked( true );
    QVERIFY( !listOf( &selector )->topLevelItem( 1 )->isHidden() );
}

void TrackSelectorTest::selectAllOnlyTouchesWhatIsShown()
{
    const QList<TrackInfoPtr> tracks = {
        track( 0, QLatin1String( "album/allegro.mp3" ), true ),
        track( 1, QLatin1String( "album/adagio.mp3" ), true )
    };

    TrackSelector selector( QLatin1String( "album" ), tracks );
    selector.findChild<QPushButton *>( QLatin1String( "selectNoneButton" ) )->click();
    QVERIFY( selector.selectedTracks().isEmpty() );

    selector.findChild<QLineEdit *>( QLatin1String( "filterLine" ) )->setText( QLatin1String( "adagio" ) );
    selector.findChild<QPushButton *>( QLatin1String( "selectAllButton" ) )->click();

    // With a filter typed in, select all means what is on screen. Three
    // hundred files coming back instead would be a surprise, and a bad one.
    const QList<TrackInfoPtr> selected = selector.selectedTracks();
    QCOMPARE( selected.count(), 1 );
    QCOMPARE( selected.first()->filePath(), QLatin1String( "album/adagio.mp3" ) );
}

QTEST_MAIN( TrackSelectorTest )

#include "TrackSelectorTest.moc"
