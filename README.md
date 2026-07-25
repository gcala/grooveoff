# Offline Grooveshark.com music

GrooveOff accesses the huge Grooveshark database through its [public api](http://developers.grooveshark.com/docs/public_api/v3/).

## Translations

Submit translations to Transifex' [page](https://www.transifex.com/projects/p/grooveoff/resource/translations/).

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