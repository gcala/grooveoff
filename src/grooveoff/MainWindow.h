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
    void setupPaths();
    bool isOnline() const;
    void loadSettings();
    void loadBootSettings();
    void saveSettings();

    /** rebuilds the list from the playlist app_state points at */
    void loadCurrentPlaylist();
    void savePlaybackState();
    void addPlaylistItem( const PlaylistItemPtr &playlistItem );

    Mpris *m_mpris;

    static QPointer< MainWindow > s_instance;
};

Q_DECLARE_METATYPE( MainWindow::GuiLayout )

#endif // MAINWINDOW_H
