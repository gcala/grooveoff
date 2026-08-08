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
#include <QDialogButtonBox>
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
    void selectAllReallySelectsAll();
    void keepsWhatWasChosenBefore();
    void refusesToConfirmAnEmptySelection();
    void hidesWhatIsNotAudioUntilAsked();
    void selectAllOnlyTouchesWhatIsShown();

private:
    /** a file of a torrent, as the catalogue would hand it over */
    static TrackInfoPtr track( int index, const QString &path, bool audio );

    QTreeWidget* listOf( TrackSelector *selector ) const;
};

TrackInfoPtr TrackSelectorTest::track( int index, const QString &path, bool audio )
{
    TrackInfoPtr info( new TrackInfo );
    info->setId( index + 1 );
    info->setInfoHash( QString( 40, QLatin1Char( 'b' ) ) );
    info->setFileIndex( index );
    info->setFilePath( path );
    info->setFileSize( 1000 * ( index + 1 ) );
    info->setIsAudio( audio );

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

/*!
  \brief selectAllReallySelectsAll

  There used to be a sticky refusal that select all stepped over, so that three
  tracks thrown out stayed out however explicitly they were asked for. It is
  gone: a file left out of the playlist is unticked and nothing more, and the
  most explicit gesture in the dialog does what it says.
*/
void TrackSelectorTest::selectAllReallySelectsAll()
{
    const QList<TrackInfoPtr> tracks = {
        track( 0, QLatin1String( "album/01.mp3" ), true ),
        track( 1, QLatin1String( "album/02.mp3" ), true ),
        track( 2, QLatin1String( "album/03.mp3" ), true )
    };

    // Reopened on a playlist holding only the first: the other two were taken
    // out at some point, which the dialog has no memory of and no opinion on.
    TrackSelector selector( QLatin1String( "album" ), tracks, { 1 } );
    QCOMPARE( selector.selectedTracks().count(), 1 );

    selector.findChild<QPushButton *>( QLatin1String( "selectAllButton" ) )->click();
    QCOMPARE( selector.selectedTracks().count(), 3 );
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

}

/*!
  \brief refusesToConfirmAnEmptySelection

  A small guard carrying a large consequence. Unticking is now the same act as
  the bin, bytes included, so a selector confirmed with nothing ticked would
  throw away a whole torrent's music in one click. The dialog cannot be
  confirmed empty, and that is what stands between the two.
*/
void TrackSelectorTest::refusesToConfirmAnEmptySelection()
{
    const QList<TrackInfoPtr> tracks = {
        track( 0, QLatin1String( "album/01.mp3" ), true ),
        track( 1, QLatin1String( "album/02.mp3" ), true )
    };

    TrackSelector selector( QLatin1String( "album" ), tracks );

    QDialogButtonBox *buttons = selector.findChild<QDialogButtonBox *>();
    QVERIFY( buttons );
    QVERIFY( buttons->button( QDialogButtonBox::Ok )->isEnabled() );

    selector.findChild<QPushButton *>( QLatin1String( "selectNoneButton" ) )->click();

    QVERIFY( selector.selectedTracks().isEmpty() );
    QVERIFY2( !buttons->button( QDialogButtonBox::Ok )->isEnabled(),
              "an empty selection could be confirmed, which now deletes every byte" );
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
