PROJECT(qtsingleapplication)
cmake_minimum_required(VERSION 2.8)

set( CMAKE_AUTOMOC ON )

SET( qtsingleapplicationSources
      qtlocalpeer.cpp
      qtsingleapplication.cpp
   )

INCLUDE_DIRECTORIES(${QT_INCLUDES}
                    ${CMAKE_CURRENT_BINARY_DIR}
                    ${CMAKE_CURRENT_SOURCE_DIR})

ADD_LIBRARY( grooveoff_qtsingleapplication STATIC ${qtsingleapplicationSources} )
TARGET_LINK_LIBRARIES( grooveoff_qtsingleapplication ${QT_QTNETWORK_LIBRARY})
