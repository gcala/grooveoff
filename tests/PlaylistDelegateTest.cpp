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
#include "widgets/PlaylistDelegate.h"

#include <QListWidget>
#include <QStyleOptionViewItem>
#include <QTest>

/**
 * @class PlaylistDelegateTest
 * @short The sidebar rows, and where the led of each of them ends up.
 *
 * Two questions, and the first one is here because getting it wrong is
 * invisible in every way a test usually looks: a row wider than the viewport
 * draws its led past the right edge, so the code runs, the painting happens,
 * and nothing appears. It cost half an hour of looking at an empty margin.
 */
class PlaylistDelegateTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void keepsEveryRowInsideTheViewport();
    void drawsNothingForAPlaylistMadeByHand();

private:
    static QListWidgetItem* add( QListWidget *list, const QString &name, int health )
    {
        QListWidgetItem *row = new QListWidgetItem( name );

        if( health >= 0 )
            row->setData( PlaylistRoles::Health, health );

        list->addItem( row );
        return row;
    }
};

/*!
  \brief keepsEveryRowInsideTheViewport: a long name must not carry the led away

  The view makes every row as wide as the longest name in the list, and a
  torrent name is routinely longer than a sidebar is wide. Left alone the row
  ends past the viewport, and the led — drawn at the right edge of the row —
  goes with it, behind a horizontal scrollbar.
*/
void PlaylistDelegateTest::keepsEveryRowInsideTheViewport()
{
    QListWidget list;
    PlaylistDelegate delegate;
    list.setItemDelegate( &delegate );
    list.resize( 200, 160 );

    add( &list, QStringLiteral( "Short one" ), GrooveOff::LiveSource );
    QListWidgetItem *longRow =
        add( &list, QStringLiteral( "The Complete Recordings 1959-1972, Remastered [FLAC] "
                                    "disc one of two, remastered" ),
             GrooveOff::SilentSource );

    list.show();
    QVERIFY( QTest::qWaitForWindowExposed( &list ) );

    QStyleOptionViewItem option;
    option.initFrom( &list );
    option.widget = &list;

    const int width = delegate.sizeHint( option, list.indexFromItem( longRow ) ).width();

    QVERIFY2( width <= list.viewport()->width(),
              qPrintable( QStringLiteral( "a row %1 wide in a viewport of %2 puts its led out of sight" )
                          .arg( width ).arg( list.viewport()->width() ) ) );

    // And what the view itself ended up with, which is the thing actually seen.
    QCOMPARE( list.visualItemRect( longRow ).right() <= list.viewport()->width(), true );
}

/*!
  \brief drawsNothingForAPlaylistMadeByHand: no health, no led

  A custom playlist holds tracks of several torrents, so the health of "the"
  source is a question without an answer for it. The absence of the role is
  what says so, and it must also cost the row nothing in width.
*/
void PlaylistDelegateTest::drawsNothingForAPlaylistMadeByHand()
{
    QListWidget list;
    PlaylistDelegate delegate;
    list.setItemDelegate( &delegate );
    list.resize( 200, 160 );

    QListWidgetItem *custom = add( &list, QStringLiteral( "Preferite" ), -1 );

    list.show();
    QVERIFY( QTest::qWaitForWindowExposed( &list ) );

    QVERIFY( !custom->data( PlaylistRoles::Health ).isValid() );

    QStyleOptionViewItem option;
    option.initFrom( &list );
    option.widget = &list;

    QCOMPARE( delegate.sizeHint( option, list.indexFromItem( custom ) ),
              QStyledItemDelegate().sizeHint( option, list.indexFromItem( custom ) ) );
}

QTEST_MAIN( PlaylistDelegateTest )

#include "PlaylistDelegateTest.moc"
