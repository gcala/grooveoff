project(grooveoff)
# ==================================================================================
# Information to update before to release this package.

# grooveoff info
SET(GROOVEOFF_VERSION "0.1.8-git" )
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config-version.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/src/config-version.h )
# ==================================================================================

cmake_minimum_required(VERSION 2.8.6)
#set(GROOVEOFF_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set( CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/modules )
set( CMAKE_AUTOMOC ON )
option(BUILD_WITH_QT4 "Build Tomahawk with Qt4 no matter if Qt5 was found" ON)

if( NOT BUILD_WITH_QT4 )
    find_package(Qt5Core)
    if( Qt5Core_DIR )
        message(STATUS "Found Qt5! Be aware that Qt5-support is still experimental and not officially supported!")
        ADD_DEFINITIONS(-fPIC)
        find_package(Qt5Widgets REQUIRED)
        find_package(Qt5Gui REQUIRED)
        find_package(Qt5Network REQUIRED)
        find_package(Qt5Xml REQUIRED)
        find_package(Qt5Svg REQUIRED)
        find_package(Qt5LinguistTools REQUIRED)
        find_package(Phonon4Qt5 REQUIRED)
    endif( Qt5Core_DIR )
endif( NOT BUILD_WITH_QT4 )
if( NOT Qt5Core_DIR )
    find_package(Qt4 COMPONENTS QTCORE QTGUI QTNETWORK QTSVG QTXML)
    find_package(Phonon REQUIRED)
    find_package(QJson REQUIRED)
endif( NOT Qt5Core_DIR )

# Handle Translations, pick all grooveoff_* files from trans directory.
file( GLOB TRANS_FILES ${CMAKE_SOURCE_DIR}/translations/grooveoff_*.ts)
set( TRANSLATIONS ${TRANS_FILES} )

add_subdirectory(src)
