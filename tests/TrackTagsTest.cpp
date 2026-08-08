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

#include "TrackTags.h"

#include <QFile>
#include <QTemporaryDir>
#include <QtTest>

/**
  * @class TrackTagsTest
  * @short That a file with nothing in it yet is not asked what it is.
  *
  * This is a test about a cost rather than about a result, which is unusual
  * enough to say why. A sparse file whose head has not arrived reads back as
  * zeros, and TagLib does not take that for an answer: the mpeg parser scans
  * forward for a sync word through the whole file. Ten megabytes of holes cost
  * a second, and an album of thirteen tracks added at once cost thirteen — the
  * window frozen, nothing drawn, nothing in the log.
  *
  * A test cannot usefully assert on a duration. What it can do is pin the
  * behaviour the guard produces: nothing comes back, and it comes back without
  * the file having been read past its first bytes. Take the guard away and
  * this still passes on a flac, which is exactly how the bug stayed hidden —
  * so the file here is an mp3.
  */
class TrackTagsTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void findsNoCoverInAFileThatHasNotArrived();
    void findsNoCoverInAFileThatIsNotThere();
    void refusesAPictureThatStopsHalfWay();
};

void TrackTagsTest::findsNoCoverInAFileThatHasNotArrived()
{
    QTemporaryDir directory;
    QVERIFY( directory.isValid() );

    // What libtorrent leaves on disk for a file it has not started: the right
    // size, and not one byte of content.
    const QString path = directory.filePath( QStringLiteral( "01 - nothing yet.mp3" ) );

    QFile file( path );
    QVERIFY( file.open( QIODevice::WriteOnly ) );
    QVERIFY( file.resize( 8 * 1024 * 1024 ) );
    file.close();

    QElapsedTimer timer;
    timer.start();

    QVERIFY2( TrackTags::cover( path ).isNull(),
              "a file made of holes cannot have a cover in it" );

    // Generous by two orders of magnitude on purpose: this is not measuring the
    // machine, it is catching a scan of the whole file, which takes about a
    // second for these eight megabytes and grows with the size.
    QVERIFY2( timer.elapsed() < 200,
              qPrintable( QStringLiteral( "asking took %1 ms, which is a scan and not a look"
                                          ).arg( timer.elapsed() ) ) );
}

void TrackTagsTest::findsNoCoverInAFileThatIsNotThere()
{
    QTemporaryDir directory;
    QVERIFY( directory.isValid() );

    QVERIFY( TrackTags::cover( directory.filePath( QStringLiteral( "absent.mp3" ) ) ).isNull() );
    QVERIFY( TrackTags::cover( QString() ).isNull() );
}

/*!
  \brief refusesAPictureThatStopsHalfWay

  The other half of the same story, and the one that took a field report to
  find. The tags of a file sit in its first two kilobytes and a cover is tens of
  them, so a picture read as soon as the head arrives comes back cut off. Qt
  decodes that into an image with a grey bottom, which is not null — so the row
  remembered it and never asked again, and the cover stayed broken until
  something unrelated rebuilt the list.

  A rule about bytes, asked without a file.
*/
void TrackTagsTest::refusesAPictureThatStopsHalfWay()
{
    QByteArray jpeg;
    jpeg.append( char( 0xFF ) ).append( char( 0xD8 ) );      // start of image
    jpeg.append( 2000, 'x' );                                 // whatever is inside
    jpeg.append( char( 0xFF ) ).append( char( 0xD9 ) );      // end of image

    QVERIFY( TrackTags::pictureIsWhole( jpeg ) );
    QVERIFY2( !TrackTags::pictureIsWhole( jpeg.left( jpeg.size() - 2 ) ),
              "a jpeg without its end marker was taken for a whole picture" );
    QVERIFY( !TrackTags::pictureIsWhole( jpeg.left( 400 ) ) );

    QByteArray png = QByteArrayLiteral( "\x89PNG\r\n\x1a\n" );
    const int header = png.size();
    png.append( 300, 'y' );

    QVERIFY( !TrackTags::pictureIsWhole( png ) );
    QVERIFY( TrackTags::pictureIsWhole( png + QByteArrayLiteral( "IEND" ) ) );
    QVERIFY( !TrackTags::pictureIsWhole( png.left( header ) ) );

    // Nothing at all is nothing, and a format nobody thought of is taken at
    // face value: refusing it would be a cover that never appears.
    QVERIFY( !TrackTags::pictureIsWhole( QByteArray() ) );
    QVERIFY( TrackTags::pictureIsWhole( QByteArrayLiteral( "some other picture format" ) ) );
}

QTEST_MAIN( TrackTagsTest )

#include "TrackTagsTest.moc"
