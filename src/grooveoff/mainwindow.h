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
#include <QNetworkRequest>
#include <QVariantMap>
#include <QVariantList>
#include <QNetworkConfigurationManager>

class QNetworkReply;
class QModelIndex;
class QLabel;
class QPushButton;
class QLed;
class QLineEdit;
class QListView;
class QListWidget;
class QSplitter;
class AudioPlayer;
class QActionGroup;
class MatchesTableModel;
class MatchesListModel;
class QNetworkAccessManager;
class QTableWidgetItem;
class Song;
class QBoxLayout;
class FilterProxyModel;
class DownloadItem;
class MatchItemListDelegate;

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
    void replyFinished(QNetworkReply*);
    void addDownloadItem(const QModelIndex &index);
    void beginSearch();
    void newToken();
    void setCompactLayout();
    void setWideLayout();
    void about();
    void configure();
    void onlineStateChanged(bool isOnline);
    void donate();
    void artistChanged();
    void albumChanged();
    void freeDownloadSlot();
    void removeFailedDeletedAborted();
    void removeDownloaded();
    void addItemToQueue(DownloadItem *);
    void reloadItemsCover();

private:
    Ui::MainWindow *ui_;
    QMovie *busyAnimation_;
    QNetworkConfigurationManager *qncm_;
    QList<QPair<QString, QStringList> > artistsAlbumsContainer_;
    FilterProxyModel *proxyModel_;
    MatchesListModel *listModel_;
    MyJar *jar_;
    QNetworkAccessManager *qnam_;
    int currentJob_;
    QString dir_;
    QString token_;
    QNetworkRequest tokenRequest_;
    QNetworkRequest mainRequest_;
    QVariantMap header_;
    QVariantList results_;
    int row_;
    QString currentSongID_;
    QList<Song *> songs_;
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
    MatchItemListDelegate *matchItemListDelegate_;

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
    void getToken();
    void getResultsFromSearch(const QString &query, const QString &what="Songs");
    QString randomHex(const int &length);
    void setTokenRequest();
    void setMainRequest();
    void populateResultsTable();
    void applyFilter();
    bool isDownloadingQueued(const QString &);
};

#endif // MAINWINDOW_H
