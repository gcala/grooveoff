set(grooveoff_SRCS main.cpp
                   App.cpp
                   ActionCollection.cpp
                   AboutDialog.cpp
                   AudioEngine.cpp
                   ConfigDialog.cpp
                   DownloadItem.cpp
                   DownloadList.cpp
                   MainWindow.cpp
                   PlayerWidget.cpp
                   Playlist.cpp
                   PlaylistAction.cpp
                   PlaylistItem.cpp
                   SettingsItem.cpp
                   Utility.cpp
                   SvgHandler.cpp
                   SvgTinter.cpp
                   PaletteHandler.cpp
                   widgets/SvgButton.cpp
                   widgets/ElidedComboBox.cpp
                   widgets/PlayAnimationWidget.cpp
                   widgets/ElidedLabel.cpp
                   widgets/TimerButton.cpp
                   widgets/TimerLabel.cpp
                   widgets/VolumeDial.cpp
                   widgets/IconButton.cpp
                   widgets/Spinner.cpp
                   widgets/SessionList.cpp
                   widgets/TrackList.cpp
                   dbus/mpris.cpp
                   dbus/mpris2/Mpris2PluginRootAdaptor.cpp
                   dbus/mpris2/Mpris2PluginPlayerAdaptor.cpp)

# handled by AUTOUIC
set(grooveoff_UI DownloadItem.ui
                 PlayerWidget.ui
                 MainWindow.ui
                 ConfigDialog.ui
                 SettingsItem.ui
                 AboutDialog.ui)

# handled by AUTORCC
set(grooveoff_RCS ${CMAKE_SOURCE_DIR}/grooveoff.qrc)

qt_add_translation(grooveoff_I18N ${TRANSLATIONS})

qt_add_executable(grooveoff ${grooveoff_SRCS}
                            ${grooveoff_UI}
                            ${grooveoff_RCS}
                            ${grooveoff_I18N})

target_link_libraries(grooveoff PRIVATE
                      Qt6::Core
                      Qt6::Gui
                      Qt6::Widgets
                      Qt6::Network
                      Qt6::Sql
                      Qt6::Svg
                      Qt6::DBus
                      Qt6::Multimedia
                      KDAB::kdsingleapplication
                      TagLib::tag
                      grooveoff_core
                      grooveoff_torrent)

install(TARGETS grooveoff RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES ${grooveoff_I18N}
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/apps/grooveoff/translations/)
install(FILES ${CMAKE_SOURCE_DIR}/grooveoff.desktop
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/applications)
install(FILES ${CMAKE_SOURCE_DIR}/resources/default-theme.svg
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/apps/grooveoff/resources)

foreach(size 16 24 32 48 64 72 96 128 256 512)
    install(FILES ${CMAKE_SOURCE_DIR}/resources/grooveoff-${size}.png
            DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/${size}x${size}/apps/
            RENAME grooveoff.png)
endforeach()

install(FILES ${CMAKE_SOURCE_DIR}/resources/grooveoff.svgz
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/scalable/apps/
        RENAME grooveoff.svgz)
