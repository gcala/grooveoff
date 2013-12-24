set(grooveoff_SRCS actioncollection.cpp aboutdialog.cpp
                   audioengine.cpp
                   combobox.cpp
                   configdialog.cpp
                   coverdownloader.cpp
                   covermanager.cpp
                   downloaditem.cpp
                   downloadlist.cpp
                   elidedlabel.cpp
                   filterproxymodel.cpp
                   mainwindow.cpp
                   playanimationwidget.cpp
                   playerwidget.cpp
                   playlist.cpp
                   playlistitem.cpp
                   qled.cpp
                   matchitem.cpp
                   settingsitem.cpp
                   timerbutton.cpp
                   timerlabel.cpp
                   utility.cpp
                   dbus/mpris2.cpp
                   dbus/mpris_manager.cpp
                   ../libgrooveshark/apirequest.cpp
                   ../libgrooveshark/config.cpp
                   ../libgrooveshark/requesthandler.cpp
                   ../libgrooveshark/song.cpp
                   ../libgrooveshark/songlist.cpp
                   ../libgrooveshark/token.cpp
                   ../libgrooveshark/urlbuilder.cpp
                   ../libgrooveshark/streamkey.cpp
                   ../libgrooveshark/downloader.cpp
                   ../libgrooveshark/mapbuilder.cpp)

set(grooveoff_RCS ${CMAKE_SOURCE_DIR}/grooveoff.qrc)

# use ui files
set(grooveoff_UI downloaditem.ui
                 playerwidget.ui
                 mainwindow.ui
                 configdialog.ui
                 settingsitem.ui
                 aboutdialog.ui
                 matchitem.ui)

if( Qt5Core_DIR )
    include_directories(${Qt5Core_INCLUDE_DIRS}
                    ${Qt5Widgets_INCLUDE_DIRS}
                    ${Qt5Network_INCLUDE_DIRS}
                    ${Qt5Xml_INCLUDE_DIRS}
                    ${Qt5Svg_INCLUDE_DIRS}
                    ${PHONON4QT5_INCLUDES}
                    ${CMAKE_CURRENT_BINARY_DIR}
                    ${CMAKE_CURRENT_SOURCE_DIR})

    qt5_add_resources(grooveoff_RC_SRCS ${grooveoff_RCS} )
    qt5_wrap_ui(grooveoff_UI_HEADERS ${grooveoff_UI})
    qt5_add_translation(grooveoff_I18N ${TRANSLATIONS})
endif( Qt5Core_DIR )
if( NOT Qt5Core_DIR )
    include_directories(${QT_INCLUDES}
                    ${PHONON_INCLUDES}
                    ${QJSON_INCLUDE_DIR}
                    ${CMAKE_CURRENT_BINARY_DIR}
                    ${CMAKE_CURRENT_SOURCE_DIR})
    qt4_add_resources(grooveoff_RC_SRCS ${grooveoff_RCS} )
    qt4_wrap_ui(grooveoff_UI_HEADERS ${grooveoff_UI})
    qt4_add_translation(grooveoff_I18N ${TRANSLATIONS})
    qt4_add_dbus_adaptor(grooveoff_SRCS
                         dbus/org.mpris.MediaPlayer2.Player.xml
                         dbus/mpris2.h Mpris2 mpris2Player Mpris2Player)
    qt4_add_dbus_adaptor(grooveoff_SRCS
                         dbus/org.mpris.MediaPlayer2.xml
                         dbus/mpris2.h Mpris2 mpris2Root Mpris2Root)
endif( NOT Qt5Core_DIR )

add_executable(grooveoff main.cpp ${grooveoff_SRCS} ${grooveoff_RC_SRCS} ${grooveoff_UI_HEADERS} ${grooveoff_I18N})

if( Qt5Core_DIR )
target_link_libraries(grooveoff
                      ${Qt5Core_LIBRARIES}
                      ${Qt5Widgets_LIBRARIES}
                      ${Qt5Network_LIBRARIES}
                      ${Qt5Svg_LIBRARIES}
                      ${Qt5Xml_LIBRARIES}
                      ${Qt5DBus_LIBRARIES}
                      ${PHONON4QT5_LIBRARY})
endif( Qt5Core_DIR )
if( NOT Qt5Core_DIR )
target_link_libraries(grooveoff
                      ${QT_QTCORE_LIBRARY}
                      ${QT_QTGUI_LIBRARY}
                      ${QT_QTNETWORK_LIBRARY}
                      ${QT_QTSVG_LIBRARY}
                      ${QT_QTXML_LIBRARY}
                      ${QT_QTDBUS_LIBRARY}
                      ${QJSON_LIBRARY}
                      ${PHONON_LIBRARY})
endif( NOT Qt5Core_DIR )

install(TARGETS grooveoff RUNTIME DESTINATION bin)
install(FILES ${grooveoff_I18N} DESTINATION share/grooveoff/translations/)
install(FILES ${CMAKE_SOURCE_DIR}/grooveoff.desktop DESTINATION share/applications )
install(FILES ${CMAKE_SOURCE_DIR}/resources/grooveoff.png
        DESTINATION share/icons/hicolor/128x128/apps/)
install(FILES ${CMAKE_SOURCE_DIR}/resources/grooveoff-16.png
        DESTINATION share/icons/hicolor/16x16/apps/ RENAME grooveoff.png)
install(FILES ${CMAKE_SOURCE_DIR}/resources/grooveoff-24.png
        DESTINATION share/icons/hicolor/24x24/apps/ RENAME grooveoff.png)
install(FILES ${CMAKE_SOURCE_DIR}/resources/grooveoff-32.png
        DESTINATION share/icons/hicolor/32x32/apps/ RENAME grooveoff.png)
install(FILES ${CMAKE_SOURCE_DIR}/resources/grooveoff-48.png
        DESTINATION share/icons/hicolor/48x48/apps/ RENAME grooveoff.png)
install(FILES ${CMAKE_SOURCE_DIR}/resources/grooveoff-64.png
        DESTINATION share/icons/hicolor/64x64/apps/ RENAME grooveoff.png)
install(FILES ${CMAKE_SOURCE_DIR}/resources/grooveoff-96.png
        DESTINATION share/icons/hicolor/96x96/apps/ RENAME grooveoff.png)
