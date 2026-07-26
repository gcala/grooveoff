add_executable(dbtest DatabaseTest.cpp)

target_link_libraries(dbtest PRIVATE
                      grooveoff_core
                      Qt6::Test)

add_test(NAME database COMMAND dbtest)

# The torrent test builds its own .torrent, hence the direct dependency on
# libtorrent: it is the one place outside src/torrent that is allowed to know
# what lt:: looks like.
add_executable(torrenttest TorrentSessionTest.cpp)

target_link_libraries(torrenttest PRIVATE
                      grooveoff_torrent
                      LibtorrentRasterbar::torrent-rasterbar
                      Qt6::Test)

add_test(NAME torrent COMMAND torrenttest)

# Same trick, one step further: the content is written into the cache the
# session uses, so every piece is there and the http side can be tested whole
# without a single peer.
add_executable(streamtest StreamServerTest.cpp)

target_link_libraries(streamtest PRIVATE
                      grooveoff_torrent
                      LibtorrentRasterbar::torrent-rasterbar
                      Qt6::Test)

add_test(NAME stream COMMAND streamtest)

# Hand driven, needs a swarm and a pair of eyes: built so that it cannot rot,
# never run by ctest.
add_executable(torrentcli TorrentCli.cpp)

target_link_libraries(torrentcli PRIVATE
                      grooveoff_torrent
                      Qt6::Core)
