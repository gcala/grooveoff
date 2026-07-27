# Music streaming from torrents

GrooveOff plays music out of a BitTorrent swarm without downloading it. You
paste a magnet link or open a `.torrent` file, the torrent is treated as a
remote playlist, and the track you choose starts as soon as enough of its
beginning has arrived — seeking included, forwards and backwards.

It used to be a Grooveshark client. That service closed in 2015, which left a
working music player attached to an API that no longer answers; this is what
became of it.

## How it works

Adding a torrent costs its metadata and nothing else: every file is set to "do
not download" the moment the file list is known, so a torrent added and never
listened to occupies no disk. A selector then asks which of its files belong in
its playlist.

Choosing a track raises the priority of that one file and puts deadlines on the
pieces ahead of the reader, and a small local HTTP server hands the player a
URL instead of a path — which is what makes a seek an ordinary request starting
somewhere else rather than a special case. The next track in the list is
brought up while the current one is still playing, so a playlist does not stop
between tracks.

What has been played stays in a cache with a ceiling you set. When it fills up,
the torrents played longest ago are thrown away — **the playlists are not
touched**: a track whose data are gone is fetched again the next time you play
it.

Playlists come in two kinds. One is born with a torrent and holds its files;
one you build yourself by dragging tracks in from any torrent. Both live in the
same sidebar.

## Privacy, stated plainly

**Taking part in a BitTorrent swarm shows your IP address to every other peer.**
Anyone who scrapes the tracker or queries the DHT can list who is taking part.
GrooveOff never uploads anything — it is configured never to unchoke a peer —
and that does not change the above: you are still a visible participant, just
one that does not give anything back. Nothing in the interface is called
anonymous, because no setting of libtorrent's own hides an address from the
swarm and naming one so would be a promise the software cannot keep.

The one setting here that does hide it is a SOCKS5 or HTTP proxy, in the
network page of the settings, and it covers peer and tracker connections both.
A VPN outside the application does the same.

**With the Grooveshark client gone, the application makes no network calls of
its own.** No telemetry, no service, no CDN: only libtorrent, towards the peers
of the torrents you added yourself. GrooveOff neither searches nor indexes
anything — what to fetch is decided entirely by whoever uses it, and so is the
responsibility for it.

## Paths

```
~/.local/share/gcala/grooveoff/grooveoff.db     catalogue and playlists — keep this
~/.cache/gcala/grooveoff/                       the cached pieces and fast resume data
```

Everything under `.cache` can be deleted by hand without losing anything that
cannot be fetched again. Nothing under `.local/share` can.

## Installation

GrooveOff requires Qt 6.5 or later, a QtMultimedia backend, TagLib 2,
libtorrent-rasterbar 2 and KDSingleApplication. See `INSTALL` for the full
list.

```bash
cmake -S . -B build -DCMAKE_INSTALL_PREFIX=/usr
cmake --build build
cmake --install build (as root)
```

## Translations

The catalogues live in `translations/` and are compiled into the binary. Eleven
languages came from a Transifex project that has been quiet for over a decade;
rather than keep pointing at it, they are maintained here now. Italian is
complete, the others are partial — where a string has no translation Qt falls
back to the English in the source, so nothing is broken, only mixed.

To pick up strings added or changed in the code:

```bash
cmake --build build --target update_translations
```

New strings appear untranslated and removed ones are marked obsolete rather
than deleted, so nobody's work is thrown away by running it. Patches against
any `translations/grooveoff_*.ts` are welcome.

## Licence

GPL v3 or later. See `LICENSE`.

Support the artists you like by buying their music.
