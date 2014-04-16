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


#include "MainWindow.h"
#include "ui_MainWindow.h"
#include "PlayerWidget.h"
#include "widgets/QLed.h"
#include "DownloadItem.h"
#include "Roles.h"
#include "ConfigDialog.h"
#include "AboutDialog.h"
#include "FilterProxyModel.h"
#include "Utility.h"
#include "GrooveOffNamespace.h"
#include "CoverManager.h"
#include "MatchItem.h"
#include "PlaylistItem.h"
#include "AudioEngine.h"
#include "ActionCollection.h"
#include "PaletteHandler.h"
#include "SvgHandler.h"
#include "Playlist.h"
#include "AudioEngine.h"
#include "widgets/Spinner.h"
#include "SessionManager.h"
#include "SessionReaderWriter.h"

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
#include <QInputDialog>

#if QT_VERSION >= QT_VERSION_CHECK( 5, 0, 0 )
#include <QStandardPaths>
#endif

#include <QTime>
#include <QFileDialog>
#include <QDir>
#include <QFile>
#include <qjson/serializer.h>
#include <qjson/parser.h>
#include <QMessageBox>
#include <QApplication>
#include <QCompleter>
#include <QDirModel>
#include <QPointer>

using namespace GrooveShark;

// version include
#include <../config-version.h>

Q_DECLARE_METATYPE(QList<int>)

QPointer<MainWindow> MainWindow::s_instance;

namespace The {
    MainWindow* mainWindow() { return MainWindow::s_instance.data(); }
}

/*!
  \brief MainWindow: this is the MainWindow constructor.
  \param parent: The Parent Widget
*/
MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui_(new Ui::MainWindow)
{
    qRegisterMetaType<PlaylistItemPtr>("PlaylistItemPtr");
    qRegisterMetaTypeStreamOperators<PlaylistItemPtr>("PlaylistItemPtr");

    ui_->setupUi(this);
    s_instance = this;
    The::paletteHandler()->setPalette( palette() );
    playerWidget = new PlayerWidget(this);
    new ActionCollection(this);
    setupUi();
    setupMenus();
    setupSignals();
    loadSettings();
    statusBar()->addPermanentWidget( playerWidget, 1 );
    statusBar()->setSizeGripEnabled(false);

    spinner_ = new Spinner(ui_->spinnerWidget);
    spinner_->setText(tr(""));
    spinner_->setType(Spinner::Sun);

#if QT_VERSION >= QT_VERSION_CHECK( 5, 0, 0 )
    if(!QFile::exists(QStandardPaths::writableLocation(QStandardPaths::CacheLocation))) {
        QDir dir;
        dir.mkpath(QStandardPaths::writableLocation(QStandardPaths::CacheLocation));
    }

    Utility::coversCachePath = QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + QDir::separator();
#else
    if(!QFile::exists(QDesktopServices::storageLocation(QDesktopServices::CacheLocation))) {
        QDir dir;
        dir.mkpath(QDesktopServices::storageLocation(QDesktopServices::CacheLocation));
    }

    Utility::coversCachePath = QDesktopServices::storageLocation(QDesktopServices::CacheLocation) + QDir::separator();
#endif

    nam_ = new QNetworkAccessManager(this);
    api_ = ApiRequest::instance();
    jar_ = new MyJar;
    nam_->setCookieJar(jar_);

    cvrMngr_ = new CoverManager(this);

    //fake
    searchInProgress_ = true;

    setWindowTitle(QString::fromLatin1( "GrooveOff %1" ).arg( GROOVEOFF_VERSION ));

    if(guiLayout_ == Mini) {
        ActionCollection::instance()->getAction( QLatin1String( "miniPlayer" ) )->setChecked(true);
        playerWidget->showElapsedTimerLabel(true);
    } else if(ui_->splitter->orientation() == Qt::Vertical) {
        ActionCollection::instance()->getAction( QLatin1String( "actionCompact" ) )->setChecked(true);
        playerWidget->showElapsedTimerLabel(false);
    }
    else {
        ActionCollection::instance()->getAction( QLatin1String( "actionWide" ) )->setChecked(true);
        playerWidget->showElapsedTimerLabel(true);
    }

    QTime time = QTime::currentTime();
    qsrand((uint)time.msec());

    qncm_ = new QNetworkConfigurationManager(this);
    connect(qncm_, SIGNAL(onlineStateChanged(bool)),
                   SLOT(onlineStateChanged(bool)));
    if(qncm_->isOnline())
        getToken();

    parallelDownloadsCount_ = 0;

    QSettings settings;
    settings.setIniCodec( "UTF-8" );

#if QT_VERSION >= QT_VERSION_CHECK( 5, 0, 0 )
    if(saveDestination_) {
        ui_->pathLine->setText(settings.value(QLatin1String("destination"),
                               QStandardPaths::writableLocation(QStandardPaths::MusicLocation)).toString());
        if(ui_->pathLine->text().isEmpty())
            ui_->pathLine->setText(QStandardPaths::writableLocation(QStandardPaths::MusicLocation));
    } else {
        ui_->pathLine->setText(QStandardPaths::writableLocation(QStandardPaths::MusicLocation));
    }
#else
    if(saveDestination_) {
        ui_->pathLine->setText(settings.value(QLatin1String("destination"),
                               QDesktopServices::storageLocation(QDesktopServices::MusicLocation)).toString());
        if(ui_->pathLine->text().isEmpty())
            ui_->pathLine->setText(QDesktopServices::storageLocation(QDesktopServices::MusicLocation));
    } else {
        ui_->pathLine->setText(QDesktopServices::storageLocation(QDesktopServices::MusicLocation));
    }
#endif

    setGeometry(settings.value(QLatin1String("windowGeometry"), QRect(100,100,350,600)).toRect());

#if QT_VERSION >= QT_VERSION_CHECK( 5, 0, 0 )
    sessionFilePath_ = QStandardPaths::writableLocation(QStandardPaths::DataLocation)
                   + QDir::separator();
#else
    sessionFilePath_ = QDesktopServices::storageLocation(QDesktopServices::DataLocation).replace( QLatin1String( "/data" ),"")
                   + QDir::separator();
#endif
    if(saveSession_)
        loadSession();

    // FIXME
    // loadSessions();

    mpris_ = new Mpris(this);
}

/*!
  \brief ~MainWindow: this is the MainWindow destructor.
*/
MainWindow::~MainWindow()
{
    delete The::paletteHandler();
    delete The::svgHandler();

    spinner_->stop();

    if(saveSession_)
        saveSession();

    saveSettings();

    delete api_;
    delete mpris_;
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

//    statusBar()->showMessage(trUtf8("Connecting...", 0));
    playerWidget->showMessage(trUtf8("Connecting..."));

    // Led
    ui_->qled->setFixedSize(QSize(24,24));

    QFontMetrics fmSystemFont(Utility::font(QFont::Bold));
    int fontHeight = fmSystemFont.height();

    ui_->label3->setText(trUtf8("Search:"));
    ui_->label3->setBuddy(ui_->searchLine);
    ui_->searchLine->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    ui_->searchLine->setFixedHeight(fontHeight > 24 ? fontHeight : 24);
    ui_->searchLine->setToolTip(trUtf8("Search for songs, artists, genres, playlists"));
    ui_->searchLine->setPlaceholderText(trUtf8("Search for songs, artists, genres, playlists"));

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

    ui_->spinnerWidget->setMinimumSize(QSize(fontHeight > 30 ? fontHeight : 30,
                                         fontHeight > 30 ? fontHeight : 30));
    ui_->spinnerWidget->setVisible(false);

    ui_->combosContainer->setVisible(false);

    ui_->batchDownloadButton->setToolTip(trUtf8("Download all files"));
    ui_->batchDownloadButton->setIcon(QIcon::fromTheme(QLatin1String("kget")));
    ui_->batchDownloadButton->setIconSize(QSize(16,16));
    ui_->batchDownloadButton->setFixedHeight(fontHeight > 25 ? fontHeight : 25);

    ui_->matchesMessage->setFont(Utility::font(QFont::Bold));

    ui_->splitter->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);

    playerWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
}

/*!
  \brief setupMenus: setup menus
  \return void
*/
void MainWindow::setupMenus()
{
    // Always create a menubar, but only create a compactMenu on Windows and X11
    m_menuBar = ActionCollection::instance()->createMenuBar( this );
    setMenuBar( m_menuBar );
    m_compactMainMenu = ActionCollection::instance()->createCompactMenu( this );
}

void MainWindow::setupSignals()
{
    // <Menu Items>
    ActionCollection *ac = ActionCollection::instance();

    connect(ac->getAction( "actionClose" ), SIGNAL(triggered()),
                                            SLOT(close()));

    connect(ac->getAction( "actionDonate" ), SIGNAL(triggered()),
                                             SLOT(donate()));

    connect(ac->getAction( "actionConfigure" ), SIGNAL(triggered()),
                                                SLOT(configure()));

    connect(ac->getAction( "actionCompact" ), SIGNAL(triggered()),
                                              SLOT(setCompactLayout()));

    connect(ac->getAction( "actionWide" ), SIGNAL(triggered()),
                                           SLOT(setWideLayout()));

    connect(ac->getAction( "miniPlayer" ), SIGNAL(triggered()),
                                           SLOT(setMiniPlayerLayout()));

    connect(ac->getAction( "actionNewToken" ), SIGNAL(triggered()),
                                               SLOT(getToken()));

    connect(ac->getAction( "actionStopDownloads" ), SIGNAL(triggered(bool)),
            ui_->downloadList,                      SLOT(abortAllDownloads()));

    connect(ac->getAction( "actionRemoveFailed" ), SIGNAL(triggered(bool)),
            ui_->downloadList,                     SLOT(removeFailedDeletedAborted()));

    connect(ac->getAction( "actionClearDownloadList" ), SIGNAL(triggered(bool)),
            ui_->downloadList,                          SLOT(removeDownloaded()));

    connect(ac->getAction( "actionAbout" ), SIGNAL(triggered()),
                                            SLOT(about()));

    connect(ac->getAction( "actionSaveSessionAs" ), SIGNAL(triggered()),
                                                    SLOT(saveSessionAs()));

    connect(ac->getAction( "actionManageSessions" ), SIGNAL(triggered()),
                                                     SLOT(openSessionManager()));

    connect(ac->getAction( "actionQtAbout" ), SIGNAL(triggered()),
            qApp,                             SLOT(aboutQt()));


    // gui widgets
    connect(ui_->searchButton, SIGNAL(clicked(bool)),
                               SLOT(beginSearch()));
    connect(ui_->browseButton, SIGNAL(clicked(bool)),
                               SLOT(selectFolder()));
    connect(ui_->searchLine, SIGNAL(returnPressed()),
                             SLOT(beginSearch()));
    connect(ui_->artistsCB, SIGNAL(activated(int)),
                            SLOT(artistChanged()));
    connect(ui_->albumsCB, SIGNAL(activated(int)),
                           SLOT(albumChanged()));
    connect(ui_->batchDownloadButton, SIGNAL(clicked()),
                                      SLOT(batchDownload()));
    connect(ui_->pathLine, SIGNAL(textChanged(QString)),
                           SLOT(changeDestinationPath()));
}


/*!
  \brief selectFolder: select save folder
  \return void
*/
void MainWindow::selectFolder()
{
    QString dir = QFileDialog::getExistingDirectory(this, trUtf8("Select Directory"),
                                              ui_->pathLine->text().isEmpty() ? QDir::homePath() : ui_->pathLine->text(),
                                              QFileDialog::ShowDirsOnly
                                              | QFileDialog::DontResolveSymlinks);

    // be sure that a valid path was selected
    if(QFile::exists(dir))
        ui_->pathLine->setText(dir);

    changeDestinationPath();
}

void MainWindow::changeDestinationPath()
{
    Utility::destinationPath = ui_->pathLine->text();
    reloadItemsDownloadButtons();
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

//    qDebug() << "GrooveOff ::" << "Searching for" << ui_->searchLine->text();

    // flag that prevents multiple searches
    searchInProgress_ = true;

    // some ui setups
    ui_->searchButton->setVisible(false);

    ui_->spinnerWidget->setVisible(true);
    spinner_->start(0);

    // clear cover manager
    cvrMngr_->clear();

    // clear listWidget
    for(int i = ui_->matchList->count() - 1; i >= 0; i--) {
        QListWidgetItem *item = ui_->matchList->takeItem(i);
        ui_->downloadList->removeItemWidget(item);
        delete item;
    }

    // clear artists, albums containers
    ui_->artistsCB->clear();
    ui_->artistsCB->addItem(trUtf8("All Artists"));
    ui_->albumsCB->clear();
    ui_->albumsCB->addItem(trUtf8("All Albums"));
    artistsAlbumsContainer_.clear();

    songList_ = api_->songs(ui_->searchLine->text(), Utility::token);

    connect(songList_.data(), SIGNAL(finished()),
                              SLOT(searchFinished()));

    connect(songList_.data(), SIGNAL(parseError()),
                              SLOT(searchError()));

    connect(songList_.data(), SIGNAL(requestError(QNetworkReply::NetworkError)),
                              SLOT(searchError()));

    // force downloadList repaint to prevent blank items
    ui_->downloadList->repaint();
}

/*!
  \brief getToken: get the token
  \return void
*/
void MainWindow::getToken()
{
    ui_->searchLine->setStyleSheet( QLatin1String( "background-color:#E7A2A9;" ) );

    // clear cookies
    jar_->clear();

    // some ui setups
    ui_->qled->setValue(false);
    //statusBar()->showMessage(trUtf8("Connecting..."), 0);
    playerWidget->showMessage(trUtf8("Connecting..."));

    token_ = api_->token();

    connect( token_.data(), SIGNAL(finished()),
             this, SLOT(tokenFinished()));

    connect(token_.data(), SIGNAL(parseError()),
            this, SLOT(tokenError()));

    connect(token_.data(), SIGNAL(requestError(QNetworkReply::NetworkError)),
            this, SLOT(tokenError()));
}

void MainWindow::tokenFinished()
{
    // the application is now free to perform a search
    searchInProgress_ = false;
    ui_->searchButton->setEnabled(true);

    if(!token_->result().isEmpty()) {
        ui_->searchLine->setStyleSheet( QLatin1String( "" ) );

        Utility::token = token_->result();

        //statusBar()->showMessage(trUtf8("Connected"), 3000);
        playerWidget->showMessage(trUtf8("Connected"));
        ui_->qled->setValue(true);
        ui_->qled->setToolTip(trUtf8("You're connected to grooveshark!"));
        ui_->matchList->setEnabled(true);

        // start download of queued songs from last session
        unqueue();
    } else {
        //statusBar()->showMessage(trUtf8("Token not received!!"), 3000);
        playerWidget->showMessage(trUtf8("Connection error!!"));
        qDebug() << "GrooveOff ::" << "Token not received!!";
        Utility::token.clear();
    }
}

void MainWindow::tokenError()
{
    playerWidget->showMessage(trUtf8("Connection error!!"));
    qDebug() << "GrooveOff ::" << token_->errorString();
}

/*!
  \brief searchFinished : fills results list
  \return void
*/
void MainWindow::searchFinished()
{
    restoreSearch();

    // check if last search returned results
    if(songList_->list().count() == 0) {
        qDebug() << "GrooveOff ::" << "Empty result list";
        return;
    }

    // row index (start from 0)
    row_ = 0;

    // 'count' contains number of elements to display
    int count;

    if(maxResults_ == 0) // no results limit
        count = songList_->list().count();
    else // limit results for performance
        count = qMin(maxResults_, songList_->list().count());

    ui_->combosContainer->setVisible(true);

    QStringList artists;
    QStringList albums;

    ui_->matchesMessage->setText(trUtf8("%n song(s) found", "", count));

    for(int i = 0; i < count; i++) {
        PlaylistItemPtr playlistItem(new PlaylistItem(songList_->list().at(i)));

        // Decide if show cover arts
        if(loadCovers_ && !QFile::exists(Utility::coversCachePath + songList_->list().at(i)->coverArtFilename())) {
            cvrMngr_->addItem(playlistItem);
        }

        // build a MathItem with all required data
        MatchItem *matchItem = new MatchItem(playlistItem, this);
        QListWidgetItem *wItem = new QListWidgetItem;
        ui_->matchList->addItem(wItem);
        ui_->matchList->setItemWidget(wItem, matchItem);
        wItem->setSizeHint(QSize(Utility::coverSize + Utility::marginSize * 2,Utility::coverSize + Utility::marginSize * 2));
        connect(matchItem, SIGNAL(download(PlaylistItemPtr)), this, SLOT(downloadRequest(PlaylistItemPtr)));

        // don't freeze gui inserting items
        QCoreApplication::processEvents();

        // populate filter widgets
        bool found = false;
        int j = 0;
        for(; j < artistsAlbumsContainer_.count(); j++) {
            if(artistsAlbumsContainer_.at(j).first == songList_->list().at(i)->artistName()) {
                found = true;
                break;
            }
        }

        if(found) {
            bool albumExists = false;
            for(int k = 0; k < artistsAlbumsContainer_[j].second.count(); k++) {
                if(artistsAlbumsContainer_[j].second.at(k) == songList_->list().at(i)->albumName())
                    albumExists = true;
            }
            if(!albumExists) {
                artistsAlbumsContainer_[j].second << songList_->list().at(i)->albumName();
                artistsAlbumsContainer_[j].second.sort();
            }
        } else {
            QPair<QString, QStringList> e;
            e.first = songList_->list().at(i)->artistName();
            e.second << songList_->list().at(i)->albumName();
            artistsAlbumsContainer_.append(e);

            if(!songList_->list().at(i)->artistName().isEmpty())
                artists << songList_->list().at(i)->artistName();
        }

        if(!songList_->list().at(i)->albumName().isEmpty())
            albums << songList_->list().at(i)->albumName();
    }

    // removing duplicates
    artists.removeDuplicates();
    albums.removeDuplicates();

    // sorting items
    artists.sort();
    albums.sort();

    ui_->artistsCB->addItems(artists);
    ui_->albumsCB->addItems(albums);
    applyFilter();

    // force downloadList repaint to prevent blank items
    ui_->downloadList->repaint();
}

void MainWindow::searchError()
{
    qDebug() << "GrooveOff ::" << songList_->errorString();
    restoreSearch();
}

void MainWindow::downloadRequest(PlaylistItemPtr playlistItem)
{
    // at this point playlistItem doesn't contain own naming schema
    QString schema = Utility::namingSchema;
    schema.replace(QLatin1String("%title"), playlistItem->song()->songName(), Qt::CaseInsensitive);
    schema.replace(QLatin1String("%artist"), playlistItem->song()->artistName(), Qt::CaseInsensitive);
    schema.replace(QLatin1String("%album"), playlistItem->song()->albumName(), Qt::CaseInsensitive);
    schema.replace(QLatin1String("%track"), QString::number(playlistItem->song()->trackNum()), Qt::CaseInsensitive);

    QFileInfo fi(ui_->pathLine->text() + QDir::separator() + schema + ".mp3");

    // check if destination folder exists
    if(!QFile::exists(ui_->pathLine->text())) {
        QMessageBox::information(this, trUtf8("Attention"),
                                       trUtf8("The destination folder does not exists.\n"
                                              "Select a valid path"),
                                 QMessageBox::Ok);
        return;
    }

    // check if destination folder is writable
    if(!QFileInfo(ui_->pathLine->text()).isWritable()) {
        QMessageBox::information(this, trUtf8("Attention"),
                                       trUtf8("The destination folder is not writable.\n"
                                              "Select a valid path"),
                                 QMessageBox::Ok);
        return;
    }

    if(isDownloadingQueued(playlistItem->song()->songID())) {
        QMessageBox::information(this, trUtf8("Attention"),
                                       trUtf8("The song is already in queue."),
                                 QMessageBox::Ok);
        return;
    }

    // check file existence
    if(QFile::exists(ui_->pathLine->text() + QDir::separator() + schema + ".mp3")) {
        int ret = QMessageBox::question(this,
                                        trUtf8("Overwrite File?"),
                                        trUtf8("A file named \"%1\" already exists. Are you sure you want to overwrite it?").arg(playlistItem->fileName()),
                                        QMessageBox::Yes | QMessageBox::Cancel,
                                        QMessageBox::Cancel);
        if(ret == QMessageBox::Yes) {
            QFile::remove(ui_->pathLine->text() + QDir::separator() + playlistItem->fileName());
        } else {
            return;
        }
    }

    playlistItem->setPath(ui_->pathLine->text() + "/");
    playlistItem->setNamingSchema(Utility::namingSchema);

    addDownloadItem(playlistItem);
}


/*!
  \brief addDownloadItem : add a new download to the list
  \param index : index of item in results list (to download)
  \return void
*/
void MainWindow::addDownloadItem(PlaylistItemPtr playlistItem)
{
    QFileInfo fi(playlistItem->path() + playlistItem->fileName());

    // Creating path, if needed
    if(!fi.absoluteDir().exists()) {
        if(!fi.absoluteDir().mkpath(fi.absolutePath())) {
            QMessageBox::information(this, trUtf8("Attention"),
                                           trUtf8("Can't create destination path:\n\n%1\n\nAborting...").arg(fi.absolutePath()),
                                           QMessageBox::Ok);
            return;
        }
    }

    // build a DownloadItem with all required data
    DownloadItem *item = new DownloadItem(playlistItem,
                                          this);

    connect(item, SIGNAL(reloadPlaylist()), ui_->downloadList, SLOT(reloadPlaylist()));
    connect(item, SIGNAL(downloadFinished()), this, SLOT(freeDownloadSlot()));
    connect(item, SIGNAL(addToQueue(DownloadItem*)), this, SLOT(addItemToQueue(DownloadItem*)));

    if(!QFile::exists(playlistItem->path() + playlistItem->fileName())) {
        // check if download queue_ is full
        if(parallelDownloadsCount_ < maxDownloads_ && !token_->result().isEmpty()) {
            parallelDownloadsCount_++;
            item->startDownload();
        } else {
            queue_.append(item);
        }
    }

    QListWidgetItem *wi = new QListWidgetItem;
    ui_->downloadList->addItem(wi);
    ui_->downloadList->setItemWidget(wi, item);
    ui_->downloadList->setCurrentItem(wi);
    wi->setSizeHint(QSize(Utility::coverSize + Utility::marginSize * 2,
                          Utility::coverSize + Utility::marginSize * 2));
}

/*!
  \brief setCompactLayout : set vertical orientation for splitter
  \return void
*/
void MainWindow::setCompactLayout()
{
    QSettings settings;
    settings.setIniCodec( "UTF-8" );

    // if previous layout was Mini, restore saved splitter size/window geometry
    if(guiLayout_ == Mini) {
        ui_->splitter->setVisible(true);
        ui_->searchWidgets->setVisible(true);
        setMaximumHeight(16777215);
        ui_->splitter->setOrientation((Qt::Orientation)settings.value(QLatin1String("splitterOrientation"), Qt::Vertical).toInt());
        QList<int> _sizes = settings.value(QLatin1String("splitterSizes")).value< QList<int> >();

        if(!_sizes.isEmpty()) {
            ui_->splitter->setSizes(_sizes);
        }

        QRect rect = settings.value(QLatin1String("windowGeometry"), QRect(100,100,350,600)).toRect();
        setGeometry(geometry().x(), geometry().y(), geometry().width(), rect.height());
    }

    ui_->splitter->setOrientation(Qt::Vertical);
    playerWidget->showElapsedTimerLabel(false);

    // save choice
    settings.setValue(QLatin1String("splitterOrientation"), ui_->splitter->orientation());

    guiLayout_ = Compact;
}

/*!
  \brief setWideLayout : set horizontal orientation for splitter
  \return void
*/
void MainWindow::setWideLayout()
{
    QSettings settings;
    settings.setIniCodec( "UTF-8" );

    // if previous layout was Mini, restore saved splitter size/window geometry
    if(guiLayout_ == Mini) {
        ui_->splitter->setVisible(true);
        ui_->searchWidgets->setVisible(true);
        setMaximumHeight(16777215);
        ui_->splitter->setOrientation((Qt::Orientation)settings.value(QLatin1String("splitterOrientation"), Qt::Vertical).toInt());
        QList<int> _sizes = settings.value(QLatin1String("splitterSizes")).value< QList<int> >();

        if(!_sizes.isEmpty()) {
            ui_->splitter->setSizes(_sizes);
        }

        QRect rect = settings.value(QLatin1String("windowGeometry"), QRect(100,100,350,600)).toRect();
        setGeometry(geometry().x(), geometry().y(), geometry().width(), rect.height());
    }

    ui_->splitter->setOrientation(Qt::Horizontal);
    playerWidget->showElapsedTimerLabel(true);

    // save choice
    settings.setValue(QLatin1String("splitterOrientation"), ui_->splitter->orientation());
    guiLayout_ = Wide;
}

void MainWindow::setMiniPlayerLayout()
{
    // if previous layout was Compact/Wide, save gemetry/splitter data
    if(guiLayout_ == Compact || guiLayout_ == Wide) {
        QSettings settings;
        settings.setIniCodec( "UTF-8" );
        settings.setValue(QLatin1String("windowGeometry"), geometry());
    }

    ui_->splitter->setVisible(false);
    ui_->searchWidgets->setVisible(false);
    playerWidget->showElapsedTimerLabel(true);
    setMaximumHeight(40);
    guiLayout_ = Mini;
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
        getToken();
    } else {
        //statusBar()->showMessage(trUtf8("Offline"),0);
        playerWidget->showMessage(trUtf8("Offline"));
        ui_->searchButton->setEnabled(false);
        ui_->qled->setValue(false);
        ui_->matchList->setEnabled(false);
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

    saveSession_     = settings.value(QLatin1String("saveSession"), false).toBool();
    showHistory_     = settings.value(QLatin1String("saveSearches"), false).toBool();
    maxResults_      = settings.value(QLatin1String("numResults"), 0).toInt();
    loadCovers_      = settings.value(QLatin1String("loadCovers"), true).toBool();
    maxDownloads_    = settings.value(QLatin1String("maxDownloads"), 5).toInt();
    saveDestination_ = settings.value(QLatin1String("saveDestination"), false).toBool();
    guiLayout_       = (GuiLayout)settings.value(QLatin1String("guiLayout"), Compact).toInt();
    sessionFileName_ = settings.value(QLatin1String("sessionFile"), QLatin1String("default")).toString();

    if(showHistory_) {
        searchSize_ = settings.value(QLatin1String("historySize"), 5).toInt();
        searchList_ << settings.value(QLatin1String("searchTerms"), QStringList()).toStringList();
        setupCompleter();
    }

    ui_->splitter->setOrientation((Qt::Orientation)settings.value(QLatin1String("splitterOrientation"), Qt::Vertical).toInt());
    playerWidget->showElapsedTimerLabel(ui_->splitter->orientation() == Qt::Vertical ? false : true);

    QList<int> _sizes = settings.value(QLatin1String("splitterSizes")).value< QList<int> >();

    if(!_sizes.isEmpty()) {
        ui_->splitter->setSizes(_sizes);
    }

    The::audioEngine()->setVolume(settings.value(QLatin1String("volume"), 50).toInt());
    The::audioEngine()->setMuted(settings.value(QLatin1String("muted"), false).toBool());

    playerWidget->setTimerState((GrooveOff::TimerState)settings.value(QLatin1String("timerState"), GrooveOff::ElapsedState).toInt());

    //Naming Schema
    Utility::namingSchema = settings.value(QLatin1String("namingSchema"),
                                        trUtf8("%1 - %2").arg(QLatin1String("%artist")).arg(QLatin1String("%title"))).toString();

    if(guiLayout_ == Mini)
        setMiniPlayerLayout();
}

/*!
  \brief saveSettings : save settings
  \return void
*/
void MainWindow::saveSettings()
{
    QSettings settings;
    settings.setIniCodec( "UTF-8" );

    // Do not overwrite splittersSizes/windowGeometry if mini layout
    if(guiLayout_ != Mini) {
        settings.setValue(QLatin1String("splitterSizes"), QVariant::fromValue< QList<int> >(ui_->splitter->sizes()));
        settings.setValue(QLatin1String("windowGeometry"), geometry());
    } else {
        // if mini layout, save only x,y,width
        QRect rect = settings.value(QLatin1String("windowGeometry"), QRect(100,100,350,600)).toRect();
        rect.setWidth(geometry().width());
        settings.setValue(QLatin1String("windowGeometry"), rect);
    }

    if(!showHistory_)
        settings.setValue(QLatin1String("searchTerms"), QStringList());
    else
        settings.setValue(QLatin1String("searchTerms"), searchList_);

    if(saveDestination_)
        settings.setValue(QLatin1String("destination"), ui_->pathLine->text());
    else
        settings.setValue(QLatin1String("destination"), QString());

    settings.setValue(QLatin1String("timerState"), playerWidget->getTimerState());
    settings.setValue(QLatin1String("muted"), The::audioEngine()->isMuted());
    settings.setValue(QLatin1String("volume"), The::audioEngine()->volume());
    settings.setValue(QLatin1String("guiLayout"), guiLayout_);
    settings.setValue(QLatin1String("sessionFile"), sessionFileName_);
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
    QString artist = ui_->artistsCB->currentText();
    QString album = ui_->albumsCB->currentText();

    for(int i = 0; i < ui_->matchList->count(); i++) {
        QString itemArtist = ((MatchItem *)ui_->matchList->itemWidget(ui_->matchList->item(i)))->playlistItem()->song()->artistName();
        QString itemAlbum  = ((MatchItem *)ui_->matchList->itemWidget(ui_->matchList->item(i)))->playlistItem()->song()->albumName();

        if( ui_->artistsCB->currentIndex() == 0  && ui_->albumsCB->currentIndex() == 0) {
            ui_->matchList->setRowHidden(i, false);
        } else if( ui_->artistsCB->currentIndex() == 0 ) {
            if( itemAlbum == album )
                ui_->matchList->setRowHidden(i, false);
            else
                ui_->matchList->setRowHidden(i, true);
        } else if( ui_->albumsCB->currentIndex() == 0 ) {
            if( itemArtist == artist )
                ui_->matchList->setRowHidden(i, false);
            else
                ui_->matchList->setRowHidden(i, true);
        } else {
            if( itemArtist == artist && itemAlbum == album   )
                ui_->matchList->setRowHidden(i, false);
            else
                ui_->matchList->setRowHidden(i, true);
        }
    }

    ui_->matchesMessage->setText(trUtf8("%n song(s) found", "", visibleItemsCount()));
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
bool MainWindow::isDownloadingQueued(const uint &id)
{
    // check if file is currently downloading
    for(int i = 0; i < ui_->downloadList->count(); i++) {
        GrooveOff::DownloadState state = qobject_cast<DownloadItem *>(ui_->downloadList->itemWidget(ui_->downloadList->item(i)))->downloadState();
        if(state != GrooveOff::DeletedState) {
            if(id == qobject_cast<DownloadItem *>(ui_->downloadList->itemWidget(ui_->downloadList->item(i)))->playlistItem()->song()->songID()) {
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

int MainWindow::visibleItemsCount()
{
    int count = 0;
    for(int i = 0; i < ui_->matchList->count(); i++) {
        if(!ui_->matchList->isRowHidden(i))
            count++;
    }

    return count;
}

void MainWindow::reloadItemsDownloadButtons()
{
    for(int i = 0; i < ui_->matchList->count(); i++) {
        qobject_cast<MatchItem *>(ui_->matchList->itemWidget(ui_->matchList->item(i)))->setDownloadIcon();
    }
}

void MainWindow::batchDownload()
{
    for(int i = 0; i < ui_->matchList->count(); i++) {
        if(!ui_->matchList->item(i)->isHidden())
            addDownloadItem(((MatchItem *)ui_->matchList->itemWidget(ui_->matchList->item(i)))->playlistItem());
    }
}

void MainWindow::saveSessionAs()
{
    bool ok;
    QString fileName = QInputDialog::getText(this, tr("New Session"),
                                         trUtf8("Session name:"), QLineEdit::Normal,
                                         QDir::home().dirName(), &ok);
    // remove separators, if any
    fileName.remove('/');
    fileName.remove('\\');

    if (ok && !fileName.isEmpty()) {
        // save current session
        saveSession();
        // change session name
        sessionFileName_ = fileName;
    }
}

void MainWindow::saveSession()
{
    The::sessionReaderWriter()->write( sessionFilePath_ + sessionFileName_ + QLatin1String(".xml"),
                                       ui_->downloadList->playlistItems() );
}

void MainWindow::loadSession()
{
    // disable temporarily AutoScroll
    ui_->downloadList->setAutoScroll(false);

    // remove old entries
    ui_->downloadList->abortAllDownloads();
    ui_->downloadList->removeDownloaded();
    ui_->downloadList->removeFailedDeletedAborted();

    QList<PlaylistItemPtr> items = The::sessionReaderWriter()->read(sessionFilePath_ + sessionFileName_ + QLatin1String(".xml"));

    foreach(PlaylistItemPtr item, items) {
        addDownloadItem(item);
    }

    ui_->downloadList->setAutoScroll(true);
    ui_->downloadList->reloadPlaylist();

    // FIXME
    // loadSessions();
}

void MainWindow::loadSessions()
{
    foreach(QAction *action, ActionCollection::instance()->getMenu("sessionsMenu")->actions()) {
        ActionCollection::instance()->getMenu("sessionsMenu")->removeAction(action);
        delete action;
    }

    QDir sessionPath(sessionFilePath_);
    QStringList sessions = sessionPath.entryList(QStringList() << "*.xml", QDir::Files, QDir::Name);

    foreach(QString session, sessions) {
        QAction *action = new QAction(session.remove(".xml", Qt::CaseInsensitive), ActionCollection::instance()->getMenu("sessionsMenu"));
        if( session == sessionFileName_ )
            action->setFont(Utility::font(QFont::Bold));
        ActionCollection::instance()->getMenu("sessionsMenu")->addAction(action);
        connect(action, SIGNAL(triggered()),
                        SLOT(loadSessionFile()));
    }
}

void MainWindow::loadSessionFile()
{
    QAction *action = (QAction *)QObject::sender();
    QString fileName = action->text();
    if(fileName != sessionFileName_ && QFile::exists(sessionFilePath_ + fileName + QLatin1String(".xml"))) {
        // ask if abort current downloads
        saveSession();
        ui_->downloadList->abortAllDownloads();
        ui_->downloadList->removeDownloaded();
        ui_->downloadList->removeFailedDeletedAborted();
        sessionFileName_ = fileName;

        loadSession();
    }
}

void MainWindow::restoreSearch()
{
    searchInProgress_ = false;
    ui_->searchButton->setVisible(true);

    ui_->spinnerWidget->setVisible(false);
    spinner_->stop();
}

void MainWindow::changeEvent( QEvent *event )
{
    if( event->type() == QEvent::PaletteChange )
        The::paletteHandler()->setPalette( palette() );
}

void MainWindow::openSessionManager()
{
    SessionManager manager(sessionFilePath_);
    manager.exec();
    if(!QFile::exists(sessionFilePath_ + sessionFileName_ + QLatin1String(".xml"))) {
        if(The::audioEngine()->state() == Phonon::PlayingState ||
            The::audioEngine()->state() == Phonon::PausedState )
            The::audioEngine()->stop();

        sessionFileName_ = QLatin1String( "default" );
    }

    loadSession();
}

