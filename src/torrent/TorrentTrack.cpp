/*
    GrooveOff - Music streaming from torrents
    Copyright (C) 2026  Giuseppe Calà <jiveaxe@gmail.com>

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


#include "TorrentTrack.h"
#include "TorrentSession.h"

#include "../grooveoff/Settings.h"

#include <QSettings>

namespace {

/**
 * How much audio a deadline window covers, guessed from the seconds the user
 * asked for. The bitrate is unknown until the player says so, and a guess that
 * is too generous only means asking for a little more than needed, so the
 * guess errs on the side of a flac.
 */
const qint64 assumedBytesPerSecond = 128 * 1024;

/** a ceiling, so that a torrent with tiny pieces does not get hundreds of deadlines */
const int maxWindowPieces = 32;

/** milliseconds added per piece of distance: the next one is wanted first */
const int deadlineStepMs = 100;

/**
 * The tail is asked for at once. ffmpeg probes the end of a file when it opens
 * it — id3v1, the seek index, the moov atom of an mp4 — and without this the
 * opening hangs on a slow torrent in a way that looks like a broken server.
 */
const int tailDeadlineMs = 3000;

/**
 * How far away a background piece is wanted. It is not a delay: it is how
 * libtorrent is told "take this with the bandwidth that is left over", which
 * is what set_piece_deadline is for.
 */
const int backgroundDeadlineMs = 30000;

} // namespace

TorrentTrack::TorrentTrack( const QString &infoHash, int fileIndex, Mode mode, QObject *parent )
    : QObject( parent )
    , m_infoHash( infoHash )
    , m_fileIndex( fileIndex )
    , m_mode( mode )
    , m_size( The::torrentSession()->fileSize( infoHash, fileIndex ) )
    , m_windowFirst( -1 )
    , m_windowLast( -1 )
{
    if( !isValid() )
        return;

    // The track being listened to never gives way; the one after it takes what
    // is left.
    The::torrentSession()->setFilePriority( m_infoHash, m_fileIndex,
                                            m_mode == Foreground ? TorrentSession::TopFile
                                                                 : TorrentSession::NormalFile );

    const TorrentSession::PieceLocation tail = The::torrentSession()->locate( m_infoHash, m_fileIndex, m_size - 1 );
    if( tail.piece >= 0 ) {
        The::torrentSession()->setPieceDeadline( m_infoHash, tail.piece,
                                                 m_mode == Foreground ? tailDeadlineMs
                                                                      : backgroundDeadlineMs );
    }

    wantAt( 0 );
}

qint64 TorrentTrack::bytesForSeconds( int seconds )
{
    return qMax( 1, seconds ) * assumedBytesPerSecond;
}

TorrentTrack::Mode TorrentTrack::mode() const
{
    return m_mode;
}

TorrentTrack::~TorrentTrack()
{
    if( !isValid() )
        return;

    for( int piece = m_windowFirst; piece >= 0 && piece <= m_windowLast; ++piece )
        The::torrentSession()->resetPieceDeadline( m_infoHash, piece );

    const TorrentSession::PieceLocation tail = The::torrentSession()->locate( m_infoHash, m_fileIndex, m_size - 1 );
    if( tail.piece >= 0 )
        The::torrentSession()->resetPieceDeadline( m_infoHash, tail.piece );

    // Back to resting. Skipping this is what leaves ten files downloading at
    // once after ten skips, with a cache full of music nobody listened to.
    The::torrentSession()->setFilePriority( m_infoHash, m_fileIndex, TorrentSession::SkipFile );
}

QString TorrentTrack::infoHash() const
{
    return m_infoHash;
}

int TorrentTrack::fileIndex() const
{
    return m_fileIndex;
}

qint64 TorrentTrack::size() const
{
    return m_size;
}

QString TorrentTrack::localPath() const
{
    return The::torrentSession()->filePath( m_infoHash, m_fileIndex );
}

bool TorrentTrack::isValid() const
{
    return m_size > 0 && The::torrentSession()->hasMetadata( m_infoHash );
}

int TorrentTrack::pieceAt( qint64 offset ) const
{
    return The::torrentSession()->locate( m_infoHash, m_fileIndex, offset ).piece;
}

bool TorrentTrack::isReadableAt( qint64 offset ) const
{
    const int piece = pieceAt( offset );
    return piece >= 0 && The::torrentSession()->havePiece( m_infoHash, piece );
}

qint64 TorrentTrack::pieceEnd( qint64 offset ) const
{
    const TorrentSession::PieceLocation location = The::torrentSession()->locate( m_infoHash, m_fileIndex, offset );
    return location.piece < 0 ? offset : location.pieceEnd;
}

void TorrentTrack::wantAt( qint64 offset )
{
    if( !isValid() || offset < 0 || offset >= m_size )
        return;

    const int piece = pieceAt( offset );
    if( piece < 0 )
        return;

    // Already covered: re-arming the same deadlines at every stall would churn
    // the request order without bringing a single byte forward any sooner.
    if( piece >= m_windowFirst && piece <= m_windowLast )
        return;

    armWindow( offset );
}

void TorrentTrack::armWindow( qint64 offset )
{
    // The pieces of the window being left go back to normal one by one, rather
    // than with clear_piece_deadlines(), which would also drop the deadlines of
    // another file of the same torrent.
    for( int piece = m_windowFirst; piece >= 0 && piece <= m_windowLast; ++piece )
        The::torrentSession()->resetPieceDeadline( m_infoHash, piece );

    QSettings settings;
    const qint64 windowBytes = bytesForSeconds( settings.value( Settings::bufferSeconds,
                                                                Settings::bufferSecondsDefault ).toInt() );

    m_windowFirst = -1;
    m_windowLast  = -1;

    qint64 cursor  = offset;
    qint64 covered = 0;
    int    step    = 0;

    // Forward from where the reader is, one piece at a time, each wanted a
    // little later than the one before it: the piece needed next is the piece
    // asked for first.
    while( cursor < m_size && covered < windowBytes && step < maxWindowPieces ) {
        const TorrentSession::PieceLocation location =
            The::torrentSession()->locate( m_infoHash, m_fileIndex, cursor );

        if( location.piece < 0 || location.pieceEnd <= cursor )
            break;

        // Short and growing for the track being listened to; far away for the
        // one being taken in the background, and growing there too so that its
        // own order is kept.
        The::torrentSession()->setPieceDeadline( m_infoHash, location.piece,
                                                 ( m_mode == Foreground ? 0 : backgroundDeadlineMs )
                                                 + deadlineStepMs * step );

        if( m_windowFirst < 0 )
            m_windowFirst = location.piece;
        m_windowLast = location.piece;

        covered += location.pieceEnd - cursor;
        cursor   = location.pieceEnd;
        ++step;
    }
}
