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


#include "PlayerWidget.h"
#include "ui_PlayerWidget.h"
#include "Utility.h"
#include "AudioEngine.h"
#include "Playlist.h"

#include <QDir>
#include <QTime>
#include <QTimer>
#include <QGraphicsDropShadowEffect>

// Taglib
#include <taglib/fileref.h>
#include <taglib/audioproperties.h>

// this is the player widget, with audio controls
PlayerWidget::PlayerWidget( QWidget *parent )
    : QWidget( parent )
    , ui( new Ui::PlayerWidget )
    , m_timer( new QTimer( this ) )
    , m_playedRemoved( false )
{
    ui->setupUi( this );

    setupShadows();
    setupFonts();
    setupWidgetsSizes();
    setupLabels();
    setupButtonsType();
    setupConnections();

    // Supply with the MediaObject object seekSlider should control
    ui->seekSlider->setMediaObject( The::audioEngine()->mediaObject() );
    
    // change background for track frame
    ui->stackedWidget->setBackgroundRole( QPalette::Midlight );

    // start with all buttons disabled
    ui->previousButton->setButtonEnabled( false );
    ui->playPauseButton->setButtonEnabled( false );
    ui->playPauseButton->setPlaying( false );
    ui->nextButton->setButtonEnabled( false );

    ui->volume->setValue( 50 );
}

PlayerWidget::~PlayerWidget()
{
    The::audioEngine()->stop( Phonon::NoError );
}

void PlayerWidget::setupShadows()
{
    // shadow for the cover label
    m_coverShadow = new QGraphicsDropShadowEffect( this );
    m_coverShadow->setBlurRadius( 15.0 );
    m_coverShadow->setColor( palette().color( QPalette::Shadow ) );
    m_coverShadow->setOffset( 0.0 );
    
    ui->coverLabel->setWindowFlags( Qt::FramelessWindowHint );
    ui->coverLabel->setAttribute( Qt::WA_TranslucentBackground );
    ui->coverLabel->setGraphicsEffect( m_coverShadow );
    
    //Enable shadow
    ui->messageLabel->enableShadow( true );
    ui->titleLabel->enableShadow( true );
    ui->album_authorLabel->enableShadow( true );
    ui->bitrateLabel->enableShadow( true );
    ui->samplerateLabel->enableShadow( true );
    ui->channelsLabel->enableShadow( true );
}

void PlayerWidget::setupFonts()
{
    // set fonts
    ui->titleLabel->setFont( Utility::font( QFont::Bold ) );
    ui->album_authorLabel->setFont( Utility::font( QFont::Bold ) );
    ui->messageLabel->setFont( Utility::font( QFont::Bold,1 ) );
    ui->timeLabel->setFont( Utility::monoFont() );
    ui->elapsedTimeLabel->setFont( Utility::monoFont() );
    ui->bitrateLabel->setFont( Utility::font( QFont::Bold,-3 ) );
    ui->samplerateLabel->setFont( Utility::font( QFont::Bold,-3 ) );
    ui->channelsLabel->setFont( Utility::font( QFont::Bold,-3 ) );
}

void PlayerWidget::setupWidgetsSizes()
{
    ui->timeLabel->setMinimumSize( QSize( 50,0 ) );
    ui->elapsedTimeLabel->setMinimumSize( QSize( 50,0 ) );
    ui->volume->setFixedSize( QSize( 48,48 ) );
    
    ui->seekSlider->setMinimumWidth( 130 );
    
    ui->bitrateLabel->setMinimumWidth( 50 );
    ui->samplerateLabel->setMinimumWidth( 50 );
    ui->channelsLabel->setMinimumWidth( 50 );
}

void PlayerWidget::setupLabels()
{
    ui->timeLabel->setText( QLatin1String( "00:00" ) );
    ui->elapsedTimeLabel->setText( QLatin1String( "00:00" ) );
    
    ui->bitrateLabel->setElideMode( Qt::ElideNone );
    ui->samplerateLabel->setElideMode( Qt::ElideNone );
    ui->channelsLabel->setElideMode( Qt::ElideNone );
    
    ui->bitrateLabel->setToolTip( trUtf8( "Bit Rate" ) );
    ui->samplerateLabel->setToolTip( trUtf8( "Sample Rate" ) );
    
    ui->coverLabel->setScaledContents( true );
}

void PlayerWidget::setupButtonsType()
{
    ui->previousButton->setType( IconButton::Previous );
    ui->playPauseButton->setType( IconButton::PlayPause );
    ui->nextButton->setType( IconButton::Next );
}


void PlayerWidget::setupConnections()
{
    connect( m_timer, SIGNAL(timeout()), SLOT(switchPage()) );
    connect( ui->previousButton, SIGNAL(buttonClicked()), SLOT(playPrevious()) );
    connect( ui->playPauseButton, SIGNAL(buttonClicked()), SLOT(pauseResumePlaying()) );
    connect( ui->nextButton, SIGNAL(buttonClicked()), SLOT(playNext()) );
    connect( ui->timeLabel, SIGNAL(clicked()), SLOT(toggleTimeLabel()) );
    connect( The::audioEngine(), SIGNAL(seeked(qint64,bool)), SLOT(tick(qint64,bool)) );
    connect( The::audioEngine(), SIGNAL(stateChanged(Phonon::State)), SLOT(stateChanged(Phonon::State)) );
    connect( The::audioEngine(), SIGNAL(sourceChanged()), SLOT(sourceChanged()) );
    connect( The::audioEngine(), SIGNAL(removedPlayingTrack()), SLOT(removedPlayingTrack()) );
    connect( The::audioEngine(), SIGNAL(volumeChanged(int)), ui->volume, SLOT(setValue(int)) );
    connect( The::audioEngine(), SIGNAL(muteStateChanged(bool)), SLOT(muteStateChanged(bool)) );
    connect( The::playlist(), SIGNAL(playlistChanged()), SLOT(reloadPreviousNextButtons()) );
    connect( ui->volume, SIGNAL(valueChanged(int)), The::audioEngine(), SLOT(setVolume(int)) );
    connect( ui->volume, SIGNAL(muteToggled(bool)), The::audioEngine(), SLOT(setMuted(bool)) );
}


void PlayerWidget::showElapsedTimerLabel( bool ok )
{
    ui->elapsedTimeLabel->setVisible( ok );
}

void PlayerWidget::stateChanged( Phonon::State state )
{
    switch ( state ) {
    case Phonon::ErrorState:
        setErrorState();
        break;
        
    case Phonon::PlayingState:
        setPlayingState();
        break;
        
    case Phonon::StoppedState:
        setStoppedState();
        break;
        
    case Phonon::PausedState:
        setPausedState();
        break;

    default:
        /* do nothing */
        break;
    }
}

void PlayerWidget::setErrorState()
{
    ui->stackedWidget->setCurrentIndex( 0 );
    ui->playPauseButton->setPlaying( false );
}

void PlayerWidget::setPlayingState()
{
    m_playedRemoved = false;
    ui->stackedWidget->setCurrentIndex( 1 );
    ui->playPauseButton->setButtonEnabled( true );
    ui->playPauseButton->setPlaying( true );
    ui->playPauseButton->setToolTip( trUtf8( "Pause" ) );
}
void PlayerWidget::setStoppedState()
{
    if( m_playedRemoved )
        ui->stackedWidget->setCurrentIndex( 0 );
    else
        ui->stackedWidget->setCurrentIndex( 1 );
    if( m_playedRemoved ) {
        if( !The::playlist()->count() ) {
            ui->playPauseButton->setButtonEnabled( false );
            ui->playPauseButton->setToolTip( "" );
        }
    }
    ui->playPauseButton->setPlaying( false );
    ui->timeLabel->setText( QLatin1String( "00:00" ) );
    ui->elapsedTimeLabel->setText( QLatin1String( "00:00" ) );
}

void PlayerWidget::setPausedState()
{
    ui->stackedWidget->setCurrentIndex( 1 );
    ui->playPauseButton->setPlaying( false );
}


void PlayerWidget::tick( qint64 elapsedTime, bool userSeek )
{
    Q_UNUSED( userSeek )

    if( The::audioEngine()->state() == Phonon::StoppedState )
        return;

    const quint64 remainingTime = The::audioEngine()->remainingTime();

    if( ui->elapsedTimeLabel->isVisible() ) {
        ui->elapsedTimeLabel->setText( QTime( 0, ( elapsedTime / 60000 ) % 60, ( elapsedTime / 1000 ) % 60 ).toString( "mm:ss" ) );
        ui->timeLabel->setText( '-' + QTime( 0, ( remainingTime / 60000 ) % 60, ( remainingTime / 1000 ) % 60 ).toString( "mm:ss" ) );
    } else {
        switch( m_timerState ) {
            case GrooveOff::ElapsedState:
                ui->timeLabel->setText( QTime( 0, ( elapsedTime / 60000 ) % 60, ( elapsedTime / 1000 ) % 60 ).toString( "mm:ss" ) );
                break;
            case GrooveOff::RemainingState:
                ui->timeLabel->setText( '-' + QTime( 0, ( remainingTime / 60000 ) % 60, ( remainingTime / 1000 ) % 60 ).toString( "mm:ss" ) );
                break;
        }
    }
}

void PlayerWidget::pauseResumePlaying()
{
    m_playedRemoved = false;
    
    if( The::audioEngine()->currentTrack() )
        The::audioEngine()->playPause();
    else if( The::playlist()->count() > 0 )
        The::audioEngine()->playItem( The::playlist()->item( 0 ) );
}

void PlayerWidget::toggleTimeLabel()
{
    m_timerState = ( GrooveOff::TimerState )!m_timerState;
}

void PlayerWidget::showMessage( const QString& message )
{
    if( The::audioEngine()->state() == Phonon::PlayingState || The::audioEngine()->state() == Phonon::PausedState ) {
        m_timer->setSingleShot( true );
        m_timer->start( 3000 );
    }
    ui->stackedWidget->setCurrentIndex( 0 );
    ui->messageLabel->setText( message );
}

void PlayerWidget::sourceChanged()
{
    const PlaylistItemPtr &track = The::audioEngine()->currentTrack();
    
    const QString &title = track->song()->songName();
    const QString &artist = track->song()->artistName();
    const QString &album = track->song()->albumName();
    const QString &coverName = track->song()->coverArtFilename();

    ui->titleLabel->setText( title );
    ui->titleLabel->setToolTip( title );

    ui->album_authorLabel->setText( artist + " - " + album );
    ui->album_authorLabel->setToolTip( artist + " - " + album );

    if( !coverName.isEmpty()
        && QFile::exists( Utility::coversCachePath + coverName )
        && coverName != "0" )
        ui->coverLabel->setPixmap( QPixmap( Utility::coversCachePath + coverName ) );
    else
        ui->coverLabel->setPixmap( QIcon::fromTheme( QLatin1String( "media-optical" ),
                                                     QIcon( QLatin1String( ":/resources/media-optical.png" ) )
                                                   ).pixmap( ui->coverLabel->size() ) );

    // Audio Properties
    const TagLib::FileRef trackFileRef( QString( track->path() + track->fileName() ).toLatin1(), 
                             true, 
                             TagLib::AudioProperties::Average 
                           );
    if( trackFileRef.isNull() ) {
        qDebug() << "GrooveOff :: TagLib ::" << track->path() + track->fileName() << "is null";
        return;
    }

    const int bitrate = trackFileRef.audioProperties()->bitrate();
    const int channels =  trackFileRef.audioProperties()->channels();
    const int sampleRate =  trackFileRef.audioProperties()->sampleRate();

    ui->bitrateLabel->setText( QString::number( bitrate ) + QLatin1String( " kb/s" ) );
    ui->samplerateLabel->setText( QString::number( sampleRate ) + QLatin1String( " Hz" ) );
    ui->channelsLabel->setText( channels >= 2 ? QLatin1String( "STEREO" ) : QLatin1String( "MONO" ) );
    
    // refresh previous/next buttons state
    reloadPreviousNextButtons();
}

void PlayerWidget::playNext() const
{
    The::audioEngine()->next();
}

void PlayerWidget::playPrevious() const
{
    The::audioEngine()->previous();
}

void PlayerWidget::reloadPreviousNextButtons()
{
    if( The::playlist()->count() == 0 ) { // no tracks in playlist
        ui->stackedWidget->setCurrentIndex( 0 );
        ui->playPauseButton->setButtonEnabled( false );
    } else {
        ui->playPauseButton->setButtonEnabled( true );
        ui->playPauseButton->setToolTip( trUtf8( "Play" ) );
        if( The::audioEngine()->canGoNext() ) {
            const int currentRow = The::playlist()->row( The::audioEngine()->currentTrack() );
            ui->nextButton->setToolTip( The::playlist()->item( currentRow + 1 )->song()->songName() );
        } else {
            ui->nextButton->setToolTip( QString() );
        }

        if( The::audioEngine()->canGoPrevious() ) {
            const int currentRow = The::playlist()->row( The::audioEngine()->currentTrack() );
            ui->previousButton->setToolTip( The::playlist()->item( currentRow - 1 )->song()->songName() );
        } else {
            ui->previousButton->setToolTip( QString() );
        }

        ui->nextButton->setButtonEnabled( The::audioEngine()->canGoNext() );
        ui->previousButton->setButtonEnabled( The::audioEngine()->canGoPrevious() );
    }
}

void PlayerWidget::removedPlayingTrack()
{
    m_playedRemoved = true;
}

void PlayerWidget::switchPage() const
{
    ui->stackedWidget->setCurrentIndex( 1 );
}

void PlayerWidget::muteStateChanged( bool mute ) const
{
    ui->volume->setMuted( mute );
}

// intercept palette changes to modify shadow color
void PlayerWidget::changeEvent( QEvent* event )
{
    if( event->type() == QEvent::PaletteChange ) {
        m_coverShadow->setColor( palette().color( QPalette::Highlight ) );
    }
    
    QWidget::changeEvent( event );
}


