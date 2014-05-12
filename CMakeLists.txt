project(grooveoff)
# ==================================================================================
# Information to update before to release this package.

# grooveoff info
SET(GROOVEOFF_VERSION "0.2.0" )
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config-version.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/src/config-version.h )
# ==================================================================================

cmake_minimum_required(VERSION 2.8.6)
set( CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/modules )
set( CMAKE_AUTOMOC ON )
SET( THIRDPARTY_DIR  ${CMAKE_SOURCE_DIR}/thirdparty )

option(BUILD_WITH_QT5 "Build GrooveOff with Qt5, if found" OFF)

if( BUILD_WITH_QT5 )
    find_package(Qt5Core)
    if( Qt5Core_DIR )
        message(STATUS "Found Qt5! Be aware that Qt5-support is still experimental and not officially supported!")
        ADD_DEFINITIONS(-fPIC)
        find_package(Qt5Widgets REQUIRED)
        find_package(Qt5Gui REQUIRED)
        find_package(Qt5Network REQUIRED)
        find_package(Qt5Xml REQUIRED)
        find_package(Qt5Svg REQUIRED)
        find_package(Qt5DBus REQUIRED)
        find_package(Qt5LinguistTools REQUIRED)
        find_package(Phonon4Qt5 REQUIRED)
    endif( Qt5Core_DIR )
endif( BUILD_WITH_QT5 )
if( NOT Qt5Core_DIR )
    find_package(Qt4 4.7.0 REQUIRED QtCore QtGui QtXml QtNetwork QtSvg QtDbus)
    find_package(Phonon REQUIRED)
    find_package(QJson REQUIRED)
endif( NOT Qt5Core_DIR )

find_package(Taglib REQUIRED)

# Handle Translations, pick all grooveoff_* files from trans directory.
file( GLOB TRANS_FILES ${CMAKE_SOURCE_DIR}/translations/grooveoff_*.ts)
set( TRANSLATIONS ${TRANS_FILES} )

add_subdirectory( thirdparty )
add_subdirectory(src)
