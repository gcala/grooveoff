/*
    GrooveOff - Offline Grooveshark.com music
    Copyright (C) 2013-2014  Giuseppe Calà <jiveaxe@gmail.com>

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


#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "PlaylistItem.h"

// Dbus & remote
#include "dbus/mpris.h"

#include <QPointer>
#include <QMainWindow>
#include <QSet>
#include "GrooveOffNamespace.h"

//Qt
class QLabel;
class QPushButton;
class QLineEdit;
class QListWidget;
class QListWidgetItem;
class QSplitter;
class QTimer;
class QBoxLayout;

//Custom
class PlayerWidget;
class TrackRow;
class Spinner;

namespace Ui {
class MainWindow;
}

class MainWindow;

namespace The {
    MainWindow* mainWindow();
}

/**
  * @class MainWindow
  * @short The MainWindow widget class.
  *
  * This is the main window widget.
  */
class MainWindow : public QMainWindow
{
    Q_OBJECT
    friend MainWindow* The::mainWindow();

public:
    enum GuiLayout {
        Compact = 0,
        Wide = 1,
        Mini = 2
    };
    Q_ENUM( GuiLayout )

    explicit MainWindow( QWidget *parent = nullptr );
    ~MainWindow() override;

    /**
     * Writes the playlist, the track and the millisecond that were playing.
     *
     * Public because the order it happens in is not this window's to decide:
     * the position is read off the player, so it has to be asked for before
     * anything stops the player, and what stops it is the shutdown in App.
     */
    void savePlaybackState();

protected:
    void changeEvent( QEvent *event ) override;
    void resizeEvent( QResizeEvent *event ) override;

private Q_SLOTS:
    /** takes what was typed in the magnet field and hands it to the session */
    void addFromMagnetField();

    void openTorrentFile();

    /** writes a torrent and its files into the catalogue, then shows them */
    void onMetadataReady( const QString &infoHash, const QString &name, const QList<TrackInfoPtr> &tracks );

    void onTorrentStats( const QString &infoHash, int peers, int seeds, qint64 rate, qint64 bytesDone );
    void onTorrentError( const QString &infoHash, const QString &message );

    /** the disk said no, which is the one thing worth a modal */
    void onStorageError( const QString &infoHash, const QString &message );

    /**
     * Asks what of the torrent goes into its playlist, and remembers the
     * answer. @p justAdded says whether this is the selector that follows an
     * add — the only case in which cancelling means "I do not want this
     * torrent after all" rather than "leave it as it was".
     */
    void chooseTracksOf( const QString &infoHash, bool justAdded = false );

    /** reopens the selector on the playlist the sidebar is pointing at */
    void chooseTracksAgain();

    /** the sticky refusal, as opposed to taking a row out of the playlist */
    void excludeTrack( TrackRow *item );

    /** writes the rows, in the order they are in, into the open playlist */
    void savePlaylistOrder();

    /** reads the tags of a track whose head has just arrived */
    void readTagsOf( const QString &infoHash, int fileIndex );

    /** and of every row of this torrent whose file has landed since last time */
    void readArrivedTags( const QString &infoHash = QString() );

    /** throws the cached bytes away, keeping every playlist as it is */
    void emptyCache();

    // --- the sidebar --------------------------------------------------------

    /** rebuilds the sidebar from what the store says is worth showing */
    void loadPlaylists();

    /** the user chose another playlist: it becomes the open one */
    void openPlaylist( QListWidgetItem *row );

    void newPlaylist();
    void renamePlaylist();

    /** takes every track out of a playlist made by hand, keeping the list */
    void emptyPlaylist();

    /**
     * The one operation of this whole application that can delete something the
     * user made by hand, so it asks first and says what is at stake.
     */
    void removePlaylist();

    void showPlaylistMenu( const QPoint &position );

    /** tracks dragged onto a playlist of the sidebar */
    void addTracksToPlaylist( qint64 playlistId, const QList<qint64> &trackIds );

    /** the same thing from the row's own menu, one track at a time */
    void addTrackToPlaylist( TrackRow *item, qint64 playlistId );

    /** a drag that landed back in the track list: an order, not a move */
    void reorderTracks( const QList<int> &rows, int targetRow );

    void setCompactLayout();
    void setWideLayout();
    void setMiniPlayerLayout();
    void about();
    void configure();
    void onlineStateChanged( bool isOnline );
    void donate();

private:
    Ui::MainWindow *ui;

    // widgets
    PlayerWidget *m_playerWidget;
    Spinner      *m_spinner;

    // settings
    GuiLayout m_guiLayout;

    // Menus and menu actions
    QMenuBar    *m_menuBar;
    QMenu       *m_compactMainMenu;

    // Methods
    void setupUi();
    void setupMenus();
    void setupConnections();
    void setupGuiLayout();
    bool isOnline() const;
    void loadSettings();
    void loadBootSettings();
    void saveSettings();

    /** the saved split, when it matches the splitter this window actually has */
    void restoreSplitterSizes();

    /**
     * The playlist on screen, which is the row the sidebar is pointing at.
     *
     * Told apart from the one being played, which is what app_state remembers
     * and what The::playlist() walks. They used to be the same thing and the
     * plan said they should be — "what plays is the open playlist" — but that
     * makes looking at a second playlist an act of stopping the first, and
     * looking around while the music goes on is the ordinary thing to want.
     */
    qint64 shownPlaylistId() const;

    /** rebuilds the list from the playlist the sidebar is pointing at */
    void loadCurrentPlaylist();

    /**
     * Fills The::playlist() — the queue the engine walks — from the playlist
     * playback started in, whatever is on screen. Rebuilt rather than kept up
     * to date piece by piece, because it also has to pick up tracks added to
     * that playlist after the music started.
     *
     * The rows on screen are preferred as the items, when they are the same
     * playlist: a row shows whether it is playing by being the very object the
     * engine holds, so a queue of copies would leave the list looking idle
     * while it plays.
     */
    void rebuildPlayQueue();

    /** the playlist the music is coming from; 0 when nothing is playing */
    qint64 m_playingPlaylistId = 0;

    /**
     * The playlist the track list is actually showing, which is not the same
     * as the row the sidebar is pointing at: openPlaylist() is called from
     * currentItemChanged, by which time the selection has already moved. A
     * guard that compared the two would find them equal every time and return
     * without drawing anything, which is a sidebar where clicking does nothing.
     */
    qint64 m_shownPlaylistId = 0;
    void addPlaylistItem( const PlaylistItemPtr &playlistItem );

    /** hands the torrent of the track that was playing back to the session */
    void wakeLastUsedTorrent();

    /** and the ones that still have chosen tracks to fetch, if fetching ahead is on */
    void wakeUnfinishedTorrents();

    /** brings the rows of one torrent out of the dormant state */
    void refreshRowsOf( const QString &infoHash );

    /** the row showing this track, if it is in the open playlist */
    TrackRow* rowFor( const TrackInfoPtr &track ) const;

    /** adds a magnet or a .torrent, and says so when it will not go in */
    void addTorrentSource( const QString &source );

    /** plain yes or no, then everything goes: rows, files and resume */
    void removeTorrentEverywhere( const QString &infoHash );

    /** whether the open playlist is one the user built by hand */
    bool openPlaylistIsCustom() const;

    /**
     * Torrents the user has just added by hand. Only those open the selector:
     * one woken up to play a track must not stop to ask questions.
     */
    QSet<QString> m_awaitingSelection;

    /** turns a buffering that never ends into a row that says it is waiting */
    QTimer *m_stallTimer;

    /** the track the rows are being marked for, so the last one can be let go */
    TrackInfoPtr m_playingTrack;

    /**
     * Rows already asked what their file says, by info hash and file index.
     * Asked once and not again: once the head of a file is there its answer
     * stops changing, and a file with no tags at all has none to find.
     */
    QSet<QString> m_tagsRead;

    /** what the torrent being listened to had last time, to see it progressing */
    QString m_lastStatsHash;
    qint64  m_lastBytesDone = -1;

    Mpris *m_mpris;

    static QPointer< MainWindow > s_instance;
};

Q_DECLARE_METATYPE( MainWindow::GuiLayout )

#endif // MAINWINDOW_H
