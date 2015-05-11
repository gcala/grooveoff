/*
    GrooveOff - Offline Grooveshark.com music
    Copyright (C) 2013-2015  Giuseppe Calà <jiveaxe@gmail.com>

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

#include "../groovesharkcom/apirequest.h"
#include "PlaylistItem.h"

// Dbus & remote
#include "dbus/mpris.h"

#include <QPointer>
#include <QMainWindow>
#include <QNetworkCookie>
#include <QNetworkConfigurationManager>
#include <phonon/phononnamespace.h>

//Qt
class QLabel;
class QPushButton;
class QLed;
class QLineEdit;
class QListView;
class QListWidget;
class QSplitter;
class QActionGroup;
class QBoxLayout;
class QNetworkAccessManager;

//Custom
class PlayerWidget;
class DownloadItem;
class MatchItemListDelegate;
class CoverManager;
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
    Q_ENUMS( GuiLayout )
    friend MainWindow* The::mainWindow();

public:
    enum GuiLayout {
        Compact = 0,
        Wide = 1,
        Mini = 2
    };

    explicit MainWindow( QWidget *parent = 0 );
    virtual ~MainWindow();

    void reloadItemsDownloadButtons();

protected:
    virtual void changeEvent( QEvent *event );
    virtual void resizeEvent( QResizeEvent *event );

private Q_SLOTS:
    void selectFolder();
    void downloadPlaylistItem( PlaylistItemPtr playlistItem );
    void getToken();
    void tokenRequestFinished();
    void tokenRequestError();
    void beginSearch();
    void searchFinished();
    void searchError();
    void setCompactLayout();
    void setWideLayout();
    void setMiniPlayerLayout();
    void about();
    void configure();
    void onlineStateChanged( bool isOnline );
    void donate();
    void artistChanged();
    void albumChanged();
    void freeDownloadSlot();
    void addItemToQueue( DownloadItem * );
    void batchDownload();
    void changeDestinationPath();
    void saveSessionAs();
    void loadSessionFile();
    void openSessionManager();

private:
    Ui::MainWindow *ui;
    
    // widgets
    PlayerWidget *m_playerWidget;
    CoverManager *m_coverManager;
    Spinner      *m_spinner;
    
    QNetworkConfigurationManager *m_qncm; // used to monitor internet connection status
    QHash<QString, QStringList> m_artistsAlbumsContainer; // artists/album container
    QString m_sessionFilePath; // contains the session file path (depends on qt internals)
    int m_parallelDownloadsCount; // holds how many downloads are running
    QList< DownloadItem * > m_queue; // download queue
    
    bool m_searchInProgress;  // true if search button is clicked
    bool m_batchDownload;     // true if "download all" button is clicked
                              // when true all warnings (qmessagebox) are suppressed and a default action is performed
    bool m_stopBatchDownload; // flag to exit from batch download (avoids adding items to download queue) if a serious error happens

    // GroovesharkCom
    GroovesharkCom::ApiRequest *m_api;
    GroovesharkCom::TokenPtr m_token;
    GroovesharkCom::SongListPtr m_songList;

    // Menus and menu actions: Accounts menu
    QMenuBar    *m_menuBar;
    QMenu       *m_compactMainMenu;
    
    // settings
    bool m_saveSession;
    bool m_saveAborted;
    bool m_showHistory;
    bool m_emptyCache;
    bool m_loadCovers;
    bool m_saveDestination;
    int m_maxResults;
    int m_maxDownloads;
    int m_searchSize;
    QString m_sessionFileName;
    QStringList m_searchList;
    GuiLayout m_guiLayout;
    

    // Methods
    void setupUi();
    void setupMenus();
    void setupConnections();
    void setupCompleter();
    void setupGuiLayout();
    void setupPaths();
    void initRandomNumberGenerator();
    void initPathLine();
    void loadSettings();
    void loadBootSettings();
    void saveSettings();
    void unqueue();
    void applyFilter();
    void restoreSearch();
    bool isItemQueued( const uint & );
    int visibleItemsCount();
    void loadSession();
    void loadSessions();
    void saveSession();
    void addDownloadItem( const PlaylistItemPtr &playlistItem );
    int numberOfElementsToShow( int searchSize );
    void appendArtistAlbum( const GroovesharkCom::SongPtr &song );
    
    Mpris *m_mpris;

    static QPointer< MainWindow > s_instance;
};

Q_DECLARE_METATYPE( MainWindow::GuiLayout )

#endif // MAINWINDOW_H
