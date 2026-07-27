# The verification by hand

Everything below needs a keyboard, a screen and a real swarm. It is what the
test suite cannot reach: the suite proves the rules, this proves that the
gestures reach them.

The order matters. It runs from the failures that are cheapest to find to the
ones that only show up after everything else works, so that a stumble early on
does not waste the long checks further down.

Useful throughout:

```bash
QT_FORCE_STDERR_LOGGING=1 grooveoff     # Qt logging goes to journald otherwise
sqlite3 ~/.local/share/gcala/grooveoff/grooveoff.db
```

To keep the real catalogue out of it, run with a scratch environment:

```bash
XDG_DATA_HOME=/tmp/go/data XDG_CACHE_HOME=/tmp/go/cache XDG_CONFIG_HOME=/tmp/go/config grooveoff
```

Test material: `tests/data/archive-org-78rpm.torrent` is in the repository and
is what a real torrent looks like — 53 files, 4 mp3, 4 flac, a 1.3 GB zip and a
pile of sidecars. Keep a badly seeded torrent within reach too; it is the case
that breaks first in practice and the one no fast torrent ever exercises.

---

## 1. Adding, and what it costs

- [ ] A magnet in the field, enter. The selector opens with everything ticked.
- [ ] `du -sh ~/.cache/gcala/grooveoff/torrents/<hash>` — **still nothing**.
      Adding a torrent must buy the file list and not one byte of content.
- [ ] Cancel the selector: the torrent stays in the sidebar, the track list does
      not change, and the torrent stops announcing.
- [ ] A malformed magnet: no row appears, the text stays in the field to be
      corrected, and the reason is on the status bar. **No modal.**
- [ ] `tests/data/archive-org-78rpm.torrent` through *Open torrent*: the
      selector shows the 8 audio files, and *Show all files* reveals all 53.

## 2. Playing

- [ ] Play a track. It goes through Buffering and starts.
- [ ] The seek slider, forwards and then backwards into a part already played.
      A jump into a part that has not arrived shows the waiting state and does
      not hang.
- [ ] Tags and cover art appear on the row once the head of the file is in.
- [ ] The led follows the peers of the torrent being listened to.
- [ ] `playerctl` or `busctl --user` drives play, pause and position.

## 3. The gestures — the part nothing else covers

- [ ] Drag a track onto a **custom** playlist in the sidebar: it lands, and the
      count in the tooltip goes up.
- [ ] Drag a track onto a **torrent** playlist: the cursor refuses it before
      the button is released.
- [ ] Drag a track down by one place inside the list. It has to *move by one*.
      Then up by one. Then several selected at once.
- [ ] Close and reopen: the order is the one that was left.
- [ ] *Add to playlist ▸* from the row's own menu does the same as the drag.
- [ ] The row's bin button takes it out of the playlist; *Exclude this track*
      is a different entry with a different result, and it is **not offered**
      while looking at a custom playlist.

## 4. Selection and exclusion

- [ ] Add a torrent with many files, tick three. Three in the list, **all** of
      them in `track`.
- [ ] Exclude a track from its menu. Reopen the selector of that torrent: it is
      greyed out and unticked.
- [ ] Press *Select all* — the excluded one must **not** come back.
- [ ] Take a different one out with the bin button, reopen the selector: that
      one *is* selectable again. Removing from a list is not excluding.

## 5. Removal — the one that can destroy the user's work

- [ ] Remove a torrent whose tracks are in no custom playlist: a plain yes or
      no, and everything goes — rows, files, resume blob.
- [ ] Put tracks of a torrent into two custom playlists, then remove it. The
      dialog must say **how many tracks and which playlists by name**.
- [ ] *Cancel*: `SELECT count(*) FROM track;` before and after — identical.
- [ ] *Keep those tracks*: the torrent leaves the sidebar, `hidden = 1`, the
      tracks stay in the custom playlists **and still play**.
- [ ] On that hidden torrent, take its tracks out of the custom playlists one
      at a time. At the last one, the torrent and its files disappear on their
      own.
- [ ] Hygiene, worth repeating after a day's use:
      ```sql
      SELECT t.info_hash FROM torrent t WHERE t.hidden = 1
        AND NOT EXISTS ( SELECT 1 FROM playlist_track pt
                         JOIN track tr ON tr.id = pt.track_id
                         JOIN playlist p ON p.id = pt.playlist_id
                        WHERE tr.info_hash = t.info_hash AND p.kind = 1 );
      ```
      Any row here means the garbage collection has a hole.
- [ ] *Remove everything* on a torrent with references: the tracks go from the
      custom playlists too, and the playlists themselves survive, empty.
- [ ] Add the same magnet again after *Keep those tracks*: it comes back with
      the selection and the exclusions it had, not as a new torrent.

## 6. Cache against catalogue

- [ ] Put a track in a custom playlist, then lower `cacheSize` below what is
      held so its torrent is evicted.
- [ ] The track must **stay in the playlist**, go back to Dormant, and be
      fetched again at the next play. If it disappears, a `CASCADE` has been
      used where an `UPDATE` belonged, and that is the worst bug this project
      can have.
- [ ] The torrent being listened to is never evicted, however old.
- [ ] *Empty the cache* from the menu: the bytes go, the playlists do not.

## 7. Persistence, the full circle

- [ ] Two torrents, one track listened to halfway. Close.
- [ ] `ls ~/.cache/gcala/grooveoff/resume/` has a file per torrent, and
      `session.dat` exists.
- [ ] **Unplug the network** and reopen. The lists appear identical, the last
      track is selected and the slider is where it was left. This is the check
      that catches a forgotten `save_info_dict`: with no network the file list
      can only have come from the disk.
- [ ] Plug the network back in, press play: it resumes from the saved position,
      and `download_rate` stays near zero because it is reading the cache.
- [ ] Close during an active buffering, reopen: nothing lost.
- [ ] `kill -9` during playback, reopen: back to the last saved resume point,
      no full recheck of the files, and the database opens without a scratch.

## 8. Prefetch

- [ ] Two tracks of **different torrents** in a row. The change of track must
      have no audible pause.
- [ ] Ten fast manual skips between tracks. Then check that ten files are not
      left downloading at once — that is the proof the cancellation works.

## 9. The local surface

```bash
ss -ltnp | grep grooveoff                     # 127.0.0.1 only, never 0.0.0.0
curl -sI http://127.0.0.1:<port>/<hash>/0     # no token -> 404
curl -sI -H 'Host: evil.example' http://127.0.0.1:<port>/<hash>/0?t=<token>   # -> 404
```

## 10. The interface itself

- [ ] The three layouts — Compact, Wide, Mini — open and the splitter behaves.
- [ ] The SVG theme recolours when the desktop palette changes.
- [ ] The settings in Italian: the three long paragraphs in the pages and the
      three-way removal dialog have to fit without pushing the buttons off.
- [ ] Nothing on the error table produces a modal, except a disk that is full
      or a cache that cannot be written to.

## 11. Zero seeders

Worth its own section because it is what happens most in practice.

- [ ] A badly seeded torrent: after about twenty seconds the row says it is
      waiting, the player does **not** start, and the led is red.
- [ ] Nothing disappears from the list on its own, however long it waits.
