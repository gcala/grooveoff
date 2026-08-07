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


#include "AudioEngine.h"
#include "Playlist.h"
#include "Settings.h"
#include "../torrent/CacheManager.h"
#include "../torrent/StreamServer.h"
#include "../torrent/TorrentSession.h"

#include <QAudio>
#include <QAudioDevice>
#include <QAudioOutput>
#include <QDir>
#include <QDebug>
#include <QMediaDevices>
#include <QSettings>
#include <QThreadPool>

namespace The {
    static AudioEngine* s_AudioEngine_instance = 0;

    AudioEngine* audioEngine()
    {
        if( !s_AudioEngine_instance )
            s_AudioEngine_instance = new AudioEngine();

        return s_AudioEngine_instance;
    }
}

AudioEngine::AudioEngine()
    : m_state( GrooveOff::PlayerState::Stopped )
    , m_volume( 0 )
{
    setupConnections();

    setVolume( 50 );
}

AudioEngine::~AudioEngine()
{
}

/*!
  \brief player: the QMediaPlayer, built the first time anybody needs to hear
         something

  Building it is not cheap and the cost is not ours: it asks the multimedia
  backend what there is to play on, which means a round trip to the sound
  server. On the machine this was found on that answer took **2.8 seconds** —
  more than two thirds of the whole start up — and it was being waited for in
  the constructor of PlayerWidget, which is built in the initialiser list of
  MainWindow. So the window did not appear until the sound system had said what
  devices exist, which is a thing it has no reason to know before somebody
  presses play.

  Everything that only wants to connect to a signal, ask what state we are in
  or set the volume goes on working with no player at all, or the laziness
  would be undone by the first of them.

  By the time this normally runs the wait has already been had, on another
  thread: see prepareOutput(). What is left here is the fallback, and the two
  lines it costs.
*/
QMediaPlayer *AudioEngine::player()
{
    if( m_mediaPlayer )
        return m_mediaPlayer;

    m_mediaPlayer = new QMediaPlayer( this );

    m_audioOutput = new QAudioOutput( this );
    m_mediaPlayer->setAudioOutput( m_audioOutput );

    connectPlayer();

    // What was asked for while there was nobody to ask.
    m_audioOutput->setMuted( m_muted );
    setVolume( m_volume );

    return m_mediaPlayer;
}

/*!
  \brief prepareOutput: pays for the sound system on a thread nobody is watching

  What costs is not building the player, it is the **first touch of the
  multimedia backend**, whoever makes it: once the backend is up, a player and
  its output are built in no measurable time at all. Asking for the list of
  audio outputs is that first touch, and it can be made from any thread.

  Which is the whole point of doing it here. Building the player in the window's
  own thread half a second after it appeared moved the wait out of the start up
  and straight into the moment somebody clicks a menu: the window was drawn and
  did not answer, because the loop that delivers the clicks was inside the
  round trip to the sound server. A window that ignores the mouse for three
  seconds is worse than one that takes three seconds to appear — it looks
  broken rather than slow.

  The thread is very likely gone before anything plays, and that is fine: what
  it leaves behind is the backend, which belongs to nobody. Playing a file
  through a player built afterwards was measured end to end, thread already
  dead, and it plays.

  Nothing waits for it either. A play that arrives while the backend is still
  coming up blocks exactly as long as it used to, and no longer.
*/
void AudioEngine::prepareOutput()
{
    if( m_mediaPlayer )
        return;

    QThreadPool::globalInstance()->start( [] { ( void )QMediaDevices::audioOutputs(); } );
}

void AudioEngine::connectPlayer()
{
    connect( m_mediaPlayer, &QMediaPlayer::positionChanged, this, &AudioEngine::onPositionChanged );
    connect( m_mediaPlayer, &QMediaPlayer::mediaStatusChanged, this, &AudioEngine::onMediaStatusChanged );
    connect( m_mediaPlayer, &QMediaPlayer::sourceChanged, this, &AudioEngine::onSourceChanged );
    connect( m_mediaPlayer, &QMediaPlayer::seekableChanged, this, &AudioEngine::onSeekableChanged );
    connect( m_mediaPlayer, &QMediaPlayer::durationChanged, this, &AudioEngine::onTrackLengthChanged );

    // the player state is derived from these three
    connect( m_mediaPlayer, &QMediaPlayer::playbackStateChanged, this, &AudioEngine::updateState );
    connect( m_mediaPlayer, &QMediaPlayer::mediaStatusChanged, this, &AudioEngine::updateState );
    connect( m_mediaPlayer, &QMediaPlayer::errorOccurred, this, &AudioEngine::updateState );

    connect( m_audioOutput, &QAudioOutput::volumeChanged, this, &AudioEngine::onVolumeChanged );
    connect( m_audioOutput, &QAudioOutput::mutedChanged, this, &AudioEngine::onMutedChanged );
}

void AudioEngine::setupConnections()
{
    // A track asked for while its torrent was asleep waits here for its file
    // list to arrive.
    connect( The::torrentSession(), &TorrentSession::metadataReady, this,
             [this]( const QString &infoHash, const QString &, const QList<TrackInfoPtr> & ) {
        // The torrent may have been woken for the next track rather than for
        // this one: then there is nothing to play, only something to take.
        if( m_prefetchingTrack && m_prefetchingTrack->track()
            && m_prefetchingTrack->track()->infoHash() == infoHash ) {
            The::streamServer()->prefetch( infoHash, m_prefetchingTrack->track()->fileIndex() );
        }

        if( !m_pendingTrack || !m_pendingTrack->track() || m_pendingTrack->track()->infoHash() != infoHash )
            return;

        const PlaylistItemPtr track = m_pendingTrack;
        m_pendingTrack.clear();

        if( openStream( track ) )
            play();
    } );

    // And here for its opening buffer, which is the second half of the same
    // wait: knowing the file list is not yet being able to read the file.
    connect( The::streamServer(), &StreamServer::trackPlayable, this,
             [this]( const QString &infoHash, int fileIndex ) {
        if( !m_pendingTrack || !m_pendingTrack->track() )
            return;

        const TrackInfoPtr &info = m_pendingTrack->track();
        if( info->infoHash() != infoHash || info->fileIndex() != fileIndex )
            return;

        const PlaylistItemPtr track = m_pendingTrack;
        m_pendingTrack.clear();

        if( openStream( track ) )
            play();
    } );

    // The buffer is checked off the statistics as well as off the position: a
    // player we have paused reports no position at all, so this is the only
    // thing that can tell it to go again.
    connect( The::torrentSession(), &TorrentSession::statsUpdated, this,
             [this]( const QString &, int, int, qint64, qint64 ) { checkBuffer(); } );

    connect( The::torrentSession(), &TorrentSession::torrentError, this,
             [this]( const QString &infoHash, const QString &message ) {
        if( !m_pendingTrack || !m_pendingTrack->track() || m_pendingTrack->track()->infoHash() != infoHash )
            return;

        qDebug() << "GrooveOff ::" << "cannot wake" << infoHash << message;
        m_pendingTrack.clear();
        setState( GrooveOff::PlayerState::Error );
    } );
}

void AudioEngine::onPositionChanged( qint64 time )
{
    m_lastPosition = time;

    emit seeked( time, false );

    checkBuffer();
    maybePrefetchNext();
}

/*!
  \brief checkBuffer: waiting is a thing a player does, dying is not

  A torrent slower than the bitrate of what it holds will run the player out of
  bytes, and there is nothing to be done about that: it is the structural limit
  of the whole idea. What can be done is to meet it properly. Left alone,
  ffmpeg reads into a piece that has not arrived, waits, gives up, and reports
  a stream it could not demux — so a slow swarm looks exactly like a corrupt
  file, and the track ends up marked as unavailable when nothing is wrong with
  it.

  So the player is paused before it gets there and started again when the file
  is far enough ahead. How far is read off the same setting as everything else
  about buffering, and the pause is ours: pressing play while we are holding it
  hands control back, because somebody who insists is entitled to whatever
  comes out.
*/
/*!
  \brief reachedTheEnd: whether the track got where it was going

  The last frames of a stream are where a decoder is likeliest to complain: it
  reads a little past what is there, finds a fragment of a frame and says the
  data are invalid. That is a stream ending, not a track failing — and calling
  it a failure marked a track as unavailable the moment it finished playing,
  which is the exact opposite of what had just happened.

  A whole second of slack, because where a decoder gives up is not exactly
  where the music stops.
*/
bool AudioEngine::reachedTheEnd() const
{
    const qint64 duration = m_mediaPlayer ? m_mediaPlayer->duration() : 0;

    return duration > 0 && m_lastPosition >= duration - 1000;
}

void AudioEngine::checkBuffer()
{
    // The statistics of a torrent arrive once a second whatever we are doing,
    // and a track restored at start up is current without ever having been
    // played: this runs with nothing to hold back and nothing to hold it with.
    if( !m_mediaPlayer )
        return;

    if( !m_currentTrack || !m_currentTrack->track() )
        return;

    const TrackInfoPtr &info = m_currentTrack->track();
    const qint64 duration = m_mediaPlayer->duration();
    const qint64 size     = info->fileSize();

    // Before the player knows how long the track is there is no way to turn
    // bytes into seconds, and guessing would hold back a track that is fine.
    if( duration <= 0 || size <= 0 )
        return;

    const qint64 buffered = The::torrentSession()->bufferedBytes( info->infoHash(), info->fileIndex() );

    // The whole file is there, so there is nothing left to wait for — and that
    // includes a wait already under way. Returning here without letting the
    // player go was a track held at buffering for ever, with a bar reading a
    // perfectly truthful hundred per cent: the one state in which nothing else
    // was ever going to come along and release it.
    if( buffered >= size ) {
        if( m_waitingForBuffer ) {
            m_waitingForBuffer = false;
            m_mediaPlayer->play();
        }

        return;
    }

    // Bytes to milliseconds through the one ratio that is certainly right for
    // this file: its own size against its own duration.
    const qint64 playableMs = buffered * duration / size;
    const qint64 aheadMs    = playableMs - m_mediaPlayer->position();

    QSettings settings;
    const qint64 wantedMs = qMax( 1, settings.value( Settings::bufferSeconds,
                                                     Settings::bufferSecondsDefault ).toInt() ) * 1000;

    // Stop with less in hand than was asked for. The margin is measured against
    // the playing position, while ffmpeg reads well ahead of it, so by the time
    // this fires the demuxer is already closer to the edge than the number says
    // — which is why the number is the whole setting and not half of it.
    if( !m_waitingForBuffer && aheadMs < wantedMs ) {
        m_waitingForBuffer = true;
        m_mediaPlayer->pause();
        setState( GrooveOff::PlayerState::Buffering );
        return;
    }

    // And start again only with twice it, never at the line that stopped it:
    // resuming with exactly enough means stopping again a moment later, and a
    // track that stutters every few seconds is worse than one that waits once
    // and then plays.
    if( m_waitingForBuffer && aheadMs >= wantedMs * 2 ) {
        m_waitingForBuffer = false;
        m_mediaPlayer->play();
    }
}

void AudioEngine::maybePrefetchNext()
{
    if( !isPlaying() || !m_oldTrack )
        return;

    // Past the middle, and not before: a listener who skips in the first half
    // is a listener who was not going to hear the next track anyway.
    const qint64 total = m_mediaPlayer->duration();
    if( total <= 0 || m_mediaPlayer->position() * 2 < total )
        return;

    if( !canGoNext() )
        return;

    const PlaylistItemPtr next = The::playlist()->item( The::playlist()->row( m_oldTrack ) + 1 );
    if( !next || !next->track() || next == m_prefetchingTrack )
        return;

    QSettings settings;
    if( !settings.value( Settings::downloadAhead, Settings::downloadAheadDefault ).toBool() )
        return;

    // Prefetched bytes are a guess, and a cache near its ceiling does not pay
    // for guesses: the prefetch is dropped rather than compensated with an
    // eviction, which would throw away something that was actually heard.
    if( !The::cacheManager()->hasRoomForPrefetch() )
        return;

    const TrackInfoPtr &info = next->track();
    m_prefetchingTrack = next;
    The::cacheManager()->setPrefetchedTorrent( info->infoHash() );

    // A sleeping torrent is woken here rather than at the change of track: the
    // handshake and the dht lookup are the slow part, and they happen while
    // the current track is still playing.
    if( !The::torrentSession()->hasMetadata( info->infoHash() ) ) {
        The::torrentSession()->reopen( info->infoHash(), info->magnetUri() );
        return;
    }

    The::streamServer()->prefetch( info->infoHash(), info->fileIndex() );
}

void AudioEngine::onSourceChanged()
{
    emit sourceChanged();
}

void AudioEngine::onMediaStatusChanged( QMediaPlayer::MediaStatus status )
{
    // Loaded, or loaded and buffered: either is late enough for a position to
    // survive, and they arrive in either order depending on how fast the
    // stream opens.
    if( status == QMediaPlayer::LoadedMedia || status == QMediaPlayer::BufferedMedia )
        applyResumePosition();

    if( status == QMediaPlayer::EndOfMedia )
        onFinished();
}

void AudioEngine::onFinished()
{
    if( m_currentTrack )
        m_currentTrack->setState( GrooveOff::PlayerState::Stopped );
    next();
}

/*!
  \brief updateState: translates the backend state into the application one.

  QtMultimedia splits what Phonon used to report as a single state across
  playbackState(), mediaStatus() and error(), so all three are folded back here.
*/
void AudioEngine::updateState()
{
    GrooveOff::PlayerState newState;

    const QMediaPlayer::PlaybackState playback = m_mediaPlayer->playbackState();
    const QMediaPlayer::MediaStatus   status   = m_mediaPlayer->mediaStatus();

    if( m_waitingForBuffer ) {
        // The player is paused because we paused it. That is buffering, and
        // calling it a pause would put a Play button in front of somebody who
        // never pressed anything.
        newState = GrooveOff::PlayerState::Buffering;
    } else if( status == QMediaPlayer::StalledMedia ) {
        // Running but with nothing to run on. This has to come before the
        // playing test and not after it: while a stream starves, the playback
        // state still says Playing — it is the media status that knows — and
        // asking in the other order gives a spinning animation and a pause
        // button over a track that has gone silent.
        newState = GrooveOff::PlayerState::Buffering;
    } else if( playback == QMediaPlayer::PlayingState ) {
        // And playing beats the error below, which is why that is last.
        newState = GrooveOff::PlayerState::Playing;
    } else if( status == QMediaPlayer::LoadingMedia
               || status == QMediaPlayer::BufferingMedia ) {
        // Opening the stream, which takes about a tenth of a second and used to
        // be reported as stopped. Nothing was stopped: the track was asked for
        // and is on its way. It showed as a play button appearing on the row
        // for a moment at the start of every track, and — once the sidebar
        // started marking the playlist the music comes from — as that mark
        // blinking off and on again at every change of track.
        newState = GrooveOff::PlayerState::Buffering;
    } else if( m_mediaPlayer->error() != QMediaPlayer::NoError
               && playback == QMediaPlayer::StoppedState
               && status != QMediaPlayer::EndOfMedia
               && !reachedTheEnd() ) {
        // QMediaPlayer keeps its last error until a new source is set, so it
        // says nothing about now. Streaming makes that difference visible in
        // the worst way: ffmpeg reports a failure whenever it runs out of bytes
        // mid packet, recovers on its own the moment more arrive, and the error
        // stays behind — the row said the track was not available while the
        // track was audibly playing, and went on saying it after the end.
        //
        // What an error means is that the player has given up, so that is what
        // is asked: stopped, and not stopped because it reached the end.
        qDebug() << "GrooveOff ::" << "playback failed:" << m_mediaPlayer->errorString();
        newState = GrooveOff::PlayerState::Error;
    } else {
        switch( playback ) {
            case QMediaPlayer::PausedState:
                newState = GrooveOff::PlayerState::Paused;
                break;
            case QMediaPlayer::StoppedState:
            default:
                newState = GrooveOff::PlayerState::Stopped;
                break;
        }
    }

    setState( newState );
}

void AudioEngine::setState( GrooveOff::PlayerState state )
{
    if( m_state == state )
        return;

    m_state = state;

    if( m_currentTrack )
        m_currentTrack->setState( state );

    emit stateChanged( state );
}

void AudioEngine::setResumePoint( const PlaylistItemPtr &track, qint64 positionMs )
{
    m_resumeTrack = track;
    m_resumePosition = positionMs;

    // It becomes the current track without being played, which is what
    // "restored, paused" is supposed to look like: the player bar has a name,
    // an album and a cover from the moment the window opens instead of the
    // blank it used to show until somebody pressed something. Nothing is
    // loaded, so the state stays Stopped and no mark appears on the sidebar.
    m_currentTrack = track;
    m_oldTrack = track;

    emit sourceChanged();
}

/*!
  \brief applyResumePosition: puts the resumed track back where it was left

  The position was written at every close and read by nobody, so what came back
  was the right track from its beginning — which is the half of "carry on where
  I left off" that is easy not to notice missing, because the track is right.

  When to ask is the whole difficulty, and it cost a run to find out. The
  player says it can seek while it is still LoadingMedia, and a position set
  then is thrown away when the loading finishes: the track starts from the
  beginning with nothing anywhere saying why. So the media status is what is
  waited for, not seekability — which is asked as well, because the two become
  true in either order.
*/
void AudioEngine::applyResumePosition()
{
    if( m_resumePosition <= 0 || !m_mediaPlayer->isSeekable() )
        return;

    const qint64 position = m_resumePosition;
    m_resumePosition = 0;

    m_mediaPlayer->setPosition( position );

    // Not a user seek: nobody dragged anything, and the slider is being told
    // where the track already was rather than being obeyed.
    emit seeked( position, false );
}

void AudioEngine::playItem( const PlaylistItemPtr &track)
{
    // Somebody chose, and what they chose is not what was left behind: the
    // saved position belongs to another track and would drop this one into its
    // middle for no reason anybody could see.
    if( track != m_resumeTrack ) {
        m_resumeTrack.clear();
        m_resumePosition = 0;
    }

    // this was added to handle triggers from playlist menu
    if(m_currentTrack == track) {
        m_currentTrack->isPlaying() ? pause() : play();
        return;
    }

    startTrack( track );
}

/*!
  \brief startTrack: everything playItem() does once it knows it has to

  Told apart from playItem() because of the track restored at start up. That
  one is already the current track — it has to be, or the player bar would open
  blank and the play button would have nothing to resume — so playItem() takes
  its shortcut and hands the question back to play(), which would hand it
  straight back: two functions politely calling each other until the stack ran
  out. This is the way out of that circle, and play() uses it directly.
*/
void AudioEngine::startTrack( const PlaylistItemPtr &track )
{
    emit trackRequested( track );

    if( m_currentTrack && m_currentTrack != track )
        m_currentTrack->setState( GrooveOff::PlayerState::Stopped );

    m_currentTrack = track;
    m_oldTrack = track;
    m_pendingTrack.clear();
    m_waitingForBuffer = false;
    m_lastPosition = 0;

    // Whatever was being brought up for the previous choice is dropped here.
    // Skipping this is how ten skips become ten downloads and a cache full of
    // music nobody listened to.
    The::streamServer()->cancelPreparation();
    The::streamServer()->cancelPrefetch();
    m_prefetchingTrack.clear();
    The::cacheManager()->setPrefetchedTorrent( QString() );

    // What is being listened to is what the eviction may never take, so the
    // cache is told before anything is asked for rather than after.
    The::cacheManager()->setPlayingTorrent( track && track->track() ? track->track()->infoHash()
                                                                   : QString() );

    if( track && track->track()
        && !The::torrentSession()->hasMetadata( track->track()->infoHash() ) ) {
        wakeTorrentFor( track );
        return;
    }

    if( openStream( track ) )
        play();
}

/*!
  \brief openStream: points the player at the local server

  A url rather than a path, which is the whole difference: a path can only be
  read where the bytes already are, a url is answered as they arrive.
*/
bool AudioEngine::openStream( const PlaylistItemPtr &track )
{
    if( !track || !track->track() )
        return false;

    const TrackInfoPtr &info = track->track();

    if( !The::torrentSession()->hasMetadata( info->infoHash() ) )
        return false;

    // Handing the url over too early is not a wait, it is a failure: ffmpeg
    // reads the head of the file and probes its tail before it will play
    // anything, and with neither there it gives up with an i/o error.
    if( !The::streamServer()->isPlayable( info->infoHash(), info->fileIndex() ) ) {
        m_pendingTrack = track;
        setState( GrooveOff::PlayerState::Buffering );
        The::streamServer()->prepare( info->infoHash(), info->fileIndex() );
        return false;
    }

    const QUrl url = The::streamServer()->urlFor( info );
    if( url.isEmpty() ) {
        qDebug() << "GrooveOff ::" << "the stream server is not running";
        setState( GrooveOff::PlayerState::Error );
        return false;
    }

    player()->setSource( url );
    return true;
}

/*!
  \brief wakeTorrentFor: lazy reactivation, from here because every play goes through here

  Most rows are dormant right after a start up: their torrent has not been
  handed to the session, on purpose, so that opening the application does not
  announce twenty torrents to as many trackers. Waking one up is asynchronous,
  so the intent is kept and playback starts when the file list comes back.
*/
void AudioEngine::wakeTorrentFor( const PlaylistItemPtr &track )
{
    if( !track || !track->track() )
        return;

    const TrackInfoPtr &info = track->track();

    m_pendingTrack = track;
    setState( GrooveOff::PlayerState::Buffering );

    QString error;
    if( The::torrentSession()->reopen( info->infoHash(), info->magnetUri(), &error ) ) {
        // The metadata may already be in hand, in which case no alert follows
        // and the opening buffer is the only thing left to wait for.
        if( The::torrentSession()->hasMetadata( info->infoHash() ) ) {
            m_pendingTrack.clear();
            if( openStream( track ) )
                play();
        }
        return;
    }

    qDebug() << "GrooveOff ::" << "cannot reopen" << info->infoHash() << error;
    m_pendingTrack.clear();
    setState( GrooveOff::PlayerState::Error );
}

void AudioEngine::playPause()
{
    isPlaying() ? pause() : play();
}

void AudioEngine::pause()
{
    // An explicit pause ends our own wait: from here on the player is stopped
    // because somebody stopped it, and starting it again is their call.
    m_waitingForBuffer = false;

    if( m_mediaPlayer )
        m_mediaPlayer->pause();
}

void AudioEngine::play()
{
    // A track is already on its way: pressing play again does not bring it any
    // sooner, and falling through would start the first row of the list, which
    // is not what anybody asked for.
    if( m_pendingTrack )
        return;

    // No player is no source, and asking the question that way keeps the whole
    // point of the laziness: pressing play with nothing anywhere to play must
    // not build the sound machinery to then say there is nothing to play.
    if( !m_mediaPlayer || m_mediaPlayer->source().isEmpty() ) {
        // What was being listened to when the window last closed. Only while
        // nothing is loaded: after that this is ordinary playback and the queue
        // is what decides. Cleared here rather than in playItem(), which is
        // about to be handed it and would drop the position with it.
        if( m_resumeTrack ) {
            const PlaylistItemPtr resume = m_resumeTrack;
            startTrack( resume );
            m_resumeTrack.clear();
            return;
        }

        if( The::playlist()->count() > 0 ) {
            playItem( The::playlist()->item( 0 ) );
            return;
        }

        // Nothing chosen, nothing queued, nothing to resume: pressing play
        // means "the one I am looking at, from the top", and what that is
        // belongs to the window.
        emit nothingToPlay();
        return;
    }

    // Somebody who presses play while we are holding the track back is entitled
    // to whatever comes out of it.
    m_waitingForBuffer = false;

    player()->play();
}

void AudioEngine::stop()
{
    if( m_mediaPlayer )
        m_mediaPlayer->stop();
}


bool AudioEngine::isPlaying() const
{
    return m_state == GrooveOff::PlayerState::Playing;
}

bool AudioEngine::isPaused() const
{
    return m_state == GrooveOff::PlayerState::Paused;
}

bool AudioEngine::isStopped() const
{
    return m_state == GrooveOff::PlayerState::Stopped;
}

GrooveOff::PlayerState AudioEngine::state() const
{
    return m_state;
}

qint64 AudioEngine::currentTime() const
{
    // Before anything has been loaded the player sits at zero, and zero is not
    // where the track is: opening the window and closing it again without
    // pressing anything would write that zero over the position being kept.
    if( m_resumePosition > 0 && ( !m_mediaPlayer || m_mediaPlayer->source().isEmpty() ) )
        return m_resumePosition;

    return m_mediaPlayer ? m_mediaPlayer->position() : 0;
}

qint64 AudioEngine::currentTrackTotalTime() const
{
    return m_mediaPlayer ? m_mediaPlayer->duration() : 0;
}

PlaylistItemPtr AudioEngine::currentTrack() const
{
    return m_currentTrack;
}

qint64 AudioEngine::remainingTime() const
{
    if( !m_mediaPlayer )
        return 0;

    return m_mediaPlayer->duration() - m_mediaPlayer->position();
}

bool AudioEngine::canGoNext()
{
    if( m_oldTrack ) {
        int currentIndex = The::playlist()->row( m_oldTrack );
        if( currentIndex >= 0 ) {
            if( currentIndex < ( The::playlist()->count() - 1 ) )
                return true;
        }
    }

    return false;
}

bool AudioEngine::canGoPrevious()
{
    if( m_oldTrack ) {
        int currentIndex = The::playlist()->row( m_oldTrack );
        if( currentIndex > 0 )
            return true;
    }

    return false;
}

void AudioEngine::next()
{
    if( canGoNext() ) {
        PlaylistItemPtr track = The::playlist()->item( The::playlist()->row( m_oldTrack ) + 1 );
        playItem(track);
    }
}

void AudioEngine::previous()
{
    if( canGoPrevious() ) {
        PlaylistItemPtr track = The::playlist()->item( The::playlist()->row( m_currentTrack ) - 1 );
        playItem(track);
    }
}

void AudioEngine::removingTrack( const PlaylistItemPtr &track )
{
    if( m_pendingTrack == track )
        m_pendingTrack.clear();

    if( m_currentTrack == track ) {
        emit removedPlayingTrack();
        if( m_mediaPlayer )
            m_mediaPlayer->stop();
        m_currentTrack = PlaylistItemPtr();
        next();
    }
}

void AudioEngine::seek( qint64 ms )
{
    if ( !isSeekable() )
    {
        qDebug() << "Could not seek!";
        return;
    }

    if ( isPlaying() || isPaused() )
    {
        m_mediaPlayer->setPosition( ms );
        emit seeked( ms, true );
    }
}

void
AudioEngine::seek( int ms )
{
    seek( (qint64) ms );
}

void AudioEngine::setMuted( bool mute )
{
    m_muted = mute;

    if( m_audioOutput )
        m_audioOutput->setMuted( mute );
    if( !isMuted() )
        setVolume( m_volume );
    emit muteStateChanged( mute );
}

/*!
  \brief setVolume: sets the output volume

  QAudioOutput works on a linear scale while the volume we store and display is
  perceptual, hence the conversion: without it the dial would feel wrong at the
  low end and volumes saved by previous versions would play back louder.
*/
void AudioEngine::setVolume( int percentage )
{
    percentage = qBound( 0, percentage, 100 );
    m_volume = percentage;

    const float volume = QtAudio::convertVolume( percentage / 100.0f,
                                                 QtAudio::LogarithmicVolumeScale,
                                                 QtAudio::LinearVolumeScale );

    // Remembered and applied when there is an output to apply it to: the volume
    // is restored from the settings while the window is being built, which is
    // exactly the moment this class exists not to be building a player in.
    if( !m_audioOutput )
        return;

    if ( !qFuzzyCompare( m_audioOutput->volume(), volume ) )
    {
        m_audioOutput->setVolume( volume );
        emit volumeChanged( percentage );
    }
}

void AudioEngine::onVolumeChanged( float volume )
{
    const float perceptual = QtAudio::convertVolume( volume,
                                                     QtAudio::LinearVolumeScale,
                                                     QtAudio::LogarithmicVolumeScale );
    int percent = qBound( 0, qRound( perceptual * 100 ), 100 );

    if ( m_volume != percent )
        emit volumeChanged( percent );
    m_volume = percent;
}

void AudioEngine::onMutedChanged( bool mute )
{
    emit muteStateChanged( mute );
}

bool AudioEngine::isMuted() const
{
    return m_audioOutput ? m_audioOutput->isMuted() : m_muted;
}

int AudioEngine::volume() const
{
    return m_volume;
}

bool AudioEngine::isSeekable() const
{
    if( m_mediaPlayer )
        return m_mediaPlayer->isSeekable();
    return false;
}

void AudioEngine::onSeekableChanged( bool seekable )
{
    emit seekableChanged( seekable );
}

void AudioEngine::onTrackLengthChanged( qint64 milliseconds )
{
    emit trackLengthChanged( milliseconds );
}
