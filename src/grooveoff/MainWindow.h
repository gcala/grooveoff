/*
    GrooveOff - Offline Grooveshark.com music
    Copyright (C) 2013  Giuseppe Calà <jiveaxe@gmail.com>

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
class QDomElement;

//Custom
class PlayerWidget;
class SongObject;
class DownloadItem;
class MatchItemListDelegate;
class CoverManager;
class Spinner;

// Dbus & Mpris
class DbusNotification;
class MprisManager;

namespace Ui {
class MainWindow;
}

class MyJar : public QNetworkCookieJar
{
public:
    QList<QNetworkCookie> getAllCookies() { return allCookies(); }
    void clear() { setAllCookies(QList<QNetworkCookie>());}
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    virtual ~MainWindow();

protected:
        virtual void changeEvent( QEvent *event );

private slots:
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
    void about();
    void configure();
    void onlineStateChanged(bool isOnline);
    void donate();
    void artistChanged();
    void albumChanged();
    void freeDownloadSlot();
    void addItemToQueue(DownloadItem *);
    void batchDownload();

private:
    Ui::MainWindow *ui_;
    QNetworkConfigurationManager *qncm_;
    QList<QPair<QString, QStringList> > artistsAlbumsContainer_;
    MyJar *jar_;
    int currentJob_;
    int row_;
    QString currentSongID_;
    QStringList searchList_;
    int searchSize_;
    int maxResults_;
    int maxDownloads_;
    bool saveSession_;
    QString sessionFile_;
    bool showHistory_;
    bool saveDestination_;
    bool loadCovers_;
    bool searchInProgress_;
    int parallelDownloadsCount_;
    QList<DownloadItem *> queue_;
    PlayerWidget *playerWidget;
    CoverManager *cvrMngr_;
    Spinner *spinner_;

    QNetworkAccessManager *nam_;
    GrooveShark::ApiRequest *api_;
    GrooveShark::TokenPtr token_;
    GrooveShark::SongListPtr songList_;

    // Menus and menu actions: Accounts menu
    QMenuBar    *m_menuBar;
    QAction     *m_compactMenuAction;
    QMenu       *m_compactMainMenu;

    MprisManager *m_mpris_manager;
    DbusNotification *m_dbus_notifier;

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
    void reloadItemsDownloadButtons();
    void loadSession();
    void saveSession();
    void parsePlaylistItem(const QDomElement & element, PlaylistItemPtr item);
    void parseSong(const QDomElement & element, GrooveShark::SongPtr song);
    void addDownloadItem(PlaylistItemPtr playlistItem);
};

#endif // MAINWINDOW_H
