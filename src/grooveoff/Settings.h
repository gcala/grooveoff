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


#ifndef SETTINGS_H
#define SETTINGS_H

#include <QLatin1String>

/**
  * @namespace Settings
  * @short The keys of everything the streaming side can be configured with.
  *
  * These are read in five different files — the session, the track, the stream
  * server, the player and the cache — and written in one, ConfigDialog. Spelling
  * a key or a default twice is therefore not a theoretical risk: a default that
  * drifts apart from the one the dialog shows gives an option that reads as one
  * thing and behaves as another, and nothing in the build catches it.
  *
  * The purely graphical settings — geometry, splitter, volume, layout — are not
  * here: they are written and read in MainWindow and nowhere else, so a name in
  * one place is already the whole story.
  */
namespace Settings {

/** ceiling of the torrent cache, in megabytes; 0 means no ceiling at all */
inline const QLatin1String cacheSize( "cacheSize" );
inline constexpr int cacheSizeDefault = 2048;

/** whether the cache is thrown away on the way out; it survives by default */
inline const QLatin1String emptyCacheOnExit( "emptyCacheOnExit" );
inline constexpr bool emptyCacheOnExitDefault = false;

/** port of the local stream server; 0 lets the system choose, which is fine */
inline const QLatin1String streamPort( "streamPort" );
inline constexpr int streamPortDefault = 0;

/** port libtorrent listens on for peers */
inline const QLatin1String listenPort( "listenPort" );
inline constexpr int listenPortDefault = 6881;

/** incoming bandwidth ceiling in kilobytes a second; 0 means unlimited */
inline const QLatin1String downloadRateLimit( "downloadRateLimit" );
inline constexpr int downloadRateLimitDefault = 0;

/**
 * Whether anything at all is sent back to the other users. Off by default,
 * which is the plan's decision and stays: what is being streamed leaving the
 * machine is a choice that belongs to the person making it. What the setting
 * is worth was measured — with nothing given back the same torrent stuttered
 * through every track, and with four slots it played through — so the dialog
 * says as much rather than leaving the trade for somebody to find.
 */
inline const QLatin1String sharing( "sharing" );
inline constexpr bool sharingDefault = false;

/**
 * How many peers may be unchoked, which is to say how many may receive
 * anything at all, once sharing is on. It is unchoking and not the rate limit
 * that decides whether this is a client that gives something back: in
 * libtorrent a rate limit of zero means unlimited, not nothing.
 */
inline const QLatin1String uploadSlots( "uploadSlots" );
inline constexpr int uploadSlotsDefault = 4;

/**
 * Outgoing bandwidth ceiling in kilobytes a second, used only while sharing is
 * on. It moves together with the slots above or the gift is not one: a peer
 * unchoked and then fed a kilobyte a second gets nothing worth having and
 * chokes us back anyway.
 */
inline const QLatin1String uploadRateLimit( "uploadRateLimit" );
inline constexpr int uploadRateLimitDefault = 50;

/** how much audio is asked for ahead of the reader, in seconds */
inline const QLatin1String bufferSeconds( "bufferSeconds" );
inline constexpr int bufferSecondsDefault = 10;

/**
 * Whether the tracks of a playlist are fetched as soon as they are chosen,
 * in order, instead of one at a time when somebody asks to hear them.
 *
 * A new key rather than the old prefetchNext, and not for tidiness: that one
 * meant "bring up the next track while this one plays" and this one means
 * "fetch the whole playlist", so a stored yes to the first is not an informed
 * yes to the second. Whoever had switched the prefetch off gets asked again.
 */
inline const QLatin1String downloadAhead( "downloadAhead" );
inline constexpr bool downloadAheadDefault = true;

/** whether the track and position of the last session come back, paused */
inline const QLatin1String restorePlayback( "restorePlayback" );
inline constexpr bool restorePlaybackDefault = true;

/**
 * "none", "socks5" or "http". A proxy is the only thing here that actually
 * hides the address from the swarm — no setting of libtorrent's own does, and
 * calling one of them anonymous in the interface would be a promise the
 * software cannot keep.
 */
inline const QLatin1String proxyType( "proxyType" );
inline const QLatin1String proxyTypeNone( "none" );
inline const QLatin1String proxyTypeSocks5( "socks5" );
inline const QLatin1String proxyTypeHttp( "http" );

inline const QLatin1String proxyHost( "proxyHost" );

inline const QLatin1String proxyPort( "proxyPort" );
inline constexpr int proxyPortDefault = 1080;

} // namespace Settings

#endif // SETTINGS_H
