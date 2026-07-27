# Offline Grooveshark.com music

GrooveOff accesses the huge Grooveshark database through its [public api](http://developers.grooveshark.com/docs/public_api/v3/).

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

## Installation

GrooveOff requires Qt 6.5 or later, a QtMultimedia backend, TagLib 2 and
KDSingleApplication. See `INSTALL` for the full list.

```bash
cmake -S . -B build -DCMAKE_INSTALL_PREFIX=/usr
cmake --build build
cmake --install build (as root)
```

## Disclaimer

GrooveOff is not affiliated with Grooveshark™ and has not been reviewed or officially approved by Grooveshark.com.
The author is not responsible for any violations this application does to Grooveshark's TOS.
The author is not related to Grooveshark™ in any way!
Support the Artists You Like by Buying Their Music.