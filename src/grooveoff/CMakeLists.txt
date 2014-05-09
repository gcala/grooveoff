set(grooveoff_SRCS App.cpp
                   ActionCollection.cpp
                   AboutDialog.cpp
                   AudioEngine.cpp
                   ConfigDialog.cpp
                   CoverDownloader.cpp
                   CoverManager.cpp
                   DownloadItem.cpp
                   DownloadList.cpp
                   MainWindow.cpp
                   MatchItem.cpp
                   PlayerWidget.cpp
                   Playlist.cpp
                   PlaylistAction.cpp
                   PlaylistItem.cpp
                   SettingsItem.cpp
                   Utility.cpp
                   SvgHandler.cpp
                   SvgTinter.cpp
                   PaletteHandler.cpp
                   SessionItem.cpp
                   SessionManager.cpp
                   SessionReaderWriter.cpp
                   widgets/SvgButton.cpp
                   widgets/ElidedComboBox.cpp
                   widgets/PlayAnimationWidget.cpp
                   widgets/ElidedLabel.cpp
                   widgets/TimerButton.cpp
                   widgets/TimerLabel.cpp
                   widgets/VolumeDial.cpp
                   widgets/IconButton.cpp
                   widgets/NamingElements.cpp
                   widgets/NamingSchema.cpp
                   widgets/Spinner.cpp
                   widgets/SessionList.cpp
                   widgets/TrackList.cpp
                   dbus/mpris.cpp
                   dbus/mpris1/Mpris1PluginRootAdaptor.cpp
                   dbus/mpris1/Mpris1PluginPlayerAdaptor.cpp
                   dbus/mpris2/Mpris2PluginRootAdaptor.cpp
                   dbus/mpris2/Mpris2PluginPlayerAdaptor.cpp
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

LIST(APPEND grooveoff_SRCS ${THIRDPARTY_DIR}/QtSingleApplication/qtsingleapplication.cpp
                           ${THIRDPARTY_DIR}/QtSingleApplication/qtlocalpeer.cpp)

set(grooveoff_RCS ${CMAKE_SOURCE_DIR}/grooveoff.qrc)

# use ui files
set(grooveoff_UI DownloadItem.ui
                 PlayerWidget.ui
                 MainWindow.ui
                 ConfigDialog.ui
                 SettingsItem.ui
                 AboutDialog.ui
                 MatchItem.ui
                 SessionManager.ui
                 SessionItem.ui)

if( Qt5Core_DIR )
    include_directories(${Qt5Core_INCLUDE_DIRS}
                    ${Qt5Widgets_INCLUDE_DIRS}
                    ${Qt5Network_INCLUDE_DIRS}
                    ${Qt5Xml_INCLUDE_DIRS}
                    ${Qt5Svg_INCLUDE_DIRS}
                    ${PHONON4QT5_INCLUDES}
                    ${CMAKE_CURRENT_BINARY_DIR}
                    ${CMAKE_CURRENT_SOURCE_DIR}
                    ${THIRDPARTY_DIR}/QtSingleApplication)

    qt5_add_resources(grooveoff_RC_SRCS ${grooveoff_RCS} )
    qt5_wrap_ui(grooveoff_UI_HEADERS ${grooveoff_UI})
    qt5_add_translation(grooveoff_I18N ${TRANSLATIONS})
endif( Qt5Core_DIR )
if( NOT Qt5Core_DIR )
    include_directories(${QT_INCLUDES}
                    ${PHONON_INCLUDES}
                    ${QJSON_INCLUDE_DIR}
                    ${CMAKE_CURRENT_BINARY_DIR}
                    ${CMAKE_CURRENT_SOURCE_DIR}
                    ${THIRDPARTY_DIR}/QtSingleApplication)
    qt4_add_resources(grooveoff_RC_SRCS ${grooveoff_RCS} )
    qt4_wrap_ui(grooveoff_UI_HEADERS ${grooveoff_UI})
    qt4_add_translation(grooveoff_I18N ${TRANSLATIONS})
endif( NOT Qt5Core_DIR )

add_executable(grooveoff main.cpp ${grooveoff_SRCS} ${grooveoff_RC_SRCS} ${grooveoff_UI_HEADERS} ${grooveoff_I18N})

LIST(APPEND LINK_LIBRARIES ${LINK_LIBRARIES} grooveoff_qtsingleapplication )

if( Qt5Core_DIR )
target_link_libraries(grooveoff
                      ${LINK_LIBRARIES}
                      ${Qt5Core_LIBRARIES}
                      ${Qt5Widgets_LIBRARIES}
                      ${Qt5Network_LIBRARIES}
                      ${Qt5Svg_LIBRARIES}
                      ${Qt5Xml_LIBRARIES}
                      ${Qt5DBus_LIBRARIES}
                      ${PHONON4QT5_LIBRARY}
                      ${TAGLIB_LIBRARY})
endif( Qt5Core_DIR )
if( NOT Qt5Core_DIR )
target_link_libraries(grooveoff
                      ${LINK_LIBRARIES}
                      ${QT_QTCORE_LIBRARY}
                      ${QT_QTGUI_LIBRARY}
                      ${QT_QTNETWORK_LIBRARY}
                      ${QT_QTSVG_LIBRARY}
                      ${QT_QTXML_LIBRARY}
                      ${QT_QTDBUS_LIBRARY}
                      ${QJSON_LIBRARY}
                      ${PHONON_LIBRARY}
                      ${TAGLIB_LIBRARY})
endif( NOT Qt5Core_DIR )

install(TARGETS grooveoff RUNTIME DESTINATION bin)
install(FILES ${grooveoff_I18N} 
        DESTINATION share/apps/grooveoff/translations/)
install(FILES ${CMAKE_SOURCE_DIR}/grooveoff.desktop 
        DESTINATION share/applications )
install(FILES ${CMAKE_SOURCE_DIR}/resources/default-theme.svg
        DESTINATION share/apps/grooveoff/resources )
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
install(FILES ${CMAKE_SOURCE_DIR}/resources/grooveoff-72.png
        DESTINATION share/icons/hicolor/72x72/apps/ RENAME grooveoff.png)
install(FILES ${CMAKE_SOURCE_DIR}/resources/grooveoff-96.png
        DESTINATION share/icons/hicolor/96x96/apps/ RENAME grooveoff.png)
install(FILES ${CMAKE_SOURCE_DIR}/resources/grooveoff-128.png
        DESTINATION share/icons/hicolor/128x128/apps/ RENAME grooveoff.png)
install(FILES ${CMAKE_SOURCE_DIR}/resources/grooveoff-256.png
        DESTINATION share/icons/hicolor/256x256/apps/ RENAME grooveoff.png)
install(FILES ${CMAKE_SOURCE_DIR}/resources/grooveoff-512.png
        DESTINATION share/icons/hicolor/512x512/apps/ RENAME grooveoff.png)
install(FILES ${CMAKE_SOURCE_DIR}/resources/grooveoff.svgz
        DESTINATION share/icons/hicolor/scalable/apps/ RENAME grooveoff.svgz)
