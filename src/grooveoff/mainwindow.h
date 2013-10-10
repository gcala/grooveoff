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

#include <QMainWindow>
#include <QNetworkCookieJar>
#include <QNetworkConfigurationManager>
#include <phononnamespace.h>
#include <libgrooveshark/apirequest.h>
#include "songitem.h"

class QLabel;
class QPushButton;
class QLed;
class QLineEdit;
class QListView;
class QListWidget;
class QSplitter;
class AudioPlayer;
class QActionGroup;
class QNetworkAccessManager;
class SongObject;
class QBoxLayout;
class DownloadItem;
class MatchItemListDelegate;
class CoverManager;

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

private slots:
    void selectFolder();
    void addDownloadItem(SongItemPtr song);
    void beginSearch();
    void getToken();
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
    void tokenReturned();
    void populateResultsList();

    void errorDuringToken();

private:
    Ui::MainWindow *ui_;
    QMovie *busyAnimation_;
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
    bool showHistory_;
    bool saveDestination_;
    bool loadCovers_;
    bool searchInProgress_;
    int parallelDownloadsCount_;
    QList<DownloadItem *> queue_;
    AudioPlayer *playerWidget;
    CoverManager *cvrMngr_;

    QNetworkAccessManager *nam_;
    GrooveShark::ApiRequest *api_;
    GrooveShark::TokenPtr token_;
    GrooveShark::SongListPtr songList_;

    //Menus
    QMenu *fileMenu_;
    QMenu *downloadsMenu_;
    QMenu *viewMenu_;
    QMenu *settingsMenu_;
    QMenu *helpMenu_;
    QMenu *layoutMenu_;

    //Actions
    QAction *actionClose_;
    QAction *actionDonate_;
    QAction *actionAbout_;
    QAction *actionQtAbout_;
    QAction *actionNewToken_;
    QAction *actionConfigure_;
    QAction *actionCompact_;
    QAction *actionWide_;
    QAction *actionRemoveFailed_;
    QAction *actionClearDownloadList_;
    QActionGroup *layoutGroup_;

    // Methods
    void setupUi();
    void setupActions();
    void setupMenus();
    void setupCompleter();
    void loadSettings();
    void saveSettings();
    void unqueue();
    void applyFilter();
    bool isDownloadingQueued(const uint &);
    int visibleItemsCount();
    void reloadItemsDownloadButtons();
};

#endif // MAINWINDOW_H
