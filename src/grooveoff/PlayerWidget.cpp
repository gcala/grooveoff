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
#include "TrackTags.h"

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
    , m_updatingSeekSlider( false )
{
    ui->setupUi( this );

    setupShadows();
    setupFonts();
    setupWidgetsSizes();
    setupLabels();
    setupButtonsType();
    setupConnections();

    // the seek slider is driven by the engine, and drives it back when the user
    // moves it: see seekSliderValueChanged() and tick()
    ui->seekSlider->setRange( 0, 0 );
    ui->seekSlider->setEnabled( false );

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
    The::audioEngine()->stop();
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
    
    ui->bitrateLabel->setToolTip( tr( "Bit Rate" ) );
    ui->samplerateLabel->setToolTip( tr( "Sample Rate" ) );
    
    // Not scaledContents: that stretches the picture to the label, and album
    // art is not always square — the covers of a real torrent turned out to be
    // 382 by 336. The pixmap is scaled keeping its shape and centred in what is
    // left over.
    ui->coverLabel->setAlignment( Qt::AlignCenter );
}

void PlayerWidget::setupButtonsType()
{
    ui->previousButton->setType( IconButton::Previous );
    ui->playPauseButton->setType( IconButton::PlayPause );
    ui->nextButton->setType( IconButton::Next );
}


void PlayerWidget::setupConnections()
{
    connect( m_timer, &QTimer::timeout, this, &PlayerWidget::switchPage );
    connect( ui->previousButton, &SvgButton::buttonClicked, this, &PlayerWidget::playPrevious );
    connect( ui->playPauseButton, &SvgButton::buttonClicked, this, &PlayerWidget::pauseResumePlaying );
    connect( ui->nextButton, &SvgButton::buttonClicked, this, &PlayerWidget::playNext );
    connect( ui->timeLabel, &TimerLabel::clicked, this, &PlayerWidget::toggleTimeLabel );
    connect( The::audioEngine(), &AudioEngine::seeked, this, &PlayerWidget::tick );
    connect( The::audioEngine(), &AudioEngine::stateChanged, this, &PlayerWidget::stateChanged );
    connect( The::audioEngine(), &AudioEngine::sourceChanged, this, &PlayerWidget::sourceChanged );
    connect( The::audioEngine(), &AudioEngine::removedPlayingTrack, this, &PlayerWidget::removedPlayingTrack );
    connect( The::audioEngine(), &AudioEngine::volumeChanged, ui->volume, &VolumeDial::setValue );
    connect( The::audioEngine(), &AudioEngine::muteStateChanged, this, &PlayerWidget::muteStateChanged );
    connect( The::playlist(), &Playlist::playlistChanged, this, &PlayerWidget::reloadPreviousNextButtons );
    connect( ui->volume, &VolumeDial::valueChanged, The::audioEngine(), &AudioEngine::setVolume );
    connect( ui->volume, &VolumeDial::muteToggled, The::audioEngine(), &AudioEngine::setMuted );

    // seek slider, which Phonon used to drive on its own
    connect( The::audioEngine(), &AudioEngine::trackLengthChanged, this, &PlayerWidget::trackLengthChanged );
    connect( The::audioEngine(), &AudioEngine::seekableChanged, ui->seekSlider, &QSlider::setEnabled );
    connect( ui->seekSlider, &QSlider::valueChanged, this, &PlayerWidget::seekSliderValueChanged );
}

/*!
  \brief trackLengthChanged: resize the seek slider to the new track
*/
void PlayerWidget::trackLengthChanged( qint64 milliseconds )
{
    m_updatingSeekSlider = true;
    ui->seekSlider->setRange( 0, milliseconds );
    m_updatingSeekSlider = false;
}

/*!
  \brief seekSliderValueChanged: seek when the change comes from the user

  valueChanged() covers dragging, clicking on the groove and keyboard alike;
  the flag filters out the updates we make ourselves while playing.
*/
void PlayerWidget::seekSliderValueChanged( int value )
{
    if( m_updatingSeekSlider )
        return;

    The::audioEngine()->seek( (qint64)value );
}


void PlayerWidget::showElapsedTimerLabel( bool ok )
{
    ui->elapsedTimeLabel->setVisible( ok );
}

void PlayerWidget::stateChanged( GrooveOff::PlayerState state )
{
    switch ( state ) {
    case GrooveOff::PlayerState::Error:
        setErrorState();
        break;
        
    case GrooveOff::PlayerState::Buffering:
        setBufferingState();
        break;

    case GrooveOff::PlayerState::Playing:
        setPlayingState();
        break;
        
    case GrooveOff::PlayerState::Stopped:
        setStoppedState();
        break;
        
    case GrooveOff::PlayerState::Paused:
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

/*!
  \brief setBufferingState: waiting is a state of its own, and it has to show

  Without this the widget kept whatever was on it, which after adding a torrent
  was the torrent's name — so pressing play on a track left the name of its
  album on the status bar and a Play button that looked like nothing had
  happened. Filling a buffer is the one moment when there is genuinely nothing
  else to report, and it is exactly then that saying nothing reads as broken.
*/
void PlayerWidget::setBufferingState()
{
    const PlaylistItemPtr track = The::audioEngine()->currentTrack();

    showMessage( track && track->track()
                 ? tr( "Buffering %1…" ).arg( track->displayTitle() )
                 : tr( "Buffering…" ) );

    ui->playPauseButton->setButtonEnabled( true );
    ui->playPauseButton->setPlaying( true );
    ui->playPauseButton->setToolTip( tr( "Pause" ) );
}

void PlayerWidget::setPlayingState()
{
    m_playedRemoved = false;
    ui->stackedWidget->setCurrentIndex( 1 );
    ui->playPauseButton->setButtonEnabled( true );
    ui->playPauseButton->setPlaying( true );
    ui->playPauseButton->setToolTip( tr( "Pause" ) );
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
    if( The::audioEngine()->state() == GrooveOff::PlayerState::Stopped )
        return;

    if( !userSeek ) {
        m_updatingSeekSlider = true;
        ui->seekSlider->setValue( elapsedTime );
        m_updatingSeekSlider = false;
    }

    showTimes( elapsedTime, The::audioEngine()->remainingTime() );
}

/*!
  \brief showTimes: the two clocks, wherever the numbers came from

  Told apart from tick() so that the track restored at start up can use it:
  there the numbers do not come from the player, which has nothing loaded, but
  from what was saved and what TagLib says the file is worth.
*/
void PlayerWidget::showTimes( qint64 elapsed, qint64 remaining )
{
    const QString elapsedText = QTime( 0, ( elapsed / 60000 ) % 60, ( elapsed / 1000 ) % 60 ).toString( "mm:ss" );
    const QString remainingText = '-' + QTime( 0, ( remaining / 60000 ) % 60, ( remaining / 1000 ) % 60 ).toString( "mm:ss" );

    // Hidden rather than visible, and the difference is not pedantry: this also
    // runs from the constructor, before the window has been shown, and there
    // isVisible() is false for every widget in it whatever it was told. The bar
    // then filled in one clock and left the other at zero.
    if( !ui->elapsedTimeLabel->isHidden() ) {
        ui->elapsedTimeLabel->setText( elapsedText );
        ui->timeLabel->setText( remainingText );
        return;
    }

    switch( m_timerState ) {
        case GrooveOff::ElapsedState:
            ui->timeLabel->setText( elapsedText );
            break;
        case GrooveOff::RemainingState:
            ui->timeLabel->setText( remainingText );
            break;
    }
}

void PlayerWidget::pauseResumePlaying()
{
    m_playedRemoved = false;
    
    // What play means when nothing is going on is the engine's to answer — the
    // track left over from last time, then the queue, then the playlist on
    // screen. Reaching past it for the first row of the queue is what made a
    // restart start the wrong track: the button walked straight over the
    // resume point that play() exists to use.
    if( The::audioEngine()->currentTrack() )
        The::audioEngine()->playPause();
    else
        The::audioEngine()->play();
}

void PlayerWidget::toggleTimeLabel()
{
    m_timerState = ( GrooveOff::TimerState )!m_timerState;
}

void PlayerWidget::showMessage( const QString& message )
{
    // The message hides the player, so it goes away again by itself — but only
    // when there is a player underneath to go back to. Having a track is what
    // that means, and not being in the middle of playing one: the track
    // restored at start up sits there stopped, and the message that lands on
    // top of it — the name of the torrent whose file list has just come back —
    // used to stay for the rest of the session, hiding everything the bar had
    // just been given. A message with nothing underneath is still all there is
    // to look at, and still stays.
    if( The::audioEngine()->currentTrack() ) {
        m_timer->setSingleShot( true );
        m_timer->start( 3000 );
    }
    ui->stackedWidget->setCurrentIndex( 0 );
    ui->messageLabel->setText( message );
}

/*!
  \brief sourceChanged: another track, so another thing to listen to

  The head of a file can arrive well after the music has started, and until it
  does there are no tags to read: the bar showed the file name, track number
  and all, beside a row that already had the title. So what the track says
  about itself is not read once here — the widget subscribes to it.
*/
void PlayerWidget::sourceChanged()
{
    const PlaylistItemPtr &track = The::audioEngine()->currentTrack();
    if( !track )
        return;

    disconnect( m_trackConnection );
    m_trackConnection = connect( track.data(), &PlaylistItem::refreshNeeded,
                                 this, &PlayerWidget::showTrackInfo );

    // There is a track to look at, so look at it rather than at whatever
    // message happened to be there. It matters at start up, where the track
    // restored from the last session arrives here without anything playing:
    // the page is only ever switched to by the playing and paused states, so
    // the bar stayed on the message and the mini player — which has no track
    // list to fall back on — showed nothing about what pressing play would do.
    ui->stackedWidget->setCurrentIndex( 1 );

    showTrackInfo();
}

void PlayerWidget::showTrackInfo()
{
    const PlaylistItemPtr track = The::audioEngine()->currentTrack();
    if( !track )
        return;

    const QString &title = track->displayTitle();
    const QString &subtitle = track->displaySubtitle();

    ui->titleLabel->setText( title );
    ui->titleLabel->setToolTip( title );

    ui->album_authorLabel->setText( subtitle );
    ui->album_authorLabel->setToolTip( subtitle );

    loadCover();

    // Nothing has arrived of this file yet, so there is nothing to measure and
    // asking would not be free: on a sparse mp3 whose first bytes are missing
    // the mpeg parser scans the whole file for a sync word that is not there,
    // which is a second of frozen window. Four bytes settle it. It matters now
    // that the bar is filled in at start up, when the track being shown may be
    // one whose torrent is still asleep.
    if( !TrackTags::hasHead( track->localPath() ) ) {
        reloadPreviousNextButtons();
        return;
    }

    // Read off whatever is already on disk, and cheaply: Fast rather than
    // Average, because for a vbr mp3 Average means scanning the whole file, and
    // scanning one made mostly of holes spends the time to arrive at a wrong
    // answer. Flac and the rest answer from the header either way.
    //
    // encodeName rather than toLatin1: a path this application did not choose
    // and cannot rename, out of a torrent somebody else made, is the last place
    // to assume the alphabet.
    const TagLib::FileRef trackFileRef( QFile::encodeName( track->localPath() ).constData(),
                             true,
                             TagLib::AudioProperties::Fast
                           );
    if( trackFileRef.isNull() || !trackFileRef.audioProperties() ) {
        qDebug() << "GrooveOff :: TagLib ::" << track->localPath() << "has no readable properties yet";
        reloadPreviousNextButtons();
        return;
    }

    const int bitrate = trackFileRef.audioProperties()->bitrate();
    const int channels =  trackFileRef.audioProperties()->channels();
    const int sampleRate =  trackFileRef.audioProperties()->sampleRate();

    ui->bitrateLabel->setText( QString::number( bitrate ) + QLatin1String( " kb/s" ) );
    ui->samplerateLabel->setText( QString::number( sampleRate ) + QLatin1String( " Hz" ) );
    ui->channelsLabel->setText( channels >= 2 ? QLatin1String( "STEREO" ) : QLatin1String( "MONO" ) );

    // With nothing loaded the player has neither a length nor a position to
    // report, so the bar would open with both clocks at zero and the slider at
    // the far left over a track that was left halfway through. TagLib has just
    // answered, so the length costs nothing more than it already did, and the
    // position is the one being kept for the resume. Both are replaced by the
    // player's own the moment it starts, which is also what corrects a length
    // a file still full of holes guessed wrong.
    const qint64 length = trackFileRef.audioProperties()->lengthInMilliseconds();

    if( The::audioEngine()->isStopped() && length > 0 ) {
        const qint64 elapsed = qMin( The::audioEngine()->currentTime(), length );

        trackLengthChanged( length );

        m_updatingSeekSlider = true;
        ui->seekSlider->setValue( elapsed );
        m_updatingSeekSlider = false;

        showTimes( elapsed, length - elapsed );
    }


    // refresh previous/next buttons state
    reloadPreviousNextButtons();
}

/*!
  \brief loadCover: the same picture the row shows, read the same way

  It comes out of the file itself, because the cdn the covers used to come from
  closed with the rest of Grooveshark and there is no other source that would
  not mean telling a stranger what is being listened to. Before the head of the
  file is in the cache there is nothing to read, so the generic disc stands in —
  and it is also what a file with no picture frame keeps for good.
*/
void PlayerWidget::loadCover()
{
    const PlaylistItemPtr track = The::audioEngine()->currentTrack();

    const QImage cover = track ? TrackTags::cover( track->localPath() ) : QImage();

    if( !cover.isNull() ) {
        ui->coverLabel->setPixmap( QPixmap::fromImage( cover ).scaled( ui->coverLabel->size(),
                                                                       Qt::KeepAspectRatio,
                                                                       Qt::SmoothTransformation ) );
        return;
    }

    ui->coverLabel->setPixmap( QIcon::fromTheme( QLatin1String( "media-optical" ),
                                                 QIcon( QLatin1String( ":/resources/media-optical.png" ) )
                                               ).pixmap( ui->coverLabel->size() ) );
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
        ui->playPauseButton->setToolTip( tr( "Play" ) );
        if( The::audioEngine()->canGoNext() ) {
            const int currentRow = The::playlist()->row( The::audioEngine()->currentTrack() );
            ui->nextButton->setToolTip( The::playlist()->item( currentRow + 1 )->displayTitle() );
        } else {
            ui->nextButton->setToolTip( QString() );
        }

        if( The::audioEngine()->canGoPrevious() ) {
            const int currentRow = The::playlist()->row( The::audioEngine()->currentTrack() );
            ui->previousButton->setToolTip( The::playlist()->item( currentRow - 1 )->displayTitle() );
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


