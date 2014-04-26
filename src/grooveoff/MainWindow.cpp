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
    ui(new Ui::MainWindow)
{
    qRegisterMetaType<PlaylistItemPtr>("PlaylistItemPtr");
    qRegisterMetaTypeStreamOperators<PlaylistItemPtr>("PlaylistItemPtr");

    ui->setupUi(this);
    s_instance = this;
    The::paletteHandler()->setPalette( palette() );
    m_playerWidget = new PlayerWidget(this);
    new ActionCollection(this);
    setupMenus();
    setupUi();
    setupSignals();
    loadSettings();
    statusBar()->addPermanentWidget( m_playerWidget, 1 );
    statusBar()->setSizeGripEnabled(false);

    m_spinner = new Spinner(ui->spinnerWidget);
    m_spinner->setText(tr(""));
    m_spinner->setType(Spinner::Sun);

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

    m_nam = new QNetworkAccessManager(this);
    m_api = ApiRequest::instance();
    m_jar = new MyJar;
    m_nam->setCookieJar(m_jar);

    m_cvrMngr = new CoverManager(this);

    //fake
    m_searchInProgress = true;

    setWindowTitle(QString::fromLatin1( "GrooveOff %1" ).arg( GROOVEOFF_VERSION ));

    if(m_guiLayout == Mini) {
        ActionCollection::instance()->getAction( QLatin1String( "miniPlayer" ) )->setChecked(true);
        m_playerWidget->showElapsedTimerLabel(true);
    } else if(ui->splitter->orientation() == Qt::Vertical) {
        ActionCollection::instance()->getAction( QLatin1String( "actionCompact" ) )->setChecked(true);
        m_playerWidget->showElapsedTimerLabel(false);
    }
    else {
        ActionCollection::instance()->getAction( QLatin1String( "actionWide" ) )->setChecked(true);
        m_playerWidget->showElapsedTimerLabel(true);
    }

    QTime time = QTime::currentTime();
    qsrand((uint)time.msec());

    m_qncm = new QNetworkConfigurationManager(this);
    
    connect(m_qncm, SIGNAL(onlineStateChanged(bool)),
                    SLOT(onlineStateChanged(bool)));
    
    if(m_qncm->isOnline())
        getToken();

    m_parallelDownloadsCount = 0;

    QSettings settings;
    settings.setIniCodec( "UTF-8" );

#if QT_VERSION >= QT_VERSION_CHECK( 5, 0, 0 )
    if(m_saveDestination) {
        ui->pathLine->setText(settings.value(QLatin1String("destination"),
                               QStandardPaths::writableLocation(QStandardPaths::MusicLocation)).toString());
        if(ui->pathLine->text().isEmpty())
            ui->pathLine->setText(QStandardPaths::writableLocation(QStandardPaths::MusicLocation));
    } else {
        ui->pathLine->setText(QStandardPaths::writableLocation(QStandardPaths::MusicLocation));
    }
#else
    if(m_saveDestination) {
        ui->pathLine->setText(settings.value(QLatin1String("destination"),
                               QDesktopServices::storageLocation(QDesktopServices::MusicLocation)).toString());
        if(ui->pathLine->text().isEmpty())
            ui->pathLine->setText(QDesktopServices::storageLocation(QDesktopServices::MusicLocation));
    } else {
        ui->pathLine->setText(QDesktopServices::storageLocation(QDesktopServices::MusicLocation));
    }
#endif

    setGeometry(settings.value(QLatin1String("windowGeometry"), QRect(100,100,350,600)).toRect());

#if QT_VERSION >= QT_VERSION_CHECK( 5, 0, 0 )
    m_sessionFilePath = QStandardPaths::writableLocation(QStandardPaths::DataLocation)
                   + QDir::separator();
#else
    m_sessionFilePath = QDesktopServices::storageLocation(QDesktopServices::DataLocation).replace( QLatin1String( "/data" ),"")
                   + QDir::separator();
#endif
    if(m_saveSession)
        loadSession();

    // FIXME
    // loadSessions();

    m_mpris = new Mpris(this);
}

/*!
  \brief ~MainWindow: this is the MainWindow destructor.
*/
MainWindow::~MainWindow()
{
    delete The::paletteHandler();
    delete The::svgHandler();

    m_spinner->stop();

    if(m_saveSession)
        saveSession();
    
    if(m_emptyCache) {
        QDir coversPath(Utility::coversCachePath);
        QStringList covers = coversPath.entryList(QStringList() << "*.jpg" << "*.png", QDir::Files, QDir::Name);

        foreach(QString cover, covers) {
            QFile::remove(Utility::coversCachePath + QDir::separator() + cover);
        }
    }

    saveSettings();

    delete m_api;
    delete m_mpris;
}

/*!
  \brief setupUi: setup widgets
  \return void
*/
void MainWindow::setupUi()
{
    ui->label0->setPixmap(QPixmap(QLatin1String(":/resources/grooveoff.png")));
    ui->label0->setMinimumHeight(30);
    ui->label0->setMaximumWidth(30);
    ui->label0->setScaledContents(30);

    ui->label1->setText(trUtf8("GrooveOff"));
    QFontMetrics fmTitle(Utility::font(QFont::Bold, 2));
    ui->label1->setFont(Utility::font(QFont::Bold, 2));
    ui->label2->setText(trUtf8("Offline Grooveshark.com music"));

//    statusBar()->showMessage(trUtf8("Connecting...", 0));
    m_playerWidget->showMessage(trUtf8("Connecting..."));

    // Led
    ui->qled->setFixedSize(QSize(24,24));

    QFontMetrics fmSystemFont(Utility::font(QFont::Bold));
    int fontHeight = fmSystemFont.height();

    ui->label3->setText(trUtf8("Search:"));
    ui->label3->setBuddy(ui->searchLine);
    ui->searchLine->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    ui->searchLine->setFixedHeight(fontHeight > 24 ? fontHeight : 24);
    ui->searchLine->setToolTip(trUtf8("Search for songs, artists, genres, playlists"));
    ui->searchLine->setPlaceholderText(trUtf8("Search for songs, artists, genres, playlists"));

    ui->searchButton->setIcon(QIcon::fromTheme(QLatin1String("system-search")));
    ui->searchButton->setIconSize(QSize(16,16));
    ui->searchButton->setFixedHeight(fontHeight > 25 ? fontHeight : 25);
    ui->searchButton->setEnabled(false);
    ui->searchButton->setToolTip(trUtf8("Start search"));

    ui->label4->setText(trUtf8("Save in:"));
    ui->label4->setBuddy(ui->pathLine);

    ui->pathLine->setToolTip(trUtf8("Current save folder"));
    ui->pathLine->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    ui->pathLine->setFixedHeight(fontHeight > 24 ? fontHeight : 24);

    // Completer used to save search history
    QCompleter *completer = new QCompleter(this);
    completer->setModel(new QDirModel(completer));
    ui->pathLine->setCompleter(completer);

    ui->browseButton->setToolTip(trUtf8("Select save foder"));
    ui->browseButton->setIcon(QIcon::fromTheme(QLatin1String("folder-open")));
    ui->browseButton->setIconSize(QSize(16,16));
    ui->browseButton->setFixedHeight(fontHeight > 25 ? fontHeight : 25);

    ui->spinnerWidget->setFixedHeight(fontHeight > 25 ? fontHeight : 25);
    ui->spinnerWidget->setVisible(false);

    ui->combosContainer->setVisible(false);

    ui->batchDownloadButton->setToolTip(trUtf8("Download all tracks"));
    ui->batchDownloadButton->setIcon(QIcon::fromTheme(QLatin1String("kget")));
    ui->batchDownloadButton->setIconSize(QSize(16,16));
    ui->batchDownloadButton->setFixedHeight(fontHeight > 25 ? fontHeight : 25);

    ui->matchesMessage->setFont(Utility::font(QFont::Bold));

    ui->splitter->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);

    m_playerWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    
    ui->compactMenuButton->setIcon( QIcon::fromTheme( "applications-system" ) );
    ui->compactMenuButton->setToolTip( trUtf8( "Main Menu" ) );
    ui->compactMenuButton->setMenu( m_compactMainMenu );
    ui->compactMenuButton->setToolButtonStyle( Qt::ToolButtonIconOnly );
    ui->compactMenuButton->setAutoRaise(true);
    ui->compactMenuButton->setIconSize(QSize(24,24));
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
            ui->downloadList,                       SLOT(abortAllDownloads()));

    connect(ac->getAction( "actionRemoveFailed" ), SIGNAL(triggered(bool)),
            ui->downloadList,                      SLOT(removeFailedDeletedAborted()));

    connect(ac->getAction( "actionClearDownloadList" ), SIGNAL(triggered(bool)),
            ui->downloadList,                           SLOT(removeDownloaded()));

    connect(ac->getAction( "actionAbout" ), SIGNAL(triggered()),
                                            SLOT(about()));

    connect(ac->getAction( "actionSaveSessionAs" ), SIGNAL(triggered()),
                                                    SLOT(saveSessionAs()));

    connect(ac->getAction( "actionManageSessions" ), SIGNAL(triggered()),
                                                     SLOT(openSessionManager()));

    connect(ac->getAction( "actionQtAbout" ), SIGNAL(triggered()),
            qApp,                             SLOT(aboutQt()));


    // gui widgets
    connect(ui->searchButton, SIGNAL(clicked(bool)),
                              SLOT(beginSearch()));
    
    connect(ui->browseButton, SIGNAL(clicked(bool)),
                              SLOT(selectFolder()));
    
    connect(ui->searchLine, SIGNAL(returnPressed()),
                            SLOT(beginSearch()));
    
    connect(ui->artistsCB, SIGNAL(activated(int)),
                           SLOT(artistChanged()));
    
    connect(ui->albumsCB, SIGNAL(activated(int)),
                          SLOT(albumChanged()));
    
    connect(ui->batchDownloadButton, SIGNAL(clicked()),
                                     SLOT(batchDownload()));
    
    connect(ui->pathLine, SIGNAL(textChanged(QString)),
                          SLOT(changeDestinationPath()));
}


/*!
  \brief selectFolder: select save folder
  \return void
*/
void MainWindow::selectFolder()
{
    QString dir = QFileDialog::getExistingDirectory(this, trUtf8("Select Directory"),
                                              ui->pathLine->text().isEmpty() ? QDir::homePath() : ui->pathLine->text(),
                                              QFileDialog::ShowDirsOnly
                                              | QFileDialog::DontResolveSymlinks);

    // be sure that a valid path was selected
    if(QFile::exists(dir))
        ui->pathLine->setText(dir);

    changeDestinationPath();
}

void MainWindow::changeDestinationPath()
{
    Utility::destinationPath = ui->pathLine->text();
    reloadItemsDownloadButtons();
}


/*!
  \brief beginSearch: start search
  \return void
*/
void MainWindow::beginSearch()
{
    // stop a search if one operation is still in progress
    if(m_searchInProgress)
        return;

    // abort search if search field is empty
    if(ui->searchLine->text().isEmpty())
        return;

    // if save searches history append last search and remove oldest if history is full
    if(m_showHistory) {
        if(m_searchList.count() >= m_searchSize)
            m_searchList.removeFirst();

        m_searchList.append(ui->searchLine->text().toLower());
        m_searchList.removeDuplicates();

        setupCompleter();
    }

//    qDebug() << "GrooveOff ::" << "Searching for" << ui->searchLine->text();

    // flag that prevents multiple searches
    m_searchInProgress = true;

    // some ui setups
    ui->searchButton->setVisible(false);

    ui->spinnerWidget->setVisible(true);
    m_spinner->start(0);

    // clear cover manager
    m_cvrMngr->clear();

    // clear listWidget
    for(int i = ui->matchList->count() - 1; i >= 0; i--) {
        QListWidgetItem *item = ui->matchList->takeItem(i);
        ui->downloadList->removeItemWidget(item);
        delete item;
    }

    // clear artists, albums containers
    ui->artistsCB->clear();
    ui->artistsCB->addItem(trUtf8("All Artists"));
    ui->albumsCB->clear();
    ui->albumsCB->addItem(trUtf8("All Albums"));
    m_artistsAlbumsContainer.clear();

    m_songList = m_api->songs(ui->searchLine->text(), Utility::token);

    connect(m_songList.data(), SIGNAL(finished()),
                               SLOT(searchFinished()));

    connect(m_songList.data(), SIGNAL(parseError()),
                               SLOT(searchError()));

    connect(m_songList.data(), SIGNAL(requestError(QNetworkReply::NetworkError)),
                               SLOT(searchError()));

    // force downloadList repaint to prevent blank items
    ui->downloadList->repaint();
}

/*!
  \brief getToken: get the token
  \return void
*/
void MainWindow::getToken()
{
    ui->searchLine->setStyleSheet( QLatin1String( "background-color:#E7A2A9;" ) );

    // clear cookies
    m_jar->clear();

    // some ui setups
    ui->qled->setValue(false);
    //statusBar()->showMessage(trUtf8("Connecting..."), 0);
    m_playerWidget->showMessage(trUtf8("Connecting..."));

    m_token = m_api->token();

    connect( m_token.data(), SIGNAL(finished()),
                             SLOT(tokenFinished()));

    connect(m_token.data(), SIGNAL(parseError()),
                            SLOT(tokenError()));

    connect(m_token.data(), SIGNAL(requestError(QNetworkReply::NetworkError)),
                            SLOT(tokenError()));
}

void MainWindow::tokenFinished()
{
    // the application is now free to perform a search
    m_searchInProgress = false;
    ui->searchButton->setEnabled(true);

    if(!m_token->result().isEmpty()) {
        ui->searchLine->setStyleSheet( QLatin1String( "" ) );

        Utility::token = m_token->result();

        //statusBar()->showMessage(trUtf8("Connected"), 3000);
        m_playerWidget->showMessage(trUtf8("Connected to\nGrooveshark"));
        ui->qled->setValue(true);
        ui->qled->setToolTip(trUtf8("You're connected to Grooveshark!"));
        ui->matchList->setEnabled(true);

        // start download of queued songs from last session
        unqueue();
    } else {
        //statusBar()->showMessage(trUtf8("Token not received!!"), 3000);
        m_playerWidget->showMessage(trUtf8("Connection error!!"));
        qDebug() << "GrooveOff ::" << "Token not received!!";
        Utility::token.clear();
    }
}

void MainWindow::tokenError()
{
    m_playerWidget->showMessage(trUtf8("Connection error!!"));
    qDebug() << "GrooveOff ::" << m_token->errorString();
}

/*!
  \brief searchFinished : fills results list
  \return void
*/
void MainWindow::searchFinished()
{
    restoreSearch();

    // check if last search returned results
    if(m_songList->list().count() == 0) {
        qDebug() << "GrooveOff ::" << "Empty result list";
        return;
    }

    // row index (start from 0)
    m_row = 0;

    // 'count' contains number of elements to display
    int count;

    if(m_maxResults == 0) // no results limit
        count = m_songList->list().count();
    else // limit results for performance
        count = qMin(m_maxResults, m_songList->list().count());

    ui->combosContainer->setVisible(true);

    QStringList artists;
    QStringList albums;

    ui->matchesMessage->setText(trUtf8("%n song(s) found", "", count));

    for(int i = 0; i < count; i++) {
        PlaylistItemPtr playlistItem(new PlaylistItem(m_songList->list().at(i)));

        // Decide if show cover arts
        if(m_loadCovers && !QFile::exists(Utility::coversCachePath + m_songList->list().at(i)->coverArtFilename())) {
            m_cvrMngr->addItem(playlistItem);
        }

        // build a MathItem with all required data
        MatchItem *matchItem = new MatchItem(playlistItem, this);
        QListWidgetItem *wItem = new QListWidgetItem;
        ui->matchList->addItem(wItem);
        ui->matchList->setItemWidget(wItem, matchItem);
        wItem->setSizeHint(QSize(Utility::coverSize + Utility::marginSize * 2,Utility::coverSize + Utility::marginSize * 2));
        
        connect(matchItem, SIGNAL(download(PlaylistItemPtr)),
                           SLOT(downloadRequest(PlaylistItemPtr)));

        // don't freeze gui inserting items
        QCoreApplication::processEvents();

        // populate filter widgets
        bool found = false;
        int j = 0;
        for(; j < m_artistsAlbumsContainer.count(); j++) {
            if(m_artistsAlbumsContainer.at(j).first == m_songList->list().at(i)->artistName()) {
                found = true;
                break;
            }
        }

        if(found) {
            bool albumExists = false;
            for(int k = 0; k < m_artistsAlbumsContainer[j].second.count(); k++) {
                if(m_artistsAlbumsContainer[j].second.at(k) == m_songList->list().at(i)->albumName())
                    albumExists = true;
            }
            if(!albumExists) {
                m_artistsAlbumsContainer[j].second << m_songList->list().at(i)->albumName();
                m_artistsAlbumsContainer[j].second.sort();
            }
        } else {
            QPair<QString, QStringList> e;
            e.first = m_songList->list().at(i)->artistName();
            e.second << m_songList->list().at(i)->albumName();
            m_artistsAlbumsContainer.append(e);

            if(!m_songList->list().at(i)->artistName().isEmpty())
                artists << m_songList->list().at(i)->artistName();
        }

        if(!m_songList->list().at(i)->albumName().isEmpty())
            albums << m_songList->list().at(i)->albumName();
    }

    // removing duplicates
    artists.removeDuplicates();
    albums.removeDuplicates();

    // sorting items
    artists.sort();
    albums.sort();

    ui->artistsCB->addItems(artists);
    ui->albumsCB->addItems(albums);
    applyFilter();

    // force downloadList repaint to prevent blank items
    ui->downloadList->repaint();
}

void MainWindow::searchError()
{
    qDebug() << "GrooveOff ::" << m_songList->errorString();
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

    QFileInfo fi(ui->pathLine->text() + QDir::separator() + schema + ".mp3");

    // check if destination folder exists
    if(!QFile::exists(ui->pathLine->text())) {
        QMessageBox::information(this, trUtf8("Attention"),
                                       trUtf8("The destination folder does not exists.\n"
                                              "Select a valid path"),
                                 QMessageBox::Ok);
        
        // if a batch download is in progress show the message above then stop all downloads.
        if(m_batchDownload)
            m_stopBatchDownload = true;
        
        return;
    }
    
    // check file existence
    if(QFile::exists(ui->pathLine->text() + QDir::separator() + schema + ".mp3")) {
        if(m_batchDownload)
            return;
        
        int ret = QMessageBox::question(this,
                                        trUtf8("Overwrite File?"),
                                        trUtf8("A file named \"%1\" already exists. Are you sure you want to overwrite it?").arg(playlistItem->fileName()),
                                        QMessageBox::Yes | QMessageBox::Cancel,
                                        QMessageBox::Cancel);
        if(ret == QMessageBox::Yes) {
            QFile::remove(ui->pathLine->text() + QDir::separator() + playlistItem->fileName());
        } else {
            return;
        }
    }

    // check if destination folder is writable
    if(!QFileInfo(ui->pathLine->text()).isWritable()) {
        QMessageBox::information(this, trUtf8("Attention"),
                                       trUtf8("The destination folder is not writable.\n"
                                              "Select a valid path"),
                                 QMessageBox::Ok);
        
        // if a batch download is in progress show the message above then stop all downloads.
        if(m_batchDownload)
            m_stopBatchDownload = true;
        
        return;
    }

    if(isDownloadingQueued(playlistItem->song()->songID())) {
        if(!m_batchDownload) {
            QMessageBox::information(this, trUtf8("Attention"),
                                     trUtf8("The song is already in queue."),
                                     QMessageBox::Ok);
        }
        return;
    }

    playlistItem->setPath(ui->pathLine->text() + "/");
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
            if(!m_batchDownload) {
                QMessageBox::information(this, trUtf8("Attention"),
                                         trUtf8("Can't create destination path:\n\n%1\n\nAborting...").arg(fi.absolutePath()),
                                         QMessageBox::Ok);
            }
            return;
        }
    }

    // build a DownloadItem with all required data
    DownloadItem *item = new DownloadItem(playlistItem,
                                          this);

    connect(item,             SIGNAL(reloadPlaylist()),
            ui->downloadList, SLOT(reloadPlaylist()));
    
    connect(item, SIGNAL(downloadFinished()), 
                  SLOT(freeDownloadSlot()));
    
    connect(item, SIGNAL(addToQueue(DownloadItem*)), 
                  SLOT(addItemToQueue(DownloadItem*)));

    if(!QFile::exists(playlistItem->path() + playlistItem->fileName())) {
        // check if download m_queue is full
        if(m_parallelDownloadsCount < m_maxDownloads && !m_token->result().isEmpty()) {
            m_parallelDownloadsCount++;
            item->startDownload();
        } else {
            m_queue.append(item);
        }
    }

    QListWidgetItem *wi = new QListWidgetItem;
    ui->downloadList->addItem(wi);
    ui->downloadList->setItemWidget(wi, item);
    ui->downloadList->setCurrentItem(wi);
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
    if(m_guiLayout == Mini) {
        ui->splitter->setVisible(true);
        ui->searchWidgets->setVisible(true);
        setMaximumHeight(16777215);
        ui->splitter->setOrientation((Qt::Orientation)settings.value(QLatin1String("splitterOrientation"), Qt::Vertical).toInt());
        QList<int> _sizes = settings.value(QLatin1String("splitterSizes")).value< QList<int> >();

        if(!_sizes.isEmpty()) {
            ui->splitter->setSizes(_sizes);
        }

        QRect rect = settings.value(QLatin1String("windowGeometry"), QRect(100,100,350,600)).toRect();
        setGeometry(geometry().x(), geometry().y(), geometry().width(), rect.height());
    }

    ui->splitter->setOrientation(Qt::Vertical);
    m_playerWidget->showElapsedTimerLabel(false);

    // save choice
    settings.setValue(QLatin1String("splitterOrientation"), ui->splitter->orientation());

    m_guiLayout = Compact;
    
    ui->compactMenuButton->setVisible(false);
    menuBar()->setVisible(true);
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
    if(m_guiLayout == Mini) {
        ui->splitter->setVisible(true);
        ui->searchWidgets->setVisible(true);
        setMaximumHeight(16777215);
        ui->splitter->setOrientation((Qt::Orientation)settings.value(QLatin1String("splitterOrientation"), Qt::Vertical).toInt());
        QList<int> _sizes = settings.value(QLatin1String("splitterSizes")).value< QList<int> >();

        if(!_sizes.isEmpty()) {
            ui->splitter->setSizes(_sizes);
        }

        QRect rect = settings.value(QLatin1String("windowGeometry"), QRect(100,100,350,600)).toRect();
        setGeometry(geometry().x(), geometry().y(), geometry().width(), rect.height());
    }

    ui->splitter->setOrientation(Qt::Horizontal);
    m_playerWidget->showElapsedTimerLabel(true);

    // save choice
    settings.setValue(QLatin1String("splitterOrientation"), ui->splitter->orientation());
    m_guiLayout = Wide;
    
    ui->compactMenuButton->setVisible(false);
    menuBar()->setVisible(true);
}

void MainWindow::setMiniPlayerLayout()
{
    // if previous layout was Compact/Wide, save gemetry/splitter data
    if(m_guiLayout == Compact || m_guiLayout == Wide) {
        QSettings settings;
        settings.setIniCodec( "UTF-8" );
        settings.setValue(QLatin1String("windowGeometry"), geometry());
        settings.setValue(QLatin1String("splitterSizes"), QVariant::fromValue< QList<int> >(ui->splitter->sizes()));
    }

    ui->splitter->setVisible(false);
    ui->searchWidgets->setVisible(false);
    ui->compactMenuButton->setVisible(true);
    menuBar()->setVisible(false);
    m_playerWidget->showElapsedTimerLabel(true);
    setMaximumHeight(40);
    m_guiLayout = Mini;
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
        m_playerWidget->showMessage(trUtf8("Offline"));
        ui->searchButton->setEnabled(false);
        ui->qled->setValue(false);
        ui->matchList->setEnabled(false);
    }
}

/*!
  \brief setupCompleter : setup a completer for search line edit
  \return void
*/
void MainWindow::setupCompleter()
{
    QCompleter *completer = new QCompleter(m_searchList, this);
    completer->setCaseSensitivity(Qt::CaseInsensitive);
    ui->searchLine->setCompleter(completer);
}

/*!
  \brief loadSettings : load settings
  \return void
*/
void MainWindow::loadSettings()
{
    QSettings settings;
    settings.setIniCodec( "UTF-8" );

    m_saveSession     = settings.value(QLatin1String("saveSession"), true).toBool();
    m_showHistory     = settings.value(QLatin1String("saveSearches"), false).toBool();
    m_maxResults      = settings.value(QLatin1String("numResults"), 0).toInt();
    m_loadCovers      = settings.value(QLatin1String("loadCovers"), true).toBool();
    if(m_loadCovers)
        m_emptyCache = settings.value(QLatin1String("emptyCache"), false).toBool();
    else
        m_emptyCache = true;
    m_maxDownloads    = settings.value(QLatin1String("maxDownloads"), 5).toInt();
    m_saveDestination = settings.value(QLatin1String("saveDestination"), true).toBool();
    m_guiLayout       = (GuiLayout)settings.value(QLatin1String("guiLayout"), Compact).toInt();
    m_sessionFileName = settings.value(QLatin1String("sessionFile"), QLatin1String("default")).toString();

    if(m_showHistory) {
        m_searchSize = settings.value(QLatin1String("historySize"), 5).toInt();
        m_searchList << settings.value(QLatin1String("searchTerms"), QStringList()).toStringList();
        setupCompleter();
    }

    ui->splitter->setOrientation((Qt::Orientation)settings.value(QLatin1String("splitterOrientation"), Qt::Vertical).toInt());
    m_playerWidget->showElapsedTimerLabel(ui->splitter->orientation() == Qt::Vertical ? false : true);

    QList<int> _sizes = settings.value(QLatin1String("splitterSizes")).value< QList<int> >();

    if(!_sizes.isEmpty()) {
        ui->splitter->setSizes(_sizes);
    }

    The::audioEngine()->setVolume(settings.value(QLatin1String("volume"), 50).toInt());
    The::audioEngine()->setMuted(settings.value(QLatin1String("muted"), false).toBool());

    m_playerWidget->setTimerState((GrooveOff::TimerState)settings.value(QLatin1String("timerState"), GrooveOff::ElapsedState).toInt());

    //Naming Schema
    Utility::namingSchema = settings.value(QLatin1String("namingSchema"),
                                           QLatin1String("%artist/%album/%track - %title")).toString();

    if(m_guiLayout == Mini)
        setMiniPlayerLayout();
    else
        ui->compactMenuButton->setVisible(false);
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
    if(m_guiLayout != Mini) {
        settings.setValue(QLatin1String("splitterSizes"), QVariant::fromValue< QList<int> >(ui->splitter->sizes()));
        settings.setValue(QLatin1String("windowGeometry"), geometry());
    } else {
        // if mini layout, save only x,y,width
        QRect rect = settings.value(QLatin1String("windowGeometry"), QRect(100,100,350,600)).toRect();
        rect.setWidth(geometry().width());
        settings.setValue(QLatin1String("windowGeometry"), rect);
    }

    if(!m_showHistory)
        settings.setValue(QLatin1String("searchTerms"), QStringList());
    else
        settings.setValue(QLatin1String("searchTerms"), m_searchList);

    if(m_saveDestination)
        settings.setValue(QLatin1String("destination"), ui->pathLine->text());
    else
        settings.setValue(QLatin1String("destination"), QString());

    settings.setValue(QLatin1String("timerState"), m_playerWidget->getTimerState());
    settings.setValue(QLatin1String("muted"), The::audioEngine()->isMuted());
    settings.setValue(QLatin1String("volume"), The::audioEngine()->volume());
    settings.setValue(QLatin1String("guiLayout"), m_guiLayout);
    settings.setValue(QLatin1String("sessionFile"), m_sessionFileName);
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
    QString artist = ui->artistsCB->currentText();
    QString album = ui->albumsCB->currentText();

    for(int i = 0; i < ui->matchList->count(); i++) {
        QString itemArtist = ((MatchItem *)ui->matchList->itemWidget(ui->matchList->item(i)))->playlistItem()->song()->artistName();
        QString itemAlbum  = ((MatchItem *)ui->matchList->itemWidget(ui->matchList->item(i)))->playlistItem()->song()->albumName();

        if( ui->artistsCB->currentIndex() == 0  && ui->albumsCB->currentIndex() == 0) {
            ui->matchList->setRowHidden(i, false);
        } else if( ui->artistsCB->currentIndex() == 0 ) {
            if( itemAlbum == album )
                ui->matchList->setRowHidden(i, false);
            else
                ui->matchList->setRowHidden(i, true);
        } else if( ui->albumsCB->currentIndex() == 0 ) {
            if( itemArtist == artist )
                ui->matchList->setRowHidden(i, false);
            else
                ui->matchList->setRowHidden(i, true);
        } else {
            if( itemArtist == artist && itemAlbum == album   )
                ui->matchList->setRowHidden(i, false);
            else
                ui->matchList->setRowHidden(i, true);
        }
    }

    ui->matchesMessage->setText(trUtf8("%n song(s) found", "", visibleItemsCount()));
}

/*!
  \brief artistChanged : slot for artistsCB
  \return void
*/
void MainWindow::artistChanged()
{
    ui->albumsCB->clear();
    ui->albumsCB->addItem(trUtf8("All Albums"));
    for(int i = 0; i < m_artistsAlbumsContainer.count(); i++) {
        if(m_artistsAlbumsContainer.at(i).first == ui->artistsCB->currentText() ||
            ui->artistsCB->currentIndex() == 0) {
            ui->albumsCB->addItems(m_artistsAlbumsContainer.at(i).second);
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
    for(; m_parallelDownloadsCount < m_maxDownloads; m_parallelDownloadsCount++) {
        // check if m_queue is empty or item already queued
        while(!m_queue.isEmpty() && ((DownloadItem *)m_queue.at(0))->downloadState() != GrooveOff::QueuedState) {
            // discard item
            m_queue.takeFirst();
        }

        if(m_queue.isEmpty())
            return;

        // valid item found
        ((DownloadItem *)m_queue.at(0))->startDownload();
        m_queue.takeFirst();
    }
}

/*!
  \brief addItemToQueue : add an item to the m_queue list
  \param item: item to add
  \return void
*/
void MainWindow::addItemToQueue(DownloadItem* item)
{
    m_queue.append(item);
    unqueue();
}

/*!
  \brief freeDownloadSlot : slot called when a songs download finish
  \return void
*/
void MainWindow::freeDownloadSlot()
{
    m_parallelDownloadsCount--;
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
    for(int i = 0; i < ui->downloadList->count(); i++) {
        GrooveOff::DownloadState state = qobject_cast<DownloadItem *>(ui->downloadList->itemWidget(ui->downloadList->item(i)))->downloadState();
        if(state != GrooveOff::DeletedState) {
            if(id == qobject_cast<DownloadItem *>(ui->downloadList->itemWidget(ui->downloadList->item(i)))->playlistItem()->song()->songID()) {
                if(!m_batchDownload) {
                    QMessageBox::information(this,
                                            trUtf8("Download in progress"),
                                            trUtf8("A file with the same name is already in your download list."),
                                            QMessageBox::Ok);
                }
                return true;
            }
        }
    }
    return false;
}

int MainWindow::visibleItemsCount()
{
    int count = 0;
    for(int i = 0; i < ui->matchList->count(); i++) {
        if(!ui->matchList->isRowHidden(i))
            count++;
    }

    return count;
}

void MainWindow::reloadItemsDownloadButtons()
{
    for(int i = 0; i < ui->matchList->count(); i++) {
        qobject_cast<MatchItem *>(ui->matchList->itemWidget(ui->matchList->item(i)))->setDownloadIcon();
    }
}

void MainWindow::batchDownload()
{
    m_batchDownload = true;
    m_stopBatchDownload = false;
    for(int i = 0; i < ui->matchList->count(); i++) {
        if(m_stopBatchDownload)
            break;
        if(!ui->matchList->item(i)->isHidden())
            downloadRequest(((MatchItem *)ui->matchList->itemWidget(ui->matchList->item(i)))->playlistItem());
    }
    m_batchDownload = false;
    m_stopBatchDownload = true;
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
        m_sessionFileName = fileName;
    }
}

void MainWindow::saveSession()
{
    The::sessionReaderWriter()->write( m_sessionFilePath + m_sessionFileName + QLatin1String(".xml"),
                                       ui->downloadList->playlistItems() );
}

void MainWindow::loadSession()
{
    // disable temporarily AutoScroll
    ui->downloadList->setAutoScroll(false);

    // remove old entries
    ui->downloadList->abortAllDownloads();
    ui->downloadList->removeDownloaded();
    ui->downloadList->removeFailedDeletedAborted();

    QList<PlaylistItemPtr> items = The::sessionReaderWriter()->read(m_sessionFilePath + m_sessionFileName + QLatin1String(".xml"));

    foreach(PlaylistItemPtr item, items) {
        // Decide if show cover arts
        if(m_loadCovers && !QFile::exists(Utility::coversCachePath + item->song()->coverArtFilename())) {
            m_cvrMngr->addItem(item);
        }
        
        addDownloadItem(item);
    }

    ui->downloadList->setAutoScroll(true);
    ui->downloadList->reloadPlaylist();

    // FIXME
    // loadSessions();
}

void MainWindow::loadSessions()
{
    foreach(QAction *action, ActionCollection::instance()->getMenu("sessionsMenu")->actions()) {
        ActionCollection::instance()->getMenu("sessionsMenu")->removeAction(action);
        delete action;
    }

    QDir sessionPath(m_sessionFilePath);
    QStringList sessions = sessionPath.entryList(QStringList() << "*.xml", QDir::Files, QDir::Name);

    foreach(QString session, sessions) {
        QAction *action = new QAction(session.remove(".xml", Qt::CaseInsensitive), ActionCollection::instance()->getMenu("sessionsMenu"));
        if( session == m_sessionFileName )
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
    if(fileName != m_sessionFileName && QFile::exists(m_sessionFilePath + fileName + QLatin1String(".xml"))) {
        // ask if abort current downloads
        saveSession();
        ui->downloadList->abortAllDownloads();
        ui->downloadList->removeDownloaded();
        ui->downloadList->removeFailedDeletedAborted();
        m_sessionFileName = fileName;

        loadSession();
    }
}

void MainWindow::restoreSearch()
{
    m_searchInProgress = false;
    ui->searchButton->setVisible(true);

    ui->spinnerWidget->setVisible(false);
    m_spinner->stop();
}

void MainWindow::changeEvent( QEvent *event )
{
    if( event->type() == QEvent::PaletteChange )
        The::paletteHandler()->setPalette( palette() );
}

void MainWindow::openSessionManager()
{
    SessionManager manager(m_sessionFilePath);
    manager.exec();
    if(!QFile::exists(m_sessionFilePath + m_sessionFileName + QLatin1String(".xml"))) {
        if(The::audioEngine()->state() == Phonon::PlayingState ||
            The::audioEngine()->state() == Phonon::PausedState )
            The::audioEngine()->stop();

        m_sessionFileName = QLatin1String( "default" );
    }

    loadSession();
}

