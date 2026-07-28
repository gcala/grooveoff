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

# The cache is the one place where a mistake deletes something the user made by
# hand, so the eviction is followed with the catalogue open beside it.
add_executable(cachetest CacheManagerTest.cpp)

target_link_libraries(cachetest PRIVATE
                      grooveoff_torrent
                      LibtorrentRasterbar::torrent-rasterbar
                      Qt6::Test)

add_test(NAME cache COMMAND cachetest)

# The order a playlist is fetched in, asked without a network, a torrent or a
# disk: the rule is a function of three things and lives in one place, which is
# exactly what makes it answerable this way.
add_executable(queuetest DownloadQueueTest.cpp)

target_link_libraries(queuetest PRIVATE
                      grooveoff_torrent
                      Qt6::Test)

add_test(NAME queue COMMAND queuetest)

# Removing a torrent is the one operation that can destroy what the user made
# by hand, and it does so silently, so both ways out of it are followed to the
# end and asked what is left.
add_executable(playlisttest PlaylistTest.cpp
                            ../src/grooveoff/Utility.cpp
                            ../src/grooveoff/widgets/PlaylistList.cpp)

# The sidebar knows the torrent session now: the led of a row is the health of
# the torrent behind it, and asking the window to relay that was making the
# window the owner of something that is not its business. Nothing here starts a
# session — the singleton is built and never told to go online.
target_link_libraries(playlisttest PRIVATE
                      grooveoff_torrent
                      Qt6::Widgets
                      Qt6::Test)

target_include_directories(playlisttest PRIVATE ${CMAKE_SOURCE_DIR}/src/grooveoff)

add_test(NAME playlist COMMAND playlisttest)
set_tests_properties(playlist PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# The selector is compiled into the test rather than linked from the
# executable: the gui is one binary and splitting it into a library to test one
# dialog would cost more than it gives.
add_executable(selectortest TrackSelectorTest.cpp
                            ../src/grooveoff/TrackSelector.cpp
                            ../src/grooveoff/TrackSelector.ui)

target_link_libraries(selectortest PRIVATE
                      grooveoff_core
                      Qt6::Widgets
                      Qt6::Test)

target_include_directories(selectortest PRIVATE ${CMAKE_SOURCE_DIR}/src/grooveoff)

add_test(NAME selector COMMAND selectortest)
set_tests_properties(selector PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# And for the sidebar rows, where the thing worth pinning is not what is drawn
# but where: a led painted past the right edge of the viewport is code that
# runs, paints, and shows nothing at all.
#
# The leds come out of the themed svg, which brings the tinting along with
# them: the theme file itself is not in this binary, and it does not need to
# be — a renderer with nothing loaded hands back an empty pixmap, and what is
# being asked here is where the led goes rather than what it looks like.
add_executable(delegatetest PlaylistDelegateTest.cpp
                            ../src/grooveoff/widgets/PlaylistDelegate.cpp
                            ../src/grooveoff/SvgHandler.cpp
                            ../src/grooveoff/SvgTinter.cpp
                            ../src/grooveoff/PaletteHandler.cpp)

target_link_libraries(delegatetest PRIVATE
                      Qt6::Widgets
                      Qt6::Svg
                      Qt6::Test)

target_include_directories(delegatetest PRIVATE ${CMAKE_SOURCE_DIR}/src/grooveoff)

add_test(NAME delegate COMMAND delegatetest)
set_tests_properties(delegate PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# Same arrangement for the settings dialog, and for one reason above the
# others: a default that drifts apart from the one its reader uses is invisible
# until somebody opens the settings, and then the value changes on its own.
add_executable(configtest ConfigDialogTest.cpp
                          ../src/grooveoff/ConfigDialog.cpp
                          ../src/grooveoff/ConfigDialog.ui
                          ../src/grooveoff/SettingsItem.cpp
                          ../src/grooveoff/SettingsItem.ui)

target_link_libraries(configtest PRIVATE
                      grooveoff_torrent
                      Qt6::Widgets
                      Qt6::Test)

target_include_directories(configtest PRIVATE ${CMAKE_SOURCE_DIR}/src/grooveoff)

add_test(NAME config COMMAND configtest)
set_tests_properties(config PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# Hand driven, needs a swarm and a pair of eyes: built so that it cannot rot,
# never run by ctest.
add_executable(torrentcli TorrentCli.cpp)

target_link_libraries(torrentcli PRIVATE
                      grooveoff_torrent
                      Qt6::Core)
