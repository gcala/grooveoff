set(grooveoff_SRCS main.cpp
                   App.cpp
                   ActionCollection.cpp
                   AboutDialog.cpp
                   AudioEngine.cpp
                   ConfigDialog.cpp
                   TrackRow.cpp
                   TrackList.cpp
                   MainWindow.cpp
                   PlayerWidget.cpp
                   Playlist.cpp
                   PlaylistAction.cpp
                   PlaylistItem.cpp
                   SettingsItem.cpp
                   TrackSelector.cpp
                   TrackTags.cpp
                   Utility.cpp
                   SvgHandler.cpp
                   SvgTinter.cpp
                   PaletteHandler.cpp
                   widgets/SvgButton.cpp
                   widgets/PlayAnimationWidget.cpp
                   widgets/ElidedLabel.cpp
                   widgets/TimerButton.cpp
                   widgets/TimerLabel.cpp
                   widgets/VolumeDial.cpp
                   widgets/IconButton.cpp
                   widgets/Spinner.cpp
                   widgets/PlaylistList.cpp
                   widgets/PlaylistDelegate.cpp
                   dbus/mpris.cpp
                   dbus/mpris2/Mpris2PluginRootAdaptor.cpp
                   dbus/mpris2/Mpris2PluginPlayerAdaptor.cpp)

# handled by AUTOUIC
set(grooveoff_UI TrackRow.ui
                 PlayerWidget.ui
                 MainWindow.ui
                 ConfigDialog.ui
                 SettingsItem.ui
                 TrackSelector.ui
                 AboutDialog.ui)

# handled by AUTORCC
set(grooveoff_RCS ${CMAKE_SOURCE_DIR}/grooveoff.qrc)

qt_add_executable(grooveoff ${grooveoff_SRCS}
                            ${grooveoff_UI}
                            ${grooveoff_RCS})

# qt_add_translations rather than the bare qt_add_translation: besides compiling
# the .ts files it generates the update_translations and release_translations
# targets, so refreshing the catalogues is
#
#     cmake --build build-cmake --target update_translations
#
# instead of going to look for lupdate by hand and remembering its arguments.
#
# The .qm files are embedded under :/i18n rather than installed beside the
# binary. What they replace was a path built out of applicationDirPath and a
# guess at the install prefix, which meant an application run from the build
# directory had no translations at all and one installed anywhere unexpected
# quietly had none either.
qt_add_translations(grooveoff
                    TS_FILES ${TRANSLATIONS}
                    LUPDATE_OPTIONS -locations relative)

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
