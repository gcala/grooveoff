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

#include "db/Catalog.h"
#include "db/Database.h"
#include "grooveoff/Settings.h"
#include "torrent/CacheManager.h"
#include "torrent/DownloadQueue.h"

#include <QFile>
#include <QSettings>
#include <QStandardPaths>
#include <QtTest>

/**
  * @class DownloadQueueTest
  * @short The order in which a playlist is fetched, and where it stops.
  *
  * Nothing here touches the network, a torrent or a disk: the rule is a
  * function of the selection, the track being played and the room left in the
  * cache, and it was written in one place so that it could be asked this way.
  *
  * The order is the part worth pinning. It is not "first to last", which is
  * what everybody assumes on reading the feature, and it is not "fill the
  * holes first" either: the cursor follows the track being played and walks
  * forward, and only when there is nothing left ahead does it come back. Get
  * that backwards and the symptom is a playlist that fetches the past while
  * the listener is moving into the future, which sounds exactly like a slow
  * swarm and is not one.
  */
class DownloadQueueTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();

    void fetchesTheWholeSelectionInOrder();
    void theCursorWalksForwardAndComesBackForTheHoles();
    void aFileOutsideTheSelectionIsLeftAlone();
    void switchedOffOnlyWhatIsPlayingIsFetched();
    void aFullCacheStopsTheQueueAndEvictsNothing();
    void asksForTheSleepersWhenFetchingCanResume();

private:
    static const QString s_hash;

    /** the ten tracks somebody chose out of a sixteen file torrent */
    static QList<int> selection()
    {
        return { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
    }
};

const QString DownloadQueueTest::s_hash = QStringLiteral( "aaaabbbbccccddddeeeeffff00001111a2a2a3a3" );

void DownloadQueueTest::initTestCase()
{
    QStandardPaths::setTestModeEnabled( true );

    QCoreApplication::setOrganizationName( QStringLiteral( "gcala-test" ) );
    QCoreApplication::setApplicationName( QStringLiteral( "grooveoff-queue-test" ) );

    QSettings settings;
    settings.clear();
    settings.sync();

    QVERIFY2( The::database()->isOpen(), qPrintable( The::database()->errorString() ) );

    The::downloadQueue()->setSelection( s_hash, selection() );
}

void DownloadQueueTest::cleanupTestCase()
{
    QSettings settings;
    settings.clear();
    settings.sync();

    // The file goes with the connection, and that is not tidiness: a database
    // left behind was written by whatever schema this build had at the time,
    // and the next build refuses to open one it does not recognise. The test
    // then fails for a reason that has nothing to do with what it is testing.
    const QString file =
        QStandardPaths::writableLocation( QStandardPaths::AppDataLocation )
        + QStringLiteral( "/grooveoff.db" );

    delete The::database();
    QFile::remove( file );
}

/*!
  \brief fetchesTheWholeSelectionInOrder: nothing playing, everything wanted

  With no cursor yet the queue starts at the top of the list, which is where
  somebody adding a torrent is going to start listening.
*/
void DownloadQueueTest::fetchesTheWholeSelectionInOrder()
{
    DownloadQueue *queue = The::downloadQueue();
    queue->clearPlaying();

    for( int fileIndex : selection() )
        QCOMPARE( queue->priorityFor( s_hash, fileIndex ), TorrentSession::NormalFile );
}

/*!
  \brief theCursorWalksForwardAndComesBackForTheHoles: the rule itself

  Ten tracks, and somebody who was listening from the beginning jumps to the
  sixth. What comes next is the seventh, not the third: people listen forwards,
  so what follows the track being played is what will be wanted next, and the
  tracks left behind are picked up when there is nothing ahead of the cursor
  left to fetch.
*/
void DownloadQueueTest::theCursorWalksForwardAndComesBackForTheHoles()
{
    DownloadQueue *queue = The::downloadQueue();

    queue->setPlaying( s_hash, 5 );

    QCOMPARE( queue->priorityFor( s_hash, 5 ), TorrentSession::TopFile );

    for( int ahead : { 6, 7, 8, 9 } )
        QCOMPARE( queue->priorityFor( s_hash, ahead ), TorrentSession::NormalFile );

    for( int behind : { 0, 1, 2, 3, 4 } )
        QCOMPARE( queue->priorityFor( s_hash, behind ), TorrentSession::QueuedFile );

    // And again, backwards this time: the cursor follows whatever was asked
    // for last, so the tracks between the two jumps go back to being ahead.
    queue->setPlaying( s_hash, 3 );

    QCOMPARE( queue->priorityFor( s_hash, 3 ), TorrentSession::TopFile );

    for( int ahead : { 4, 5, 6, 7, 8, 9 } )
        QCOMPARE( queue->priorityFor( s_hash, ahead ), TorrentSession::NormalFile );

    for( int behind : { 0, 1, 2 } )
        QCOMPARE( queue->priorityFor( s_hash, behind ), TorrentSession::QueuedFile );
}

/*!
  \brief aFileOutsideTheSelectionIsLeftAlone: six of the sixteen were not chosen

  Fetching ahead is about the playlist and not about the torrent. A file left
  out of the selector, or thrown out of it afterwards, must not arrive anyway
  through the back door.
*/
void DownloadQueueTest::aFileOutsideTheSelectionIsLeftAlone()
{
    DownloadQueue *queue = The::downloadQueue();
    queue->setPlaying( s_hash, 5 );

    for( int unchosen : { 10, 11, 12, 13, 14, 15 } )
        QCOMPARE( queue->priorityFor( s_hash, unchosen ), TorrentSession::SkipFile );
}

/*!
  \brief switchedOffOnlyWhatIsPlayingIsFetched: the application it used to be

  Somebody paying by the megabyte turns this off and gets the old behaviour
  back exactly: a track arrives because it was asked for, and nothing else
  moves. The track being played still has to be fetched, or the setting would
  be a way of breaking the player.
*/
void DownloadQueueTest::switchedOffOnlyWhatIsPlayingIsFetched()
{
    DownloadQueue *queue = The::downloadQueue();

    QSettings settings;
    settings.setValue( Settings::downloadAhead, false );
    settings.sync();
    queue->reloadSettings();

    queue->setPlaying( s_hash, 5 );

    QCOMPARE( queue->priorityFor( s_hash, 5 ), TorrentSession::TopFile );

    for( int other : { 0, 4, 6, 9 } )
        QCOMPARE( queue->priorityFor( s_hash, other ), TorrentSession::SkipFile );

    settings.setValue( Settings::downloadAhead, true );
    settings.sync();
    queue->reloadSettings();

    QCOMPARE( queue->priorityFor( s_hash, 6 ), TorrentSession::NormalFile );
}

/*!
  \brief aFullCacheStopsTheQueueAndEvictsNothing: the invariant that matters most

  Bytes nobody asked for must never cost bytes somebody did. A queue that
  reacted to a full cache by making room would throw away a torrent listened to
  yesterday in order to fetch one nobody has pressed play on yet — the worst
  trade in the program, and a silent one.
*/
void DownloadQueueTest::aFullCacheStopsTheQueueAndEvictsNothing()
{
    DownloadQueue *queue = The::downloadQueue();
    Catalog *catalog = The::catalog();

    QVERIFY( catalog->addTorrent( s_hash, QStringLiteral( "magnet:?xt=urn:btih:" ) + s_hash,
                                  QStringLiteral( "A torrent that filled the cache" ) ) );
    QVERIFY( catalog->setBytesDone( s_hash, 200 * 1024 * 1024 ) );

    QSettings settings;
    settings.setValue( Settings::cacheSize, 100 );   // megabytes, and already over
    settings.sync();

    QVERIFY2( !The::cacheManager()->hasRoomForPrefetch(), "the cache should be over its ceiling" );

    queue->setPlaying( s_hash, 5 );

    // What is being listened to is still fetched: it was asked for.
    QCOMPARE( queue->priorityFor( s_hash, 5 ), TorrentSession::TopFile );

    for( int other : { 0, 4, 6, 9 } )
        QCOMPARE( queue->priorityFor( s_hash, other ), TorrentSession::SkipFile );

    // And nothing was thrown away to make room. The bytes are where they were.
    QCOMPARE( catalog->totalBytesDone(), qint64( 200 ) * 1024 * 1024 );

    settings.setValue( Settings::cacheSize, Settings::cacheSizeDefault );
    settings.sync();

    QCOMPARE( queue->priorityFor( s_hash, 6 ), TorrentSession::NormalFile );
}

/*!
  \brief asksForTheSleepersWhenFetchingCanResume: the half that is easy to forget

  The queue stops on its own — the cache fills up, or somebody switches the
  fetching off — and that half works by itself, since a priority is simply not
  handed out. Starting again does not: the torrents that would do the fetching
  are asleep by then, and a priority handed to a torrent the session is not
  holding goes nowhere at all. Nothing else in the program is watching for the
  moment it becomes worth waking them, because neither of the two things that
  change it is a gesture anybody made.

  So the queue says so, and this asks that it says so exactly when the answer
  turns from no to yes: not on the way down, and not twice for one change.
*/
void DownloadQueueTest::asksForTheSleepersWhenFetchingCanResume()
{
    DownloadQueue *queue = The::downloadQueue();
    QSignalSpy spy( queue, &DownloadQueue::fetchingResumed );

    QSettings settings;

    // Off: nothing would be fetched, so there is nothing to wake up for.
    settings.setValue( Settings::downloadAhead, false );
    settings.sync();
    queue->reloadSettings();
    QCOMPARE( spy.count(), 0 );

    settings.setValue( Settings::downloadAhead, true );
    settings.sync();
    queue->reloadSettings();
    QCOMPARE( spy.count(), 1 );

    // And again with the same answer, which is not a change. Said twice, a
    // start up would announce every unfinished torrent for a second time.
    queue->reloadSettings();
    QCOMPARE( spy.count(), 1 );

    // The other road in, and the direction that must stay quiet: the cache
    // filling up takes the fetching away rather than giving it back.
    settings.setValue( Settings::cacheSize, 100 );   // megabytes, against 200 held
    settings.sync();
    QVERIFY( !The::cacheManager()->hasRoomForPrefetch() );

    QVERIFY( QMetaObject::invokeMethod( queue, "checkCacheRoom" ) );
    QCOMPARE( spy.count(), 1 );

    settings.setValue( Settings::cacheSize, Settings::cacheSizeDefault );
    settings.sync();
    QVERIFY( The::cacheManager()->hasRoomForPrefetch() );

    QVERIFY( QMetaObject::invokeMethod( queue, "checkCacheRoom" ) );
    QCOMPARE( spy.count(), 2 );
}

QTEST_MAIN( DownloadQueueTest )

#include "DownloadQueueTest.moc"
