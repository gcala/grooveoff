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


#include "Roles.h"
#include "Utility.h"
#include "widgets/PlaylistList.h"

#include "db/Catalog.h"
#include "db/Database.h"
#include "db/PlaylistStore.h"

#include <QFile>
#include <QMimeData>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QtTest>

/**
  * @class PlaylistTest
  * @short The three ways out of removing a torrent, and the sidebar in front
  *        of them.
  *
  * Removal is the one operation of this application that can destroy something
  * the user made by hand, and the damage is silent: nobody notices a track
  * missing from a playlist until they go looking for it, weeks later. So the
  * two ways that are not Cancel are followed one at a time, and each is asked
  * afterwards what is left rather than what was returned.
  *
  * Told in order, like the other tests here: it is one story about two torrents
  * and a playlist made out of both.
  */
class PlaylistTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();

    void namesThePlaylistsThatWouldSuffer();
    void keepingTheTracksOnlyHidesTheTorrent();
    void theHiddenTorrentGoesWithItsLastTrack();
    void removingEverythingTakesTheTracksWithIt();
    void forgetsTheOpenPlaylistWhenItIsRemoved();

    void refusesTracksDroppedOnATorrentPlaylist();
    void acceptsTracksDroppedOnACustomPlaylist();

    void reordersRowsWithoutLosingOne();
    void reordersRowsMovedDownwards();
    void reordersSeveralRowsAtOnce();
    void refusesAnImpossibleReorder();

private:
    static TrackInfoPtr makeTrack( const QString &infoHash, int fileIndex, const QString &filePath );

    /** a sidebar holding one playlist of each kind */
    PlaylistList* buildSidebar( qint64 torrentPlaylistId, qint64 customPlaylistId ) const;


    Catalog*       m_catalog   = nullptr;
    PlaylistStore* m_playlists = nullptr;

    QString m_hashA;
    QString m_hashB;
    QList<TrackInfoPtr> m_tracksA;
    QList<TrackInfoPtr> m_tracksB;

    qint64 m_customId = 0;
};

TrackInfoPtr PlaylistTest::makeTrack( const QString &infoHash, int fileIndex, const QString &filePath )
{
    TrackInfoPtr track( new TrackInfo() );
    track->setInfoHash( infoHash );
    track->setFileIndex( fileIndex );
    track->setFilePath( filePath );
    track->setFileSize( 5 * 1024 * 1024 );
    track->setIsAudio( TrackInfo::isAudioFile( filePath ) );

    return track;
}

void PlaylistTest::initTestCase()
{
    QStandardPaths::setTestModeEnabled( true );
    QCoreApplication::setOrganizationName( QStringLiteral( "gcala" ) );
    QCoreApplication::setApplicationName( QStringLiteral( "grooveoff-playlisttest" ) );

    const QString &path = QStandardPaths::writableLocation( QStandardPaths::AppDataLocation )
                          + QLatin1String( "/grooveoff.db" );
    QFile::remove( path );
    QFile::remove( path + QLatin1String( "-wal" ) );
    QFile::remove( path + QLatin1String( "-shm" ) );

    m_catalog   = The::catalog();
    m_playlists = The::playlistStore();

    QVERIFY2( The::database()->isOpen(), qPrintable( The::database()->errorString() ) );

    m_hashA = QStringLiteral( "aaaa000000000000000000000000000000000001" );
    m_hashB = QStringLiteral( "bbbb000000000000000000000000000000000002" );

    QVERIFY( m_catalog->addTorrent( m_hashA, QLatin1String( "magnet:?xt=urn:btih:" ) + m_hashA,
                                    QStringLiteral( "Album A" ) ) );
    QVERIFY( m_catalog->addTorrent( m_hashB, QLatin1String( "magnet:?xt=urn:btih:" ) + m_hashB,
                                    QStringLiteral( "Album B" ) ) );

    m_tracksA << makeTrack( m_hashA, 0, QStringLiteral( "Album A/01.mp3" ) )
              << makeTrack( m_hashA, 1, QStringLiteral( "Album A/02.mp3" ) );
    m_tracksB << makeTrack( m_hashB, 0, QStringLiteral( "Album B/01.flac" ) )
              << makeTrack( m_hashB, 1, QStringLiteral( "Album B/02.flac" ) );

    QVERIFY( m_catalog->addTracks( m_tracksA ) );
    QVERIFY( m_catalog->addTracks( m_tracksB ) );

    QVERIFY( m_playlists->setTracks( m_playlists->torrentPlaylistId( m_hashA ), m_tracksA ) );
    QVERIFY( m_playlists->setTracks( m_playlists->torrentPlaylistId( m_hashB ), m_tracksB ) );

    // The list made by hand, out of both torrents: this is what the removal has
    // to be careful with.
    m_customId = m_playlists->createCustomPlaylist( QStringLiteral( "Sunday morning" ) );
    QVERIFY( m_customId > 0 );
    QVERIFY( m_playlists->appendTracks( m_customId, { m_tracksA.at( 0 ), m_tracksA.at( 1 ), m_tracksB.at( 0 ) } ) );

    QCOMPARE( m_playlists->visiblePlaylists().count(), 3 );
}

void PlaylistTest::namesThePlaylistsThatWouldSuffer()
{
    // The dialog cannot ask its question without these numbers: "some tracks
    // are used elsewhere" leaves nothing to choose between.
    const QList<Catalog::PlaylistReference> references = m_catalog->customPlaylistReferences( m_hashA );

    QCOMPARE( references.count(), 1 );
    QCOMPARE( references.first().playlistId, m_customId );
    QCOMPARE( references.first().name, QStringLiteral( "Sunday morning" ) );
    QCOMPARE( references.first().trackCount, 2 );

    // A torrent nobody borrowed from is the simple yes or no case.
    QVERIFY( m_catalog->customPlaylistReferences( QStringLiteral( "no-such-torrent" ) ).isEmpty() );
}

void PlaylistTest::keepingTheTracksOnlyHidesTheTorrent()
{
    QVERIFY( m_catalog->hideTorrent( m_hashA ) );

    // Out of the sidebar.
    const QList<PlaylistStore::PlaylistInfo> visible = m_playlists->visiblePlaylists();
    QCOMPARE( visible.count(), 2 );

    for( const PlaylistStore::PlaylistInfo &playlist : visible )
        QVERIFY( playlist.infoHash != m_hashA );

    // And nowhere else. The tracks are still in the list they were put in, and
    // still playable — which is the whole promise of this way out.
    QCOMPARE( m_playlists->tracks( m_customId ).count(), 3 );
    QVERIFY( m_catalog->isKnown( m_hashA ) );
    QVERIFY( m_catalog->isHidden( m_hashA ) );
    QVERIFY( m_catalog->track( m_hashA, 0 ) );
    QVERIFY( !m_catalog->magnetUri( m_hashA ).isEmpty() );
}

void PlaylistTest::theHiddenTorrentGoesWithItsLastTrack()
{
    // Taking the tracks out one at a time, which is the trigger the store has
    // to notice: a hidden torrent nobody refers to any more is rubbish that no
    // interface would ever show again.
    QCOMPARE( m_playlists->tracks( m_customId ).at( 0 )->infoHash(), m_hashA );
    QVERIFY( m_playlists->removeTrackAt( m_customId, 0 ) );

    // One of the two is still there, so the torrent stays.
    QVERIFY( m_catalog->isKnown( m_hashA ) );
    QCOMPARE( m_playlists->tracks( m_customId ).count(), 2 );

    QVERIFY( m_playlists->removeTrackAt( m_customId, 0 ) );

    // That was the last one.
    QVERIFY( !m_catalog->isKnown( m_hashA ) );
    QVERIFY( m_catalog->orphanedHiddenTorrents().isEmpty() );

    // And what belonged to the other torrent did not go with it.
    QCOMPARE( m_playlists->tracks( m_customId ).count(), 1 );
    QCOMPARE( m_playlists->tracks( m_customId ).first()->infoHash(), m_hashB );
}

void PlaylistTest::removingEverythingTakesTheTracksWithIt()
{
    // The other way out, and the destructive one: it is meant to take the
    // tracks out of the lists made by hand, and the user was told so.
    QCOMPARE( m_catalog->customPlaylistReferences( m_hashB ).count(), 1 );

    QVERIFY( m_catalog->removeTorrent( m_hashB ) );

    QVERIFY( !m_catalog->isKnown( m_hashB ) );
    QVERIFY( m_playlists->tracks( m_customId ).isEmpty() );

    // The list itself is not a casualty: it is empty, not gone.
    QVERIFY( m_playlists->playlist( m_customId ).isValid() );
    QCOMPARE( m_playlists->visiblePlaylists().count(), 1 );
}

/*!
  What the window shows is the playlist app_state points at, so a playlist that
  has been removed must stop being that one. The schema promises it with an
  ON DELETE SET NULL and nothing was checking the promise; if it ever went, the
  window would ask for the tracks of a list that no longer exists, get none, and
  have no reason to empty itself — the rows of the removed playlist would stay
  on screen until the next start.
*/
void PlaylistTest::forgetsTheOpenPlaylistWhenItIsRemoved()
{
    QVERIFY( m_playlists->setPlaybackState( m_customId, 0, 0 ) );
    QCOMPARE( m_playlists->currentPlaylistId(), m_customId );

    QVERIFY( m_playlists->removeCustomPlaylist( m_customId ) );
    QCOMPARE( m_playlists->currentPlaylistId(), qint64( 0 ) );

    // And by the other road, which reaches app_state through two cascades
    // rather than one: the torrent takes its playlist, the playlist takes the
    // reference, and the track that was playing goes the same way.
    const QString hashC = QStringLiteral( "cccc000000000000000000000000000000000003" );
    QVERIFY( m_catalog->addTorrent( hashC, QLatin1String( "magnet:?xt=urn:btih:" ) + hashC,
                                    QStringLiteral( "Album C" ) ) );

    const QList<TrackInfoPtr> tracksC = { makeTrack( hashC, 0, QStringLiteral( "Album C/01.mp3" ) ) };
    QVERIFY( m_catalog->addTracks( tracksC ) );

    const qint64 playlistC = m_playlists->torrentPlaylistId( hashC );
    QVERIFY( playlistC > 0 );
    QVERIFY( m_playlists->setTracks( playlistC, tracksC ) );

    QVERIFY( m_playlists->setPlaybackState( playlistC, tracksC.first()->id(), 12000 ) );
    QCOMPARE( m_playlists->currentPlaylistId(), playlistC );
    QCOMPARE( m_playlists->currentTrackId(), tracksC.first()->id() );

    QVERIFY( m_catalog->removeTorrent( hashC ) );

    QCOMPARE( m_playlists->currentPlaylistId(), qint64( 0 ) );
    QCOMPARE( m_playlists->currentTrackId(), qint64( 0 ) );
    QVERIFY( m_playlists->visiblePlaylists().isEmpty() );
}

PlaylistList* PlaylistTest::buildSidebar( qint64 torrentPlaylistId, qint64 customPlaylistId ) const
{
    PlaylistList *sidebar = new PlaylistList();
    sidebar->resize( 200, 200 );

    QListWidgetItem *torrentRow = new QListWidgetItem( QStringLiteral( "Album" ) );
    torrentRow->setData( PlaylistRoles::Id, torrentPlaylistId );
    torrentRow->setData( PlaylistRoles::Kind, static_cast<int>( PlaylistStore::TorrentPlaylist ) );
    sidebar->addItem( torrentRow );

    QListWidgetItem *customRow = new QListWidgetItem( QStringLiteral( "Mine" ) );
    customRow->setData( PlaylistRoles::Id, customPlaylistId );
    customRow->setData( PlaylistRoles::Kind, static_cast<int>( PlaylistStore::CustomPlaylist ) );
    sidebar->addItem( customRow );

    sidebar->show();
    QCoreApplication::processEvents();

    return sidebar;
}

void PlaylistTest::refusesTracksDroppedOnATorrentPlaylist()
{
    // Not an omission: what a torrent playlist holds is chosen in the file
    // selector, out of the files that torrent actually has. A track of another
    // torrent dropped in would be a row its own torrent cannot play.
    PlaylistList *sidebar = buildSidebar( 1, 2 );

    QVERIFY( !sidebar->acceptsTracks( 1 ) );

    // A drop that landed on no row at all is not a drop on the first one.
    QVERIFY( !sidebar->acceptsTracks( 0 ) );
    QVERIFY( !sidebar->acceptsTracks( 99 ) );

    delete sidebar;
}

void PlaylistTest::acceptsTracksDroppedOnACustomPlaylist()
{
    PlaylistList *sidebar = buildSidebar( 1, 2 );

    QVERIFY( sidebar->acceptsTracks( 2 ) );

    // The row under the cursor is found by position, and a wrong answer here
    // would send the tracks to a playlist nobody pointed at.
    QCOMPARE( sidebar->playlistAt( sidebar->visualItemRect( sidebar->item( 0 ) ).center() ), qint64( 1 ) );
    QCOMPARE( sidebar->playlistAt( sidebar->visualItemRect( sidebar->item( 1 ) ).center() ), qint64( 2 ) );
    QCOMPARE( sidebar->playlistAt( QPoint( -10, -10 ) ), qint64( 0 ) );

    // And the ids come back out the other side unchanged, which is what makes
    // a dropped track point at the same catalogue row as the dragged one.
    QMimeData mime;
    mime.setData( GrooveOffMime::tracks, GrooveOffMime::encodeTracks( { 10, 11 } ) );
    QCOMPARE( GrooveOffMime::decodeTracks( &mime ), QList<qint64>( { 10, 11 } ) );

    QVERIFY( GrooveOffMime::decodeTracks( nullptr ).isEmpty() );

    QMimeData foreign;
    foreign.setText( QStringLiteral( "something else entirely" ) );
    QVERIFY( GrooveOffMime::decodeTracks( &foreign ).isEmpty() );

    delete sidebar;
}

void PlaylistTest::reordersRowsWithoutLosingOne()
{
    // Upwards: the row lands where the indicator was, and nothing shifts under
    // it, because everything taken out came from below the target.
    QCOMPARE( Utility::reorderedIndexes( 5, { 3 }, 1 ), QList<int>( { 0, 3, 1, 2, 4 } ) );
    QCOMPARE( Utility::reorderedIndexes( 5, { 4 }, 0 ), QList<int>( { 4, 0, 1, 2, 3 } ) );

    // A drop that changes nothing is still a valid answer, not an error.
    QCOMPARE( Utility::reorderedIndexes( 3, { 1 }, 1 ), QList<int>( { 0, 1, 2 } ) );
    QCOMPARE( Utility::reorderedIndexes( 3, { 1 }, 2 ), QList<int>( { 0, 1, 2 } ) );
}

void PlaylistTest::reordersRowsMovedDownwards()
{
    // This is the off-by-one the function exists for: the row leaves a hole
    // above the target, so the target moves up by one as it goes.
    QCOMPARE( Utility::reorderedIndexes( 5, { 0 }, 3 ), QList<int>( { 1, 2, 0, 3, 4 } ) );
    QCOMPARE( Utility::reorderedIndexes( 5, { 0 }, 5 ), QList<int>( { 1, 2, 3, 4, 0 } ) );
    QCOMPARE( Utility::reorderedIndexes( 4, { 1 }, 4 ), QList<int>( { 0, 2, 3, 1 } ) );
}

void PlaylistTest::reordersSeveralRowsAtOnce()
{
    // Several rows keep the order they had among themselves, whichever way
    // round they were selected.
    QCOMPARE( Utility::reorderedIndexes( 5, { 0, 1 }, 4 ), QList<int>( { 2, 3, 0, 1, 4 } ) );
    QCOMPARE( Utility::reorderedIndexes( 5, { 1, 0 }, 4 ), QList<int>( { 2, 3, 0, 1, 4 } ) );
    QCOMPARE( Utility::reorderedIndexes( 5, { 1, 3 }, 0 ), QList<int>( { 1, 3, 0, 2, 4 } ) );
}

void PlaylistTest::refusesAnImpossibleReorder()
{
    // A short list rather than a wrong one: the caller compares the length and
    // gives up, which beats writing a playlist with a track missing.
    QVERIFY( Utility::reorderedIndexes( 3, { 7 }, 0 ).isEmpty() );
    QVERIFY( Utility::reorderedIndexes( 3, { -1 }, 0 ).isEmpty() );
}

QTEST_MAIN( PlaylistTest )

#include "PlaylistTest.moc"
