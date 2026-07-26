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

# Hand driven, needs a swarm and a pair of eyes: built so that it cannot rot,
# never run by ctest.
add_executable(torrentcli TorrentCli.cpp)

target_link_libraries(torrentcli PRIVATE
                      grooveoff_torrent
                      Qt6::Core)
