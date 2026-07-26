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
#include "GrooveOffNamespace.h"

//Qt
class QLabel;
class QPushButton;
class QLineEdit;
class QListWidget;
class QSplitter;
class QBoxLayout;

//Custom
class PlayerWidget;
class DownloadItem;
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

    /** rebuilds the list from the playlist app_state points at */
    void loadCurrentPlaylist();
    void savePlaybackState();
    void addPlaylistItem( const PlaylistItemPtr &playlistItem );

    /** hands the torrent of the track that was playing back to the session */
    void wakeLastUsedTorrent();

    /** brings the rows of one torrent out of the dormant state */
    void refreshRowsOf( const QString &infoHash );

    /** adds a magnet or a .torrent, and says so when it will not go in */
    void addTorrentSource( const QString &source );

    Mpris *m_mpris;

    static QPointer< MainWindow > s_instance;
};

Q_DECLARE_METATYPE( MainWindow::GuiLayout )

#endif // MAINWINDOW_H
