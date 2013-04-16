project(grooveoff)
# ==================================================================================
# Information to update before to release this package.

# grooveoff info
SET(GROOVEOFF_VERSION "0.1" )
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config-version.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/src/config-version.h )
# ==================================================================================

cmake_minimum_required(VERSION 2.6)
#set(GROOVEOFF_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set( CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/modules )
include_directories(BEFORE "${CMAKE_CURRENT_BINARY_DIR}/src/mirall/")

find_package(Qt4 REQUIRED)
find_package(Phonon REQUIRED)
find_package(QJson REQUIRED)

# Handle Translations, pick all grooveoff_* files from trans directory.
file( GLOB TRANS_FILES ${CMAKE_SOURCE_DIR}/translations/grooveoff_*.ts)
set(TRANSLATIONS ${TRANS_FILES})

add_subdirectory(src)
