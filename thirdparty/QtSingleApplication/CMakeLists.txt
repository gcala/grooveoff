PROJECT(qtsingleapplication)
cmake_minimum_required(VERSION 2.8)

set( CMAKE_AUTOMOC ON )

SET( qtsingleapplicationSources
      qtlocalpeer.cpp
      qtsingleapplication.cpp
   )

if( Qt5Core_DIR )
    include_directories(${Qt5Network_INCLUDE_DIRS}
                        ${Qt5Widgets_INCLUDE_DIRS}
                        ${CMAKE_CURRENT_BINARY_DIR}
                        ${CMAKE_CURRENT_SOURCE_DIR})
endif( Qt5Core_DIR )
if( NOT Qt5Core_DIR )
    INCLUDE_DIRECTORIES(${QT_INCLUDES}
                        ${CMAKE_CURRENT_BINARY_DIR}
                        ${CMAKE_CURRENT_SOURCE_DIR})
endif( NOT Qt5Core_DIR )

ADD_LIBRARY( grooveoff_qtsingleapplication STATIC ${qtsingleapplicationSources} )

if( Qt5Core_DIR )
    TARGET_LINK_LIBRARIES( grooveoff_qtsingleapplication ${Qt5Network_LIBRARIES})
endif( Qt5Core_DIR )
if( NOT Qt5Core_DIR )
    TARGET_LINK_LIBRARIES( grooveoff_qtsingleapplication ${QT_QTNETWORK_LIBRARY})
endif( NOT Qt5Core_DIR )