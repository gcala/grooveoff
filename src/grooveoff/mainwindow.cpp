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

#include "mainwindow.h"
#include "audioplayer.h"
#include "matcheslistmodel.h"
#include "matchitemlistdelegate.h"
#include "qled.h"
#include "downloaditem.h"
#include "roles.h"
#include "configdialog.h"
#include "aboutdialog.h"
#include "filterproxymodel.h"
#include "utility.h"
#include "grooveoffnamespace.h"
#include "ui_mainwindow.h"

#include <QtGui/QLabel>
#include <QtGui/QMenu>
#include <QtGui/QMenuBar>
#include <QtGui/QAction>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QSettings>
#include <QSplitter>
#include <QListView>
#include <QListWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QDesktopServices>
#include <QTime>
#include <QFileDialog>
#include <QDir>
#include <QFile>
#include <qjson/serializer.h>
#include <qjson/parser.h>
#include <QMovie>
#include <QMessageBox>
#include <QApplication>
#include <QCompleter>
#include <QDirModel>

// version include
#include <../config-version.h>

Q_DECLARE_METATYPE(QList<int>)

/*!
  \brief MainWindow: this is the MainWindow constructor.
  \param parent: The Parent Widget
*/
MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui_(new Ui::MainWindow)
{
    ui_->setupUi(this);
    setupUi();
    setupActions();
    setupMenus();
    loadSettings();

    //fake
    searchInProgress_ = true;

    setWindowTitle(QString("GrooveOff %1").arg(GROOVEOFF_VERSION));

    if(ui_->splitter->orientation() == Qt::Vertical) {
        actionCompact_->setChecked(true);
    }
    else {
        actionWide_->setChecked(true);
    }

    listModel_ = new MatchesListModel(this);
    proxyModel_ = new FilterProxyModel(this);
    ui_->listView->setModel(proxyModel_);

    matchItemListDelegate_ = new MatchItemListDelegate(ui_->listView);
    ui_->listView->setItemDelegate(matchItemListDelegate_);
    //connecting delegate's signal to this class's slot
    connect(matchItemListDelegate_, SIGNAL(downloadRequest(QModelIndex)),
            this, SLOT(addDownloadItem(QModelIndex)));

    QTime time = QTime::currentTime();
    qsrand((uint)time.msec());

    setTokenRequest();
    setMainRequest();

    token_ = "None";

    qnam_ = new QNetworkAccessManager;
    connect(qnam_, SIGNAL(finished(QNetworkReply*)), this, SLOT(replyFinished(QNetworkReply*)));

    jar_ = new MyJar;
    qnam_->setCookieJar(jar_);

    qncm_ = new QNetworkConfigurationManager(this);
    connect(qncm_, SIGNAL(onlineStateChanged(bool)), this, SLOT(onlineStateChanged(bool)));
    if(qncm_->isOnline())
        newToken();

    parallelDownloadsCount_ = 0;

    QSettings settings;
    settings.setIniCodec( "UTF-8" );

    if(saveDestination_) {
        ui_->pathLine->setText(settings.value(QLatin1String("destination"), QDesktopServices::storageLocation(QDesktopServices::MusicLocation)).toString());
        if(ui_->pathLine->text().isEmpty())
            ui_->pathLine->setText(QDesktopServices::storageLocation(QDesktopServices::MusicLocation));
    } else {
        ui_->pathLine->setText(QDesktopServices::storageLocation(QDesktopServices::MusicLocation));
    }

    matchItemListDelegate_->setPath(ui_->pathLine->text());

    setGeometry(settings.value(QLatin1String("windowGeometry"), QRect(100,100,350,600)).toRect());
}

/*!
  \brief ~MainWindow: this is the MainWindow destructor.
*/
MainWindow::~MainWindow()
{
    delete qnam_;

    saveSettings();
}

/*!
  \brief setupUi: setup widgets
  \return void
*/
void MainWindow::setupUi()
{
    ui_->label0->setPixmap(QPixmap(QLatin1String(":/resources/grooveoff.png")));
    ui_->label0->setMinimumHeight(30);
    ui_->label0->setMaximumWidth(30);
    ui_->label0->setScaledContents(30);

    ui_->label1->setText(trUtf8("GrooveOff"));
    QFontMetrics fmTitle(Utility::font(QFont::Bold, 2));
    ui_->label1->setFont(Utility::font(QFont::Bold, 2));
    ui_->label2->setText(trUtf8("Offline Grooveshark.com music"));

    statusBar()->showMessage(trUtf8("Connecting...", 0));

    // Led
    ui_->qled->setFixedSize(QSize(24,24));

    QFontMetrics fmSystemFont(Utility::font(QFont::Bold));
    int fontHeight = fmSystemFont.height();

    ui_->label3->setText(trUtf8("Search:"));
    ui_->label3->setBuddy(ui_->searchLine);
    ui_->searchLine->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    ui_->searchLine->setFixedHeight(fontHeight > 24 ? fontHeight : 24);
    ui_->searchLine->setToolTip(trUtf8("Search for songs, artists, genres"));
    ui_->searchLine->setPlaceholderText(trUtf8("Search for songs, artists, genres"));

    ui_->searchButton->setIcon(QIcon::fromTheme(QLatin1String("system-search")));
    ui_->searchButton->setIconSize(QSize(16,16));
    ui_->searchButton->setFixedHeight(fontHeight > 25 ? fontHeight : 25);
    ui_->searchButton->setEnabled(false);
    ui_->searchButton->setToolTip(trUtf8("Start search"));

    ui_->label4->setText(trUtf8("Save in:"));
    ui_->label4->setBuddy(ui_->pathLine);

    ui_->pathLine->setToolTip(trUtf8("Current save folder"));
    ui_->pathLine->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    ui_->pathLine->setFixedHeight(fontHeight > 24 ? fontHeight : 24);

    // Completer used to save search history
    QCompleter *completer = new QCompleter(this);
    completer->setModel(new QDirModel(completer));
    ui_->pathLine->setCompleter(completer);

    ui_->browseButton->setToolTip(trUtf8("Select save foder"));
    ui_->browseButton->setIcon(QIcon::fromTheme(QLatin1String("folder-open")));
    ui_->browseButton->setIconSize(QSize(16,16));
    ui_->browseButton->setFixedHeight(fontHeight > 25 ? fontHeight : 25);

    // label with spinning image: requires QMovie
    ui_->busyLabel->setMaximumSize(QSize(fontHeight > 25 ? fontHeight : 25, fontHeight > 25 ? fontHeight : 25));
    ui_->busyLabel->setScaledContents(true);
    busyAnimation_ = new QMovie(QLatin1String(":/resources/busywidget.gif"));
    ui_->busyLabel->setMovie(busyAnimation_);
    ui_->busyLabel->setVisible(false);

    ui_->combosContainer->setVisible(false);

    // Connections
    connect(ui_->searchButton, SIGNAL(clicked(bool)), this, SLOT(beginSearch()));
    connect(ui_->browseButton, SIGNAL(clicked(bool)), this, SLOT(selectFolder()));
    connect(ui_->searchLine, SIGNAL(returnPressed()), this, SLOT(beginSearch()));
    connect(ui_->artistsCB, SIGNAL(activated(int)), this, SLOT(artistChanged()));
    connect(ui_->albumsCB, SIGNAL(activated(int)), this, SLOT(albumChanged()));

    ui_->matchesMessage->setFont(Utility::font(QFont::Bold));

    ui_->splitter->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);

    ui_->playerWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
}

/*!
  \brief setupActions: setup actions
  \return void
*/
void MainWindow::setupActions()
{
    actionClose_ = new QAction(QIcon::fromTheme(QLatin1String("application-exit"), QIcon(QLatin1String(":/resources/application-exit.png"))), trUtf8("&Exit"), this);
    actionClose_->setToolTip(trUtf8("Close GrooveOff"));
    actionClose_->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_Q));
    connect(actionClose_, SIGNAL(triggered()), this, SLOT(close()));

    actionDonate_ = new QAction(QIcon::fromTheme(QLatin1String("help-donate"), QIcon(QLatin1String(":/resources/help-donate.png"))), trUtf8("&Donate"), this);
    actionDonate_->setToolTip(trUtf8("Donate with PayPal"));
    connect(actionDonate_, SIGNAL(triggered()), this, SLOT(donate()));

    actionConfigure_ = new QAction(trUtf8("&Configure GrooveOff..."), this);
    if(QIcon::hasThemeIcon(QLatin1String("configure")))
        actionConfigure_->setIcon(QIcon::fromTheme(QLatin1String("configure")));
    else
        actionConfigure_->setIcon(QIcon::fromTheme(QLatin1String("gconf-editor"), QIcon(QLatin1String(":/resources/configure.png"))));

    connect(actionConfigure_, SIGNAL(triggered()), this, SLOT(configure()));

    actionCompact_ = new QAction(QIcon::fromTheme(QLatin1String("view-split-top-bottom"), QIcon(QLatin1String(":/resources/view-split-top-bottom.png"))), trUtf8("&Compact"), this);
    actionCompact_->setCheckable(true);
    connect(actionCompact_, SIGNAL(triggered()), this, SLOT(setCompactLayout()));

    actionWide_ = new QAction(QIcon::fromTheme(QLatin1String("view-split-left-right"), QIcon(QLatin1String(":/resources/view-split-left-right"))), trUtf8("&Wide"), this);
    actionWide_->setCheckable(true);
    connect(actionWide_, SIGNAL(triggered()), this, SLOT(setWideLayout()));

    actionNewToken_ = new QAction(QIcon::fromTheme(QLatin1String("emblem-new"), QIcon(QLatin1String(":/resources/emblem-new.png"))), trUtf8("&Get new token..."), this);
    connect(actionNewToken_, SIGNAL(triggered()), this, SLOT(newToken()));

    layoutGroup_ = new QActionGroup(this);
    layoutGroup_->addAction(actionCompact_);
    layoutGroup_->addAction(actionWide_);

    actionRemoveFailed_ = new QAction(QIcon::fromTheme(QLatin1String("edit-delete"),QIcon(QLatin1String(":/resources/edit-delete.png"))), trUtf8("&Remove deleted/failed songs"), this);
    connect(actionRemoveFailed_, SIGNAL(triggered(bool)), this, SLOT(removeFailedDeletedAborted()));

    actionClearDownloadList_ = new QAction(QIcon::fromTheme(QLatin1String("edit-clear"),QIcon(QLatin1String(":/resources/edit-clear"))), trUtf8("Remove &downloaded songs"), this);
    connect(actionClearDownloadList_, SIGNAL(triggered(bool)), this, SLOT(removeDownloaded()));

    actionAbout_ = new QAction(QIcon::fromTheme(QLatin1String("help-about"), QIcon(QLatin1String(":/resources/help-about.png"))), trUtf8("&About GrooveOff"), this);
    connect(actionAbout_, SIGNAL(triggered()), this, SLOT(about()));

    actionQtAbout_ = new QAction(QIcon::fromTheme(QLatin1String("qtlogo"), QIcon(QLatin1String(":/resources/qtlogo.png"))), trUtf8("About &Qt"), this);
    connect(actionQtAbout_, SIGNAL(triggered()), qApp, SLOT(aboutQt()));
}

/*!
  \brief setupMenus: setup menus
  \return void
*/
void MainWindow::setupMenus()
{
    fileMenu_ = new QMenu(trUtf8("&File"));
    fileMenu_->addAction(actionNewToken_);
    fileMenu_->addSeparator();
    fileMenu_->addAction(actionDonate_);
    fileMenu_->addSeparator();
    fileMenu_->addAction(actionClose_);
    downloadsMenu_ = new QMenu(trUtf8("&Downloads"));
    downloadsMenu_->addAction(actionRemoveFailed_);
    downloadsMenu_->addAction(actionClearDownloadList_);
    viewMenu_ = new QMenu(trUtf8("&View"));
    settingsMenu_ = new QMenu(trUtf8("&Settings"));
    settingsMenu_->addAction(actionConfigure_);
    layoutMenu_ = new QMenu(trUtf8("&Layout"));
    layoutMenu_->setIcon(QIcon::fromTheme(QLatin1String("view-multiple-objects"), QIcon(QLatin1String(":/resources/view-multiple-objects.png"))));
    viewMenu_->addMenu(layoutMenu_);
    layoutMenu_->addAction(actionCompact_);
    layoutMenu_->addAction(actionWide_);
    helpMenu_ = new QMenu(trUtf8("&Help"));
    helpMenu_->addAction(actionAbout_);
    helpMenu_->addAction(actionQtAbout_);

    menuBar()->addMenu(fileMenu_);
    menuBar()->addMenu(downloadsMenu_);
    menuBar()->addMenu(viewMenu_);
    menuBar()->addMenu(settingsMenu_);
    menuBar()->addMenu(helpMenu_);
}

/*!
  \brief selectFolder: select save folder
  \return void
*/
void MainWindow::selectFolder()
{
    dir_ = QFileDialog::getExistingDirectory(this, trUtf8("Select Directory"),
                                              ui_->pathLine->text().isEmpty() ? QDir::homePath() : ui_->pathLine->text(),
                                              QFileDialog::ShowDirsOnly
                                              | QFileDialog::DontResolveSymlinks);

    // be sure that a valid path was selected
    if(QFile::exists(dir_))
        ui_->pathLine->setText(dir_);
    else
        dir_ = ui_->pathLine->text();

    matchItemListDelegate_->setPath(ui_->pathLine->text());
}

/*!
  \brief beginSearch: start search
  \return void
*/
void MainWindow::beginSearch()
{
    // stop a search if one operation is still in progress
    if(searchInProgress_)
        return;

    // abort search if search field is empty
    if(ui_->searchLine->text().isEmpty())
        return;

    // if save searches history append last search and remove oldest if history is full
    if(showHistory_) {
        if(searchList_.count() >= searchSize_)
            searchList_.removeFirst();

        searchList_.append(ui_->searchLine->text().toLower());
        searchList_.removeDuplicates();

        setupCompleter();
    }

    // the real network routine for searching songs
    getResultsFromSearch(ui_->searchLine->text());
}

/*!
  \brief randomHex: generate a session unique random hex
  \param length : length of the random string
  \return random hex as string
*/
QString MainWindow::randomHex(const int &length)
{
    QString randomHex;

    for(int i = 0; i < length; i++) {
        int n = qrand() % 16;
        randomHex.append(QString::number(n,16));
    }

    return randomHex;
}

/*!
  \brief setTokenRequest: setup a request used for token retrieval
  \return void
*/
void MainWindow::setTokenRequest()
{
    tokenRequest_.setRawHeader(QByteArray("User-Agent"), Utility::userAgent);
    tokenRequest_.setHeader(QNetworkRequest::ContentTypeHeader, QLatin1String("application/json"));
}

/*!
  \brief setMainRequest: setup a request used for search
  \return void
*/
void MainWindow::setMainRequest()
{
    mainRequest_.setRawHeader(QByteArray("User-Agent"), Utility::userAgent);
    mainRequest_.setRawHeader(QByteArray("Referer"), QString("http://%1/JSQueue.swf?%2").arg(Utility::host).arg(Utility::jsqueue().at(1)).toAscii());
    mainRequest_.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
}

/*!
  \brief getToken: get the token
  \return void
*/
void MainWindow::getToken()
{
    ui_->searchLine->setStyleSheet("background-color:#E7A2A9;");

    // clear cookies
    jar_->clear();

    // some ui setups
    ui_->qled->setValue(false);
    statusBar()->showMessage(trUtf8("Connecting..."), 0);

    // set current operation
    currentJob_ = GrooveOff::TokenJob;

    QJson::Serializer serializer;
    QByteArray json = serializer.serialize(Utility::getTokenMap());

    tokenRequest_.setUrl(QUrl(QLatin1String("https://") + Utility::host + QLatin1String("/more.php")));

    qnam_->post(tokenRequest_,json);
}

/*!
  \brief getResultsFromSearch : return match results
  \param query : text to search
  \param what : type of search
  \return void
*/
void MainWindow::getResultsFromSearch(const QString &query, const QString &what)
{
    qDebug() << "GrooveOff ::" << "Searching for" << query;

    // flag that prevents multiple searches
    searchInProgress_ = true;

    // some ui setups
    ui_->busyLabel->setVisible(true);
    ui_->searchButton->setVisible(false);
    busyAnimation_->start();

    // clear songs list
    songs_.clear();

    // clear list of previous results
    results_.clear();

    // clear the model
    listModel_->clear();
    proxyModel_->setSourceModel(listModel_);

    // clear artists, albums containers
    ui_->artistsCB->clear();
    ui_->artistsCB->addItem(trUtf8("All Artists"));
    ui_->albumsCB->clear();
    ui_->albumsCB->addItem(trUtf8("All Albums"));
    artistsAlbumsContainer_.clear();

    QVariantMap map = Utility::searchMap(query, what, token_);
    QJson::Serializer serializer;
    QByteArray json = serializer.serialize(map);

    tokenRequest_.setUrl(QUrl(QLatin1String("http://") + Utility::host + QLatin1String("/more.php?") + map.value(QLatin1String("method")).toByteArray()));
    currentJob_ = GrooveOff::SearchJob;
    qnam_->post(tokenRequest_,json);
}

/*!
  \brief populateResultsTable : fills results list
  \return void
*/
void MainWindow::populateResultsTable()
{
    // check if last search returned results
    if(results_.count() == 0) {
        qDebug() << "GrooveOff :: " << "Empty result list";
        return;
    }

    // check if it's a playlist
    bool isPlaylist;
    ui_->searchLine->text().toInt(&isPlaylist, 10);

    // row index (start from 0)
    row_ = 0;

    // 'count' contains number of elements to display
    int count;

    if(maxResults_ == 0 || isPlaylist) // no results limit and do not limit playlists
        count = results_.count();
    else // limit results for performance
        count = qMin(maxResults_, results_.count());

    ui_->combosContainer->setVisible(true);

    QStringList artists;
    QStringList albums;

    qDebug() << "GrooveOff :: " << "Parsing " << count << " items";
    // start building a qlist of Song objects
    for(int i = 0; i < count; i++) {
        QVariantMap m = results_.at(i).toMap();

        // quite obvious...
        Song *song = new Song(this);
        song->setTitle(m[QLatin1String(isPlaylist ? "Name" : "SongName")].toString());
        song->setAlbum(m[QLatin1String("AlbumName")].toString());
        song->setArtist(m[QLatin1String("ArtistName")].toString());
        song->setYear(m[QLatin1String("Year")].toString());
        song->setId(m[QLatin1String("SongID")].toString());

        // Decide if show cover arts
        if(loadCovers_ && !m[QLatin1String("CoverArtFilename")].toString().isEmpty())
            song->setCoverName(m[QLatin1String("CoverArtFilename")].toString());
        else
            song->setCoverName(QLatin1String("NoCoverArt"));

        connect(song, SIGNAL(trigRepaint()), this, SLOT(reloadItemsCover()));

        songs_.append(song);

        bool found = false;
        int j = 0;
        for(; j < artistsAlbumsContainer_.count(); j++) {
            if(artistsAlbumsContainer_.at(j).first == m[QLatin1String("ArtistName")].toString()) {
                found = true;
                break;
            }
        }

        if(found) {
            bool albumExists = false;
            for(int k = 0; k < artistsAlbumsContainer_[j].second.count(); k++) {
                if(artistsAlbumsContainer_[j].second.at(k) == m[QLatin1String("AlbumName")].toString())
                    albumExists = true;
            }
            if(!albumExists) {
                artistsAlbumsContainer_[j].second << m[QLatin1String("AlbumName")].toString();
                artistsAlbumsContainer_[j].second.sort();
            }
        } else {
            QPair<QString, QStringList> e;
            e.first = m[QLatin1String("ArtistName")].toString();
            e.second << m[QLatin1String("AlbumName")].toString();
            artistsAlbumsContainer_.append(e);
            artists << m[QLatin1String("ArtistName")].toString();
        }

        albums << m[QLatin1String("AlbumName")].toString();
    }

    // use 'songs' in our model
    listModel_->setElements(songs_);
    proxyModel_->setSourceModel(listModel_);

    // removing duplicates
    artists.removeDuplicates();
    albums.removeDuplicates();

    // sorting items
    artists.sort();
    albums.sort();

    ui_->artistsCB->addItems(artists);
    ui_->albumsCB->addItems(albums);
    applyFilter();
}

/*!
  \brief newToken : get a new token (if previous is outdated or not working)
  \return void
*/
void MainWindow::newToken()
{
    getToken();
}

/*!
  \brief replyFinished : evaluate network reply once finished
  \param reply : network reply
  \return void
*/
void MainWindow::replyFinished(QNetworkReply *reply)
{
    // check if reply was wrong
    if(!reply->error() == QNetworkReply::NoError) {
        qDebug() << "GrooveOff ::" << reply->errorString();

        // the error occurred during token request...
        if(currentJob_ == GrooveOff::TokenJob) {
            statusBar()->showMessage(reply->errorString(), 3000);
            ui_->qled->setToolTip(trUtf8("Check your connection and try again"));
            ui_->searchButton->setEnabled(false);
        } else if(currentJob_ == GrooveOff::SearchJob) { // the error occurred during search request...
            populateResultsTable();
        }
        return;
    }

    // the application is now free to perform a search
    searchInProgress_ = false;
    ui_->searchButton->setEnabled(true);

    QJson::Parser parser;
    bool ok;

    // get the reply data
    QByteArray data = reply->readAll();
    // json is a QString containing the data to convert
    QVariantMap result = parser.parse (data, &ok).toMap();

    if (!ok) {
        qDebug() << "GrooveOff ::" << "An error occurred during parsing";
        ui_->busyLabel->setVisible(false);
        ui_->searchButton->setVisible(true);
        busyAnimation_->stop();
        return;
    }

    switch(currentJob_) {
    case GrooveOff::TokenJob:
        token_ = result[QLatin1String("result")].toString();
        if(!token_.isEmpty()) {
            ui_->searchLine->setStyleSheet("");

            // give token to existing download items
            for(int i = 0; i < ui_->downloadList->count(); i++) {
                ((DownloadItem *)ui_->downloadList->itemWidget(ui_->downloadList->item(i)))->setToken(token_);
            }

            statusBar()->showMessage(trUtf8("Connected"), 3000);
            ui_->qled->setValue(true);
            ui_->qled->setToolTip(trUtf8("You're connected to grooveshark!"));
            ui_->listView->setEnabled(true);
        } else {
            statusBar()->showMessage(trUtf8("Token not received!!"), 3000);
            qDebug() << "GrooveOff ::" << "Token not received!!";
            qDebug() << "GrooveOff ::" << result;
        }

        break;
    case GrooveOff::SearchJob:
        bool isPlaylist;
        ui_->searchLine->text().toInt(&isPlaylist, 10);

        if(isPlaylist)
            results_ = result[QLatin1String("result")].toMap()[QLatin1String("Songs")].toList();
        else
            results_ = result[QLatin1String("result")].toMap()[QLatin1String("result")].toList();

        populateResultsTable();

        ui_->busyLabel->setVisible(false);
        ui_->searchButton->setVisible(true);
        busyAnimation_->stop();

        break;
    }
}

/*!
  \brief addDownloadItem : add a new download to the list
  \param index : index of item in results list (to download)
  \return void
*/
void MainWindow::addDownloadItem(const QModelIndex &index)
{
    // check if destination folder exists
    if(!QFile::exists(ui_->pathLine->text())) {
        QMessageBox::information(this, trUtf8("Attention"),
                                       trUtf8("The destination folder does not exists.\n"
                                              "Select a valid path"),
                                 QMessageBox::Ok);
        return;
    }

    // check if destination folder is writable
    QFileInfo fi(ui_->pathLine->text());
    if(!fi.isWritable()) {
        QMessageBox::information(this, trUtf8("Attention"),
                                       trUtf8("The destination folder is not writable.\n"
                                              "Select a valid path"),
                                 QMessageBox::Ok);
        return;
    }

    if(isDownloadingQueued(index.data(SongRoles::Id).toString()))
        return;

    QString fileName = index.data(SongRoles::Artist).toString() + " - " + index.data(Qt::DisplayRole).toString();

    // check file existence
    if(QFile::exists(ui_->pathLine->text() + QDir::separator() + fileName + ".mp3")) {
        int ret = QMessageBox::question(this,
                                        trUtf8("Overwrite File?"),
                                        trUtf8("A file named \"%1\" already exists. Are you sure you want to overwrite it?").arg(fileName),
                                        QMessageBox::Yes | QMessageBox::Cancel,
                                        QMessageBox::Cancel);
        if(ret == QMessageBox::Yes) {
            QFile::remove(ui_->pathLine->text() + QDir::separator() + fileName + ".mp3");
        } else {
            return;
        }
    }

    // build a DownloadItem with all required data
    DownloadItem *item = new DownloadItem(ui_->pathLine->text(), // save folder
                                          fileName,
                                          index.data(SongRoles::Id).toString(), // song id
                                          token_,
                                          index.data(SongRoles::Cover).toString(),
                                          this);

    // check if download queue_ is full
    if(parallelDownloadsCount_ < maxDownloads_) {
        parallelDownloadsCount_++;
        item->startDownload();
    } else {
        queue_.append(item);
    }

    connect(item, SIGNAL(play(DownloadItem*)), ui_->playerWidget, SLOT(playItem(DownloadItem*)));
    connect(item, SIGNAL(remove(DownloadItem*)), ui_->playerWidget, SLOT(removeItem(DownloadItem*)));
    connect(item, SIGNAL(downloadFinished()), this, SLOT(freeDownloadSlot()));
    connect(item, SIGNAL(pauseResumePlaying()), ui_->playerWidget, SLOT(pauseResumePlaying()));
    connect(item, SIGNAL(addToQueue(DownloadItem*)), this, SLOT(addItemToQueue(DownloadItem*)));
    connect(item, SIGNAL(stateChangedSignal()), listModel_, SLOT(forceRepaint()));

    QListWidgetItem *i = new QListWidgetItem;
    ui_->downloadList->addItem(i);
    ui_->downloadList->setItemWidget(i, item);
    ui_->downloadList->setCurrentItem(i);
    i->setSizeHint(QSize(50,50));
}

/*!
  \brief setCompactLayout : set vertical orientation for splitter
  \return void
*/
void MainWindow::setCompactLayout()
{
    ui_->splitter->setOrientation(Qt::Vertical);

    // save choice
    QSettings settings;
    settings.setIniCodec( "UTF-8" );
    settings.setValue(QLatin1String("splitterOrientation"), ui_->splitter->orientation());
}

/*!
  \brief setWideLayout : set horizontal orientation for splitter
  \return void
*/
void MainWindow::setWideLayout()
{
    ui_->splitter->setOrientation(Qt::Horizontal);

    // save choice
    QSettings settings;
    settings.setIniCodec( "UTF-8" );
    settings.setValue(QLatin1String("splitterOrientation"), ui_->splitter->orientation());
}

/*!
  \brief about : open application's about dialog
  \return void
*/
void MainWindow::about()
{
    AboutDialog dialog;
    dialog.exec();
}

/*!
  \brief configure : open application's configure dialog
  \return void
*/
void MainWindow::configure()
{
    ConfigDialog config;
    config.exec();

    loadSettings();
}

/*!
  \brief onlineStateChanged : slot called when online state changes
  \param isOnline : true if online
  \return void
*/
void MainWindow::onlineStateChanged(bool isOnline) {
    if(isOnline) { // when returning online get a new token
        newToken();
    } else {
        statusBar()->showMessage(trUtf8("Offline"),0);
        ui_->searchButton->setEnabled(false);
        ui_->qled->setValue(false);
        ui_->listView->setEnabled(false);
    }
}

/*!
  \brief setupCompleter : setup a completer for search line edit
  \return void
*/
void MainWindow::setupCompleter()
{
    QCompleter *completer = new QCompleter(searchList_, this);
    completer->setCaseSensitivity(Qt::CaseInsensitive);
    ui_->searchLine->setCompleter(completer);
}

/*!
  \brief loadSettings : load settings
  \return void
*/
void MainWindow::loadSettings()
{
    QSettings settings;
    settings.setIniCodec( "UTF-8" );

    showHistory_     = settings.value(QLatin1String("saveSearches"), false).toBool();
    maxResults_      = settings.value(QLatin1String("numResults"), 0).toInt();
    loadCovers_      = settings.value(QLatin1String("loadCovers"), true).toBool();
    maxDownloads_    = settings.value(QLatin1String("maxDownloads"), 5).toInt();
    saveDestination_ = settings.value(QLatin1String("saveDestination"), false).toBool();

    if(showHistory_) {
        searchSize_ = settings.value(QLatin1String("historySize"), 5).toInt();
        searchList_ << settings.value(QLatin1String("searchTerms"), QStringList()).toStringList();
        setupCompleter();
    }

    ui_->splitter->setOrientation((Qt::Orientation)settings.value(QLatin1String("splitterOrientation"), Qt::Vertical).toInt());

    QList<int> _sizes = settings.value(QLatin1String("splitterSizes")).value< QList<int> >();

    if(!_sizes.isEmpty()) {
        ui_->splitter->setSizes(_sizes);
    }
}

/*!
  \brief saveSettings : save settings
  \return void
*/
void MainWindow::saveSettings()
{
    QSettings settings;
    settings.setIniCodec( "UTF-8" );

    settings.setValue(QLatin1String("splitterSizes"), QVariant::fromValue< QList<int> >(ui_->splitter->sizes()));
    settings.setValue(QLatin1String("windowGeometry"), geometry());
    if(!showHistory_)
        settings.setValue(QLatin1String("searchTerms"), QStringList());
    else
        settings.setValue(QLatin1String("searchTerms"), searchList_);

    if(saveDestination_)
        settings.setValue(QLatin1String("destination"), ui_->pathLine->text());
    else
        settings.setValue(QLatin1String("destination"), QString());
}

/*!
  \brief donate : open web page for donations
  \return void
*/
void MainWindow::donate()
{
    QDesktopServices::openUrl(QUrl(QLatin1String("https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=WJNETV7GLTKDG&item_name=") + trUtf8("Donation to Grooveoff's author")));
}

/*!
  \brief applyFilter : apply filter to proxy model
  \return void
*/
void MainWindow::applyFilter()
{
    QString artist;
    QString album;

    if(ui_->artistsCB->currentIndex() == 0)
        artist = "All Artists";
    else
        artist = ui_->artistsCB->currentText();

    if(ui_->albumsCB->currentIndex() == 0)
        album = "All Albums";
    else
        album = ui_->albumsCB->currentText();

    proxyModel_->setMatchTerms(artist, album);
    proxyModel_->setFilterRegExp(QRegExp());
    //ui_->matchesMessage->setText(trUtf8("%1 songs found").arg(proxyModel_->rowCount()));
    ui_->matchesMessage->setText(trUtf8("%n song(s) found", "", proxyModel_->rowCount()));
}

/*!
  \brief artistChanged : slot for artistsCB
  \return void
*/
void MainWindow::artistChanged()
{
    ui_->albumsCB->clear();
    ui_->albumsCB->addItem(trUtf8("All Albums"));
    for(int i = 0; i < artistsAlbumsContainer_.count(); i++) {
        if(artistsAlbumsContainer_.at(i).first == ui_->artistsCB->currentText() ||
            ui_->artistsCB->currentIndex() == 0) {
            ui_->albumsCB->addItems(artistsAlbumsContainer_.at(i).second);
        }
    }

    applyFilter();
}

/*!
  \brief artistChanged : slot for albumsCB
  \return void
*/
void MainWindow::albumChanged()
{
    applyFilter();
}

/*!
  \brief unqueue : queue management
  \return void
*/
void MainWindow::unqueue()
{
    // this loop iterates for each free download slot
    for(; parallelDownloadsCount_ < maxDownloads_; parallelDownloadsCount_++) {
        // check if queue_ is empty or item already queued
        while(!queue_.isEmpty() && ((DownloadItem *)queue_.at(0))->downloadState() != GrooveOff::QueuedState) {
            // discard item
            queue_.takeFirst();
        }

        if(queue_.isEmpty())
            return;

        // valid item found
        ((DownloadItem *)queue_.at(0))->startDownload();
        queue_.takeFirst();
    }
}

/*!
  \brief removeFailedDeletedAborted : remove failed/deleted/aborted items from download list
  \return void
*/
void MainWindow::removeFailedDeletedAborted()
{
    for(int i = ui_->downloadList->count() - 1; i >= 0; i--) {
        GrooveOff::DownloadState state = ((DownloadItem *)ui_->downloadList->itemWidget(ui_->downloadList->item(i)))->downloadState();
        if(state == GrooveOff::AbortedState ||
           state == GrooveOff::DeletedState ||
           state == GrooveOff::ErrorState ) {
            QListWidgetItem *item = ui_->downloadList->takeItem(i);
            ui_->downloadList->removeItemWidget(item);
            delete item;
        }
    }
}

/*!
  \brief removeDownloaded : remove successfully downloaded items from download list
  \return void
*/
void MainWindow::removeDownloaded()
{
    for(int i = ui_->downloadList->count() - 1; i >= 0; i--) {
        GrooveOff::DownloadState state = ((DownloadItem *)ui_->downloadList->itemWidget(ui_->downloadList->item(i)))->downloadState();
        if(state == GrooveOff::FinishedState) {
            QListWidgetItem *item = ui_->downloadList->takeItem(i);
            ui_->downloadList->removeItemWidget(item);
            delete item;
        }
    }
}

/*!
  \brief addItemToQueue : add an item to the queue_ list
  \param item: item to add
  \return void
*/
void MainWindow::addItemToQueue(DownloadItem* item)
{
    queue_.append(item);
    unqueue();
}

/*!
  \brief freeDownloadSlot : slot called when a songs download finish
  \return void
*/
void MainWindow::freeDownloadSlot()
{
    parallelDownloadsCount_--;
    unqueue();
}

/*!
  \brief isDownloadingQueued : check if current song id is downloading/queued
  \param id: song id
  \return bool
*/
bool MainWindow::isDownloadingQueued(const QString &id)
{
    // check if file is currently downloading
    for(int i = 0; i < ui_->downloadList->count(); i++) {
        GrooveOff::DownloadState state = qobject_cast<DownloadItem *>(ui_->downloadList->itemWidget(ui_->downloadList->item(i)))->downloadState();
        if(state != GrooveOff::DeletedState) {
            if(id == qobject_cast<DownloadItem *>(ui_->downloadList->itemWidget(ui_->downloadList->item(i)))->id()) {
                QMessageBox::information(this,
                                        trUtf8("Download in progress"),
                                        trUtf8("A file with the same name is already in your download list."),
                                        QMessageBox::Ok);
                return true;
            }
        }
    }
    return false;
}

/*!
  \brief reloadItemsCover : when a new cover was downloaded on cache
  refresh both listviews
  \return void
*/
void MainWindow::reloadItemsCover()
{
    //FIXME : this is quite brutal but it avoids a lot of code (and bugs) and seems working without issues :P
    listModel_->forceRepaint();

    for(int i = 0; i < ui_->downloadList->count(); i++) {
        qobject_cast<DownloadItem *>(ui_->downloadList->itemWidget(ui_->downloadList->item(i)))->pickCover();
    }
}


#include "mainwindow.moc"
