cmake_minimum_required(VERSION 3.21)

# ==================================================================================
# Information to update before to release this package.
project(grooveoff
        VERSION 0.2.0
        DESCRIPTION "Offline Grooveshark.com music"
        LANGUAGES CXX
)
set(GROOVEOFF_VERSION ${PROJECT_VERSION})
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config-version.h.cmake
               ${CMAKE_CURRENT_BINARY_DIR}/src/config-version.h)
# ==================================================================================

include(GNUInstallDirs)

find_package(Qt6 6.5 REQUIRED COMPONENTS
             Core
             Gui
             Widgets
             Network
             Xml
             Svg
             DBus
             Multimedia
             LinguistTools
)

qt_standard_project_setup()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTORCC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(TagLib 2.0 REQUIRED CONFIG)

# BitTorrent engine: metadata, piece deadlines and streaming primitives.
# CONFIG mode on purpose: the FindLibtorrentRasterbar module shipped with CMake
# falls through to its pkg-config branch even after config mode succeeded, and
# then fails creating an alias target that already exists.
find_package(LibtorrentRasterbar 2.0 REQUIRED CONFIG)

# Single instance handling, replaces the bundled Qt Solutions QtSingleApplication.
find_package(KDSingleApplication-qt6 REQUIRED)

# Handle Translations, pick all grooveoff_* files from translations directory.
file(GLOB TRANSLATIONS ${CMAKE_SOURCE_DIR}/translations/grooveoff_*.ts)

add_subdirectory(src)
