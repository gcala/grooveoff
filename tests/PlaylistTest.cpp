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


#include "GrooveOffNamespace.h"
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

    void keepsThePinnedPlaylistFirstAndUndeletable();
    void holdsATrackAtMostOnce();
    void poursWholeTorrentsIntoOneList();
    void namesTheListsThatHoldTheseTracks();
    void keepsTheTracksSomethingElseUses();
    void takingATrackOutTakesItOutOfEveryList();
    void removingATrackFromACustomListLeavesItEverywhereElse();
    void emptiesAPlaylistWithoutRemovingIt();
    void namesThePlaylistsThatWouldSuffer();
    void keepingOnlyWhatIsUsedElsewhere();
    void removingEverythingTakesTheTracksWithIt();
    void forgetsTheOpenPlaylistWhenItIsRemoved();

    void refusesTracksDroppedOnATorrentPlaylist();
    void acceptsTracksDroppedOnACustomPlaylist();

    void readsTheNameAMagnetGivesItself();
    void showsAMagnetStillBeingLookedFor();
    void stopsBlinkingWhenTheLookingIsOver();

    void reordersRowsWithoutLosingOne();
    void reordersRowsMovedDownwards();
    void reordersSeveralRowsAtOnce();
    void refusesAnImpossibleReorder();
    void putsARetickedTrackBackWhereItWas();

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

    // Four and not three: the one that is always there was there before any of
    // these were made.
    QCOMPARE( m_playlists->visiblePlaylists().count(), 4 );
}

/*!
  \brief keepsThePinnedPlaylistFirstAndUndeletable

  A list to drop things into without naming one first, so it has to be where
  the hand goes — at the top, before the torrents — and it has to survive every
  gesture that removes a playlist. Refusing in the store and not only in the
  menu is the point: a menu can be worked around, a refusal here cannot.
*/
void PlaylistTest::keepsThePinnedPlaylistFirstAndUndeletable()
{
    const qint64 pinned = m_playlists->pinnedPlaylistId();
    QVERIFY( pinned > 0 );

    const QList<PlaylistStore::PlaylistInfo> visible = m_playlists->visiblePlaylists();
    QVERIFY( !visible.isEmpty() );
    QCOMPARE( visible.first().id, pinned );
    QVERIFY( visible.first().pinned );

    // It is a playlist made by hand in every other respect, which is what lets
    // the rest of the program go on asking one question instead of two.
    QCOMPARE( visible.first().kind, PlaylistStore::CustomPlaylist );

    QVERIFY2( !m_playlists->removeCustomPlaylist( pinned ), "the pinned playlist let itself be removed" );
    QVERIFY2( !m_playlists->rename( pinned, QStringLiteral( "Something else" ) ),
              "the pinned playlist let itself be renamed" );

    QVERIFY( m_playlists->playlist( pinned ).isValid() );
}

/*!
  \brief emptiesAPlaylistWithoutRemovingIt

  Emptying is the gesture the pinned playlist exists for, and it carries the
  same danger as a removal: the tracks going out may be the last thing keeping
  a hidden torrent alive. So the garbage collection runs with it, and a torrent
  nobody can see and nobody references is not left behind holding cache.
*/
/*!
  \brief holdsATrackAtMostOnce

  The same track in three lists is what having lists is for; the same track
  twice in one list is a list nobody meant to build, and the second copy says
  nothing the first does not. The schema refuses it and the one writer every
  path goes through drops it, which are two different jobs: the schema makes it
  impossible, the writer makes a caller that tries it lose one row rather than
  the whole write.
*/
/*!
  \brief putsARetickedTrackBackWhereItWas: the selector, used a second time

  Reopening the selector on a torrent already in the sidebar is for two things:
  taking more of it, and putting back a track removed by mistake. The second
  only works if "back" means where it was. Handing the selector's own order
  straight to setTracks() would send it to the end instead — and would quietly
  rearrange a playlist somebody had dragged into shape, which is the same
  mistake seen from the other side.

  Arithmetic, so it builds its own three tracks rather than borrowing the ones
  the story is holding: this runs last, by which time that torrent has been
  removed twice over.
*/
void PlaylistTest::putsARetickedTrackBackWhereItWas()
{
    QList<TrackInfoPtr> album;
    for( int i = 0; i < 3; ++i ) {
        TrackInfoPtr track( new TrackInfo );
        track->setId( i + 1 );
        track->setFileIndex( i );
        album << track;
    }

    // The second was taken out of the playlist, and is ticked again.
    const QList<TrackInfoPtr> current = { album.at( 0 ), album.at( 2 ) };
    const QList<TrackInfoPtr> merged  = Utility::mergedSelection( current, album );

    QCOMPARE( merged.count(), 3 );
    QCOMPARE( merged.at( 0 )->id(), album.at( 0 )->id() );
    QCOMPARE( merged.at( 1 )->id(), album.at( 1 )->id() );
    QCOMPARE( merged.at( 2 )->id(), album.at( 2 )->id() );

    // An order made by hand survives, and the newcomer lands after the track it
    // follows in the album rather than at the end.
    const QList<TrackInfoPtr> byHand = { album.at( 2 ), album.at( 0 ) };
    const QList<TrackInfoPtr> after  = Utility::mergedSelection( byHand, album );

    QCOMPARE( after.count(), 3 );
    QCOMPARE( after.at( 0 )->id(), album.at( 2 )->id() );
    QCOMPARE( after.at( 1 )->id(), album.at( 0 )->id() );
    QCOMPARE( after.at( 2 )->id(), album.at( 1 )->id() );

    // And unticking is how a track leaves.
    const QList<TrackInfoPtr> fewer = Utility::mergedSelection( album, { album.at( 0 ), album.at( 2 ) } );
    QCOMPARE( fewer.count(), 2 );
    QCOMPARE( fewer.at( 1 )->id(), album.at( 2 )->id() );
}

void PlaylistTest::holdsATrackAtMostOnce()
{
    const qint64 list = m_playlists->pinnedPlaylistId();
    const TrackInfoPtr track = m_tracksA.at( 0 );

    QVERIFY( m_playlists->clearPlaylist( list ) );
    QVERIFY( m_playlists->appendTracks( list, { track } ) );
    QCOMPARE( m_playlists->tracks( list ).count(), 1 );

    // Asked for again, by the menu or by a drag: it is already there.
    QVERIFY( m_playlists->appendTracks( list, { track } ) );
    QCOMPARE( m_playlists->tracks( list ).count(), 1 );
    QVERIFY( m_playlists->contains( list, track->id() ) );

    // And a list handed to it with a repeat inside keeps the first one, in the
    // place it was in, rather than failing on the constraint.
    QVERIFY( m_playlists->setTracks( list, { track, m_tracksB.at( 0 ), track } ) );

    const QList<TrackInfoPtr> written = m_playlists->tracks( list );
    QCOMPARE( written.count(), 2 );
    QCOMPARE( written.at( 0 )->id(), track->id() );
    QCOMPARE( written.at( 1 )->id(), m_tracksB.at( 0 )->id() );

    QVERIFY( m_playlists->clearPlaylist( list ) );
}

/*!
  \brief poursWholeTorrentsIntoOneList

  Three torrents of Vivaldi into «To listen» to hear them one after the other:
  the gesture the sidebar offers, asked of the store the way the window asks it
  — one torrent playlist at a time, whole.

  What is checked is the arithmetic of the second pour and the third, not the
  first: albums have to arrive one after the other and not interleaved, and a
  torrent poured into a list that already holds part of it must add the rest
  without moving what was there. Pouring one twice adds nothing, which is the
  same rule as [holdsATrackAtMostOnce] seen from the gesture that meets it.
*/
void PlaylistTest::poursWholeTorrentsIntoOneList()
{
    const qint64 list = m_playlists->pinnedPlaylistId();
    QVERIFY( m_playlists->clearPlaylist( list ) );

    const QList<TrackInfoPtr> fromA = m_playlists->tracks( m_playlists->torrentPlaylistId( m_hashA ) );
    const QList<TrackInfoPtr> fromB = m_playlists->tracks( m_playlists->torrentPlaylistId( m_hashB ) );
    QCOMPARE( fromA.count(), 2 );
    QCOMPARE( fromB.count(), 2 );

    QVERIFY( m_playlists->appendTracks( list, fromA ) );
    QVERIFY( m_playlists->appendTracks( list, fromB ) );

    QList<TrackInfoPtr> written = m_playlists->tracks( list );
    QCOMPARE( written.count(), 4 );
    QCOMPARE( written.at( 0 )->id(), fromA.at( 0 )->id() );
    QCOMPARE( written.at( 1 )->id(), fromA.at( 1 )->id() );
    QCOMPARE( written.at( 2 )->id(), fromB.at( 0 )->id() );
    QCOMPARE( written.at( 3 )->id(), fromB.at( 1 )->id() );

    // The same torrent again, by somebody who does not remember having done it.
    QVERIFY( m_playlists->appendTracks( list, fromA ) );
    QCOMPARE( m_playlists->tracks( list ).count(), 4 );

    // And half of one already there: the rest joins it, the one that was there
    // keeps its place rather than being moved to the end.
    QVERIFY( m_playlists->clearPlaylist( list ) );
    QVERIFY( m_playlists->appendTracks( list, { fromA.at( 1 ) } ) );
    QVERIFY( m_playlists->appendTracks( list, fromA ) );

    written = m_playlists->tracks( list );
    QCOMPARE( written.count(), 2 );
    QCOMPARE( written.at( 0 )->id(), fromA.at( 1 )->id() );
    QCOMPARE( written.at( 1 )->id(), fromA.at( 0 )->id() );

    QVERIFY( m_playlists->clearPlaylist( list ) );
}

/*!
  \brief namesTheListsThatHoldTheseTracks

  The question the removal of a track asks before it destroys anything, and it
  has to name the lists because that is the whole content of the warning. The
  one that is always there answers like any other and reports itself as pinned,
  since its stored name is English and the interface says it in its own
  language: without the flag the question prints «To listen» to somebody who has
  only ever seen «Da ascoltare».
*/
void PlaylistTest::namesTheListsThatHoldTheseTracks()
{
    const qint64 pinned = m_playlists->pinnedPlaylistId();
    QVERIFY( m_playlists->clearPlaylist( pinned ) );
    QVERIFY( m_playlists->appendTracks( pinned, { m_tracksA.at( 0 ) } ) );

    const QList<Catalog::PlaylistReference> holding =
        m_catalog->customPlaylistsHolding( { m_tracksA.at( 0 )->id() } );

    QCOMPARE( holding.count(), 2 );

    int pinnedSeen = 0;
    for( const Catalog::PlaylistReference &reference : holding ) {
        QCOMPARE( reference.trackCount, 1 );
        if( reference.pinned ) {
            ++pinnedSeen;
            QCOMPARE( reference.playlistId, pinned );
        }
    }
    QCOMPARE( pinnedSeen, 1 );

    // A track only its own torrent holds is nobody else's business, and neither
    // is nothing at all: both have to come back empty rather than answer with
    // every list there is.
    QVERIFY( m_catalog->customPlaylistsHolding( { m_tracksB.at( 1 )->id() } ).isEmpty() );
    QVERIFY( m_catalog->customPlaylistsHolding( {} ).isEmpty() );

    QVERIFY( m_playlists->clearPlaylist( pinned ) );
}

void PlaylistTest::keepsTheTracksSomethingElseUses()
{
    // What «remove only what nothing else uses» spares, and what the torrent's
    // own playlist is left holding.
    const QList<qint64> kept = m_catalog->tracksInCustomPlaylists( m_hashA );
    QCOMPARE( kept.count(), 2 );
    QVERIFY( kept.contains( m_tracksA.at( 0 )->id() ) );
    QVERIFY( kept.contains( m_tracksA.at( 1 )->id() ) );

    // Torrent B has one track in the list made by hand and one in nothing.
    const QList<qint64> keptB = m_catalog->tracksInCustomPlaylists( m_hashB );
    QCOMPARE( keptB.count(), 1 );
    QCOMPARE( keptB.first(), m_tracksB.at( 0 )->id() );

    QVERIFY( m_catalog->tracksInCustomPlaylists( QStringLiteral( "nothing" ) ).isEmpty() );
}

/*!
  \brief takingATrackOutTakesItOutOfEveryList

  The removal that means «never chosen». A torrent's playlist is the list of
  what was chosen from it, so a track leaving it has to leave the lists made by
  hand too — otherwise one of them would go on pointing at music whose bytes are
  about to be thrown away.

  What is checked besides the disappearance is the renumbering: the positions
  have to come back contiguous from zero in every list touched, because a drag
  reads them as indexes and one hole makes a track move by two.
*/
void PlaylistTest::takingATrackOutTakesItOutOfEveryList()
{
    const qint64 pinned   = m_playlists->pinnedPlaylistId();
    const qint64 torrentA = m_playlists->torrentPlaylistId( m_hashA );
    const qint64 removed  = m_tracksA.at( 0 )->id();

    QVERIFY( m_playlists->clearPlaylist( pinned ) );
    QVERIFY( m_playlists->appendTracks( pinned, { m_tracksB.at( 0 ), m_tracksA.at( 0 ),
                                                  m_tracksB.at( 1 ) } ) );

    QVERIFY( m_playlists->removeTracksEverywhere( { removed } ) );

    // Out of all three, and the torrent's own playlist is not an exception.
    QVERIFY( !m_playlists->contains( pinned, removed ) );
    QVERIFY( !m_playlists->contains( m_customId, removed ) );
    QVERIFY( !m_playlists->contains( torrentA, removed ) );

    // The neighbours keep their order, and close up over the hole.
    const QList<TrackInfoPtr> left = m_playlists->tracks( pinned );
    QCOMPARE( left.count(), 2 );
    QCOMPARE( left.at( 0 )->id(), m_tracksB.at( 0 )->id() );
    QCOMPARE( left.at( 1 )->id(), m_tracksB.at( 1 )->id() );

    // The row in track survives: the file is known and not chosen, which is
    // where every file starts and the only state left besides being in a list.
    QVERIFY( m_catalog->track( removed ) );

    // Asked again for something already gone: nothing to do and no failure.
    QVERIFY( m_playlists->removeTracksEverywhere( { removed } ) );
    QVERIFY( m_playlists->removeTracksEverywhere( {} ) );

    QVERIFY( m_playlists->clearPlaylist( pinned ) );
    QVERIFY( m_playlists->setTracks( torrentA, m_tracksA ) );
    QVERIFY( m_playlists->setTracks( m_customId, { m_tracksA.at( 0 ), m_tracksA.at( 1 ),
                                                   m_tracksB.at( 0 ) } ) );
}

void PlaylistTest::removingATrackFromACustomListLeavesItEverywhereElse()
{
    const qint64 torrentB = m_playlists->torrentPlaylistId( m_hashB );
    const qint64 taken    = m_tracksB.at( 0 )->id();

    QVERIFY( m_playlists->removeTracks( m_customId, { taken } ) );

    // The simple case, and the reason it is simple: a list made by hand is a
    // list and nothing else, so dropping something from it says nothing about
    // the track and nothing about the torrent.
    QVERIFY( !m_playlists->contains( m_customId, taken ) );
    QVERIFY( m_playlists->contains( torrentB, taken ) );
    QVERIFY( m_catalog->track( taken ) );
    QCOMPARE( m_playlists->tracks( m_customId ).count(), 2 );

    QVERIFY( m_playlists->setTracks( m_customId, { m_tracksA.at( 0 ), m_tracksA.at( 1 ),
                                                   m_tracksB.at( 0 ) } ) );
}

void PlaylistTest::emptiesAPlaylistWithoutRemovingIt()
{
    const qint64 pinned = m_playlists->pinnedPlaylistId();

    QVERIFY( m_playlists->appendTracks( pinned, { m_tracksA.at( 0 ), m_tracksB.at( 0 ) } ) );
    QCOMPARE( m_playlists->tracks( pinned ).count(), 2 );

    QVERIFY( m_playlists->clearPlaylist( pinned ) );

    QVERIFY( m_playlists->tracks( pinned ).isEmpty() );
    QVERIFY2( m_playlists->playlist( pinned ).isValid(), "emptying removed the playlist" );

    // And the tracks are still in the catalogue: emptying a list is not
    // throwing anything away.
    QVERIFY( m_catalog->track( m_tracksA.at( 0 )->id() ) );
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

/*!
  \brief keepingOnlyWhatIsUsedElsewhere

  The middle way out of «Remove playlist…» on a torrent somebody has borrowed
  from. It used to hide the torrent, which meant the row left the sidebar and
  the reason it was still alive lived in a column nobody could see. Now the row
  stays holding what survived: what is left and why is on the screen.

  The arithmetic is what the window does, asked of the store — the tracks a list
  made by hand still holds are kept, the rest go from every list, and the ones
  kept stay in the order they were in rather than coming back in file order.
*/
void PlaylistTest::keepingOnlyWhatIsUsedElsewhere()
{
    const qint64 torrentA = m_playlists->torrentPlaylistId( m_hashA );

    // The list made by hand holds the second track of A and nothing else of it.
    QVERIFY( m_playlists->setTracks( m_customId, { m_tracksA.at( 1 ), m_tracksB.at( 0 ) } ) );

    const QList<qint64> kept = m_catalog->tracksInCustomPlaylists( m_hashA );
    QCOMPARE( kept.count(), 1 );
    QCOMPARE( kept.first(), m_tracksA.at( 1 )->id() );

    QList<TrackInfoPtr> remaining;
    QList<qint64>       going;
    for( const TrackInfoPtr &track : m_playlists->tracks( torrentA ) ) {
        if( kept.contains( track->id() ) )
            remaining << track;
        else
            going << track->id();
    }

    QVERIFY( m_playlists->setTracks( torrentA, remaining ) );
    QVERIFY( m_playlists->removeTracksEverywhere( going ) );

    // The row is still there, with what survived in it.
    const QList<PlaylistStore::PlaylistInfo> visible = m_playlists->visiblePlaylists();
    QCOMPARE( visible.count(), 4 );

    QCOMPARE( m_playlists->tracks( torrentA ).count(), 1 );
    QCOMPARE( m_playlists->tracks( torrentA ).first()->id(), m_tracksA.at( 1 )->id() );

    // Still known, still playable, and still in the list it was put in.
    QVERIFY( m_catalog->isKnown( m_hashA ) );
    QVERIFY( m_catalog->track( m_hashA, 0 ) );
    QVERIFY( m_playlists->contains( m_customId, m_tracksA.at( 1 )->id() ) );

    QVERIFY( m_playlists->setTracks( torrentA, m_tracksA ) );
    QVERIFY( m_playlists->setTracks( m_customId, { m_tracksA.at( 0 ), m_tracksA.at( 1 ),
                                                   m_tracksB.at( 0 ) } ) );
}

void PlaylistTest::removingEverythingTakesTheTracksWithIt()
{
    // The other way out, and the destructive one: it is meant to take the
    // tracks out of the lists made by hand, and the user was told so.
    QCOMPARE( m_catalog->customPlaylistReferences( m_hashB ).count(), 1 );

    QVERIFY( m_catalog->removeTorrent( m_hashB ) );

    QVERIFY( !m_catalog->isKnown( m_hashB ) );

    // What belonged to that torrent went, and what belonged to the other one
    // stayed: a cascade that reached further would be deleting music somebody
    // put in a list by hand.
    const QList<TrackInfoPtr> left = m_playlists->tracks( m_customId );
    QCOMPARE( left.count(), 2 );
    for( const TrackInfoPtr &track : left )
        QCOMPARE( track->infoHash(), m_hashA );

    // The list itself is not a casualty either.
    QVERIFY( m_playlists->playlist( m_customId ).isValid() );

    // Torrent A, the list made by hand and the pinned one.
    QCOMPARE( m_playlists->visiblePlaylists().count(), 3 );
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
    // What is left is torrent A, which nothing in this story removed, and the
    // one nothing here created and nothing here can remove.
    const QList<PlaylistStore::PlaylistInfo> left = m_playlists->visiblePlaylists();
    QCOMPARE( left.count(), 2 );
    QVERIFY( left.first().pinned );

    // And now the same thing from the other side, which is how it actually
    // happens: the player is still holding the track when the rows behind it
    // are deleted, so the next write of the playback state names a track that
    // no longer exists. Written straight through, that fails on the foreign
    // key and takes the position with it — the whole upsert is one statement —
    // and the only sign of it is a line in the log nobody is reading. So the
    // writer drops what is gone and writes the rest.
    const qint64 deletedTrack = tracksC.first()->id();
    QVERIFY( deletedTrack > 0 );

    QVERIFY2( m_playlists->setPlaybackState( playlistC, deletedTrack, 34000 ),
              "writing the state of a track that has just been removed must not fail" );

    QCOMPARE( m_playlists->currentPlaylistId(), qint64( 0 ) );
    QCOMPARE( m_playlists->currentTrackId(), qint64( 0 ) );
    QCOMPARE( m_playlists->currentPositionMs(), qint64( 34000 ) );
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

/*!
  \brief readsTheNameAMagnetGivesItself

  It is the only thing known about a torrent for up to a minute, so the row
  waiting for its track list is called by it. The plus signs are the part worth
  a test: magnets are written by hand and by web pages, both spell a space that
  way, and QUrlQuery does not undo it — the row would be called
  "Camila+Cabello+-+Havana" and nothing anywhere would say why.
*/
void PlaylistTest::readsTheNameAMagnetGivesItself()
{
    QCOMPARE( Utility::magnetDisplayName( QStringLiteral(
                  "magnet:?xt=urn:btih:9D4A9495BE35D97B13E60D143F37CC38378D8233"
                  "&dn=Camila+Cabello+-+Havana&tr=udp%3A%2F%2Ftracker.org%3A80" ) ),
              QStringLiteral( "Camila Cabello - Havana" ) );

    // Percent encoding is undone as well, and the two live side by side.
    QCOMPARE( Utility::magnetDisplayName( QStringLiteral(
                  "magnet:?xt=urn:btih:0123456789abcdef&dn=Bach%3A+Cantate" ) ),
              QStringLiteral( "Bach: Cantate" ) );

    // A magnet without a name, which is legal and happens.
    QVERIFY( Utility::magnetDisplayName( QStringLiteral(
                 "magnet:?xt=urn:btih:0123456789abcdef" ) ).isEmpty() );

    // And something that is not a magnet at all: the caller falls back rather
    // than showing whatever this made of it.
    QVERIFY( Utility::magnetDisplayName( QStringLiteral( "/home/somebody/album.torrent" ) ).isEmpty() );
    QVERIFY( Utility::magnetDisplayName( QString() ).isEmpty() );
}

/*!
  \brief showsAMagnetStillBeingLookedFor

  The row exists before the catalogue knows anything, which is the whole point:
  the wait for a track list is up to a minute long and used to happen with
  nothing on screen saying so.
*/
void PlaylistTest::showsAMagnetStillBeingLookedFor()
{
    PlaylistList *sidebar = buildSidebar( 1, 2 );

    const QString hash = QStringLiteral( "1111111111111111111111111111111111111111" );
    sidebar->addPending( hash, QStringLiteral( "Camila Cabello - Havana" ) );

    QCOMPARE( sidebar->count(), 3 );

    QListWidgetItem *row = sidebar->item( 2 );
    QCOMPARE( row->text(), QStringLiteral( "Camila Cabello - Havana" ) );

    // No id, because there is no playlist to point at. Everything that asks a
    // row which playlist it is gets 0 and steps aside.
    QCOMPARE( row->data( PlaylistRoles::Id ).toLongLong(), qint64( 0 ) );
    QVERIFY( row->data( PlaylistRoles::Searching ).toBool() );
    QVERIFY( !row->toolTip().isEmpty() );

    // Found by position like any other row, but through the door meant for it.
    const QPoint centre = sidebar->visualItemRect( row ).center();
    QCOMPARE( sidebar->pendingAt( centre ), hash );
    QCOMPARE( sidebar->playlistAt( centre ), qint64( 0 ) );
    QVERIFY( sidebar->pendingAt( sidebar->visualItemRect( sidebar->item( 0 ) ).center() ).isEmpty() );

    // The same magnet pasted twice is one row, not two. So is a magnet whose
    // torrent the catalogue already has: something on screen already stands
    // for it, and a second row would be the sidebar disagreeing with itself.
    sidebar->addPending( hash, QStringLiteral( "Camila Cabello - Havana" ) );
    QCOMPARE( sidebar->count(), 3 );
    QVERIFY( sidebar->hasRow( hash ) );
    QVERIFY( !sidebar->hasRow( QStringLiteral( "2222222222222222222222222222222222222222" ) ) );

    // Nothing open must not land on it either. Provisional rows carry an id of
    // 0, and selectPlaylist() used to have no reason to guard against that.
    sidebar->setCurrentRow( 0 );
    sidebar->selectPlaylist( 0 );
    QCOMPARE( sidebar->currentRow(), 0 );

    sidebar->selectPending( hash );
    QCOMPARE( sidebar->currentPendingHash(), hash );

    sidebar->setCurrentRow( 0 );
    QVERIFY( sidebar->currentPendingHash().isEmpty() );

    delete sidebar;
}

/*!
  \brief stopsBlinkingWhenTheLookingIsOver

  The blinking is what tells a row that is waiting apart from one that is
  asleep, and the two static states were both already taken. It is driven by a
  timer flipping the Health role, so what can be asked here is whether the led
  actually changes on its own — and, just as important, whether it stops.
*/
void PlaylistTest::stopsBlinkingWhenTheLookingIsOver()
{
    PlaylistList *sidebar = buildSidebar( 1, 2 );

    const QString hash = QStringLiteral( "3333333333333333333333333333333333333333" );
    sidebar->addPending( hash, QStringLiteral( "Something slow" ) );

    QListWidgetItem *row = sidebar->item( 2 );
    const int first = row->data( PlaylistRoles::Health ).toInt();

    QTRY_VERIFY_WITH_TIMEOUT( row->data( PlaylistRoles::Health ).toInt() != first, 3000 );

    const QString searchingTip = row->toolTip();

    sidebar->setPendingSearching( hash, false );

    QVERIFY( !row->data( PlaylistRoles::Searching ).toBool() );
    QCOMPARE( row->data( PlaylistRoles::Health ).toInt(), static_cast<int>( GrooveOff::SilentSource ) );

    // What it says changes with it: "I am looking" and "I looked and nobody
    // answered" are different things to be told.
    QVERIFY( row->toolTip() != searchingTip );

    // And it stays there. A timer left ticking over a sidebar where nothing is
    // waiting would go on for as long as the program is open, and the led would
    // never settle on the answer it has.
    QTest::qWait( 1500 );
    QCOMPARE( row->data( PlaylistRoles::Health ).toInt(), static_cast<int>( GrooveOff::SilentSource ) );

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
