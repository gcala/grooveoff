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

#include "../libgrooveshark/apirequest.h"
#include "PlaylistItem.h"

// Dbus & remote
#include "dbus/mpris.h"

#include <QPointer>
#include <QMainWindow>
#include <QNetworkCookie>
#include <QNetworkCookieJar>
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

class MyJar : public QNetworkCookieJar
{
public:
    QList<QNetworkCookie> getAllCookies() { return allCookies(); }
    void clear() { setAllCookies(QList<QNetworkCookie>());}
};

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

    explicit MainWindow(QWidget *parent = 0);
    virtual ~MainWindow();

    void reloadItemsDownloadButtons();

protected:
    virtual void changeEvent( QEvent *event );

private Q_SLOTS:
    void selectFolder();
    void downloadRequest(PlaylistItemPtr playlistItem);
    void getToken();
    void tokenFinished();
    void tokenError();
    void beginSearch();
    void searchFinished();
    void searchError();
    void setCompactLayout();
    void setWideLayout();
    void setMiniPlayerLayout();
    void about();
    void configure();
    void onlineStateChanged(bool isOnline);
    void donate();
    void artistChanged();
    void albumChanged();
    void freeDownloadSlot();
    void addItemToQueue(DownloadItem *);
    void batchDownload();
    void changeDestinationPath();
    void saveSessionAs();
    void loadSessionFile();
    void openSessionManager();

private:
    Ui::MainWindow *ui;
    QNetworkConfigurationManager *m_qncm;
    QList<QPair<QString, QStringList> > m_artistsAlbumsContainer;
    MyJar *m_jar;
    int m_currentJob;
    int m_row;
    QString m_currentSongID;
    QStringList m_searchList;
    int m_searchSize;
    int m_maxResults;
    int m_maxDownloads;
    bool m_saveSession;
    QString m_sessionFileName;
    QString m_sessionFilePath;
    bool m_showHistory;
    bool m_saveDestination;
    bool m_loadCovers;
    bool m_searchInProgress;
    int m_parallelDownloadsCount;
    QList<DownloadItem *> m_queue;
    PlayerWidget *m_playerWidget;
    CoverManager *m_cvrMngr;
    Spinner *m_spinner;
    GuiLayout m_guiLayout;
    bool m_sessionChanged;
    bool m_batchDownload;
    bool m_stopBatchDownload;

    QNetworkAccessManager *m_nam;
    GrooveShark::ApiRequest *m_api;
    GrooveShark::TokenPtr m_token;
    GrooveShark::SongListPtr m_songList;

    // Menus and menu actions: Accounts menu
    QMenuBar    *m_menuBar;
    QAction     *m_compactMenuAction;
    QMenu       *m_compactMainMenu;

    Mpris *m_mpris;

    static QPointer<MainWindow> s_instance;

    // Methods
    void setupUi();
    void setupMenus();
    void setupSignals();
    void setupCompleter();
    void loadSettings();
    void saveSettings();
    void unqueue();
    void applyFilter();
    void restoreSearch();
    bool isDownloadingQueued(const uint &);
    int visibleItemsCount();
    void loadSession();
    void loadSessions();
    void saveSession();
    void addDownloadItem(PlaylistItemPtr playlistItem);
};

Q_DECLARE_METATYPE( MainWindow::GuiLayout )

#endif // MAINWINDOW_H
