# Model and persistence, built once and linked both into the application and
# into the tests. Static on purpose: the installed product stays a single
# executable, this only avoids compiling the same sources twice.
add_library(grooveoff_core STATIC
            ../grooveoff/TrackInfo.cpp
            Database.cpp
            Catalog.cpp
            PlaylistStore.cpp)

target_link_libraries(grooveoff_core PUBLIC
                      Qt6::Core
                      Qt6::Sql)

target_include_directories(grooveoff_core PUBLIC ${CMAKE_SOURCE_DIR}/src)
