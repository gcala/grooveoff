# The BitTorrent engine, built apart so that the tests and the command line
# harness can drive it without a gui. Static for the same reason as
# grooveoff_core: the installed product stays a single executable.
add_library(grooveoff_torrent STATIC
            TorrentSession.cpp
            TorrentTrack.cpp
            StreamServer.cpp
            CacheManager.cpp)

target_link_libraries(grooveoff_torrent PUBLIC
                      Qt6::Core
                      Qt6::Network
                      grooveoff_core
                      PRIVATE
                      LibtorrentRasterbar::torrent-rasterbar)

target_include_directories(grooveoff_torrent PUBLIC ${CMAKE_SOURCE_DIR}/src)
