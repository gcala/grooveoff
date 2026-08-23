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
#include "../db/PlaylistStore.h"

// Dbus & remote
#include "dbus/mpris.h"

#include <QHash>
#include <QPointer>
#include <QMainWindow>
#include <QSet>
#include "GrooveOffNamespace.h"

//Qt
class QAction;
class QLabel;
class QPushButton;
class QLineEdit;
class QListWidget;
class QListWidgetItem;
class QMessageBox;
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

    /**
     * The tracks one gesture on this row acts on: the whole selection when the
     * row is part of it, that row alone otherwise.
     *
     * Public because the row's own menu has to ask the same question the window
     * will answer with — a menu offering the lists that are missing one track
     * beside an action that adds five is two rules for one gesture.
     */
    QList<TrackInfoPtr> tracksForGesture( TrackRow *row ) const;

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

    /**
     * Hides from the playlist menu what the playlist the sidebar is pointing at
     * cannot do. Hidden and not greyed out: a disabled entry says "not now",
     * and there is no now in which «Choose tracks» will apply to a list somebody
     * filled by hand — the files it offers to choose between belong to a
     * torrent, and that list has no torrent.
     *
     * There is one rule and both menus read the answer off the actions
     * themselves, since adding a hidden action to a menu shows nothing. The
     * menu bar used to offer all four whatever was selected, so on a custom
     * playlist «Choose tracks» was there and did nothing at all when clicked.
     */
    void updatePlaylistActions();

    /** the lists a whole torrent can be poured into, rebuilt at every row */
    void fillAddToPlaylistMenu( const PlaylistStore::PlaylistInfo &playlist );

    /** tracks dragged onto a playlist of the sidebar */
    void addTracksToPlaylist( qint64 playlistId, const QList<qint64> &trackIds );

    /** the same thing from the row's own menu, one track at a time */
    void addTrackToPlaylist( TrackRow *item, qint64 playlistId );

    /** the whole of a torrent's playlist into a list made by hand */
    void addShownTorrentToPlaylist( QAction *chosen );

    /**
     * A row asked to be taken out of the playlist. What that means depends on
     * the kind of list open, which is why the row asks instead of acting: out
     * of a list made by hand it is only that, out of a torrent's playlist it is
     * the removal of a track from everywhere, bytes included.
     */
    void removeTracksRequested( TrackRow *row );

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

    /** starts the playlist on screen from its first track; see the definition */
    void playShownPlaylist();

    /**
     * Builds the queue out of the playlist that was playing when the window
     * last closed and tells the engine where in the track to carry on from, so
     * that pressing play resumes rather than doing nothing. From the
     * constructor only — see the note on it.
     */
    void restoreWhatWasPlaying();

    /**
     * Puts the mark on the sidebar row the music is coming from, or takes it
     * off when it is coming from nowhere. Asked again at every change of state
     * rather than decided once when play is pressed: a playlist reaches its
     * end without anybody choosing anything.
     */
    void updatePlayingMark();

    /**
     * The playlist the music is coming from; 0 when nothing was ever started.
     * It outlives the playing — a finished playlist is still the one to come
     * back to, and that is what app_state remembers — so it is not what says
     * whether the mark is drawn. See updatePlayingMark().
     */
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

    /**
     * And the ones that still have chosen tracks to fetch — if fetching ahead
     * is on and the cache has room for it, both asked inside rather than by
     * whoever calls. Connected to DownloadQueue::fetchingResumed as well, which
     * is what brings the sleepers back when either answer turns to yes.
     */
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
     * The rows one gesture acts on: the whole selection when the row it was
     * made on is part of it, that row alone otherwise. Ten rows highlighted and
     * one of them removed is a gesture that looks broken.
     */
    QList<TrackRow *> rowsForGesture( TrackRow *row ) const;

    /**
     * The one question asked before any track is thrown away, naming the lists
     * made by hand that would lose something. True when there is nothing to ask
     * about, which is the common case and the one that must not cost a click.
     */
    bool confirmDiscard( const QList<TrackInfoPtr> &tracks );

    /**
     * Takes these tracks out of every list and throws their bytes away. The one
     * place that does it, because the bin, the row's menu and the selector all
     * mean the same thing by it and a second implementation is a second chance
     * to leave bytes on the disk with nothing pointing at them.
     *
     * Asks nothing and reloads nothing: the question belongs to the gesture,
     * and the sidebar is rebuilt once by the caller however many tracks went.
     */
    void discardTracks( const QList<TrackInfoPtr> &tracks );

    /** takes these items out of the play queue, the one playing last of all */
    void detachFromPlayer( const QList<PlaylistItemPtr> &items );

    /** offers to remove a torrent whose playlist has just been emptied */
    void offerToRemoveEmptied( const QString &infoHash, const QString &name );

    /** what is known about a magnet while its track list is still being looked for */
    struct PendingAdd
    {
        /** what the magnet calls itself, which is all there is to go on until the metadata land */
        QString name;

        /**
         * Kept here rather than asked of the session at the end, because the
         * end is where it is needed and the session is being taken down around
         * the same time.
         */
        QString magnetUri;

        /** whether the led is still blinking: the sixty seconds turn it off */
        bool searching = true;

        /**
         * Whether the question has already been put. Kept apart from the
         * blinking because a magnet somebody chose to go on looking for is
         * looked for again at every start, and asking again each time would be
         * asking a question that has been answered.
         */
        bool asked = false;

        /**
         * The question asked at that point, while it is still on screen. Held
         * so that it can be taken away: metadata arriving late open the
         * selector, and a stale «Remove» left behind it would throw away a
         * torrent whose tracks have just been chosen.
         */
        QPointer<QMessageBox> question;
    };

    /**
     * Torrents the user has just added by hand. Only those open the selector:
     * one woken up to play a track must not stop to ask questions.
     *
     * An entry outlives the metadata timeout on purpose. The torrent stays in
     * the session and goes on announcing, so its track list may still arrive —
     * and dropping the entry at the timeout, which is what used to happen, left
     * whoever pasted the magnet with an empty unexplained row instead of the
     * selector they asked for.
     */
    QHash<QString, PendingAdd> m_awaitingSelection;

    /** what to call a torrent whose metadata have not arrived: its magnet's name, or a stand-in */
    QString pendingName( const QString &infoHash ) const;

    /** the row of a magnet still being looked for, put back after every rebuild */
    void addPendingRows();

    /**
     * Picks up the magnets somebody chose to go on looking for and starts
     * looking again. They are the ones with a row in the catalogue and no
     * playlist, which is what a torrent whose metadata never arrived looks
     * like; nothing is asked about them, since the question was answered the
     * day they were left waiting.
     */
    void resumeMagnetSearches();

    /** the question at the end of the wait, asked without blocking anything */
    void askAboutSlowMagnet( const QString &infoHash );

    /** writes the magnet down so the looking survives the program being closed */
    void keepLookingFor( const QString &infoHash );

    /**
     * And writes down every magnet still being looked for when the program is
     * closed. Closing early — inside the first minute, or with the question
     * still on screen — is taken as wanting to keep the torrent until either a
     * source turns up or it is removed by hand, so the only way a magnet is
     * forgotten is somebody saying so.
     */
    void rememberPendingMagnets();

    /** gives up on a magnet: out of the session, off the sidebar, nothing left behind */
    void stopLookingFor( const QString &infoHash );

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

    /**
     * Tracks whose cover question is settled: one was found, or the file is
     * whole and there is none. Kept apart from the tags because the two answers
     * arrive at different moments — the tags sit in the first kilobytes and a
     * picture is tens of them, so a cover read as early as the tags comes back
     * cut in half.
     */
    QSet<QString> m_coversRead;

    /** what the torrent being listened to had last time, to see it progressing */
    QString m_lastStatsHash;
    qint64  m_lastBytesDone = -1;

    Mpris *m_mpris;

    static QPointer< MainWindow > s_instance;
};

Q_DECLARE_METATYPE( MainWindow::GuiLayout )

#endif // MAINWINDOW_H
