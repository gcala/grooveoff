include_directories(${QT_INCLUDES}
                    ${PHONON_INCLUDES}
                    ${QJSON_INCLUDE_DIR}
                    ${CMAKE_CURRENT_BINARY_DIR}
                    ${CMAKE_CURRENT_SOURCE_DIR})

set(grooveoff_SRCS utility.cpp
                   combobox.cpp
                   filterproxymodel.cpp
                   elidedlabel.cpp
                   aboutdialog.cpp
                   settingsitem.cpp
                   configdialog.cpp
                   timerbutton.cpp
                   songdownloader.cpp
                   song.cpp
                   qled.cpp
                   matchitemlistdelegate.cpp
                   matcheslistmodel.cpp
                   downloaditem.cpp
                   audioplayer.cpp
                   mainwindow.cpp)

set(grooveoff_RCS ${CMAKE_SOURCE_DIR}/grooveoff.qrc)
qt4_add_resources(grooveoff_RC_SRCS ${grooveoff_RCS} )

# use ui files
set(grooveoff_UI downloaditem.ui
                 audioplayer.ui
                 mainwindow.ui
                 configdialog.ui
                 settingsitem.ui
                 aboutdialog.ui)
qt4_wrap_ui(grooveoff_UI_HEADERS ${grooveoff_UI})

qt4_automoc(${grooveoff_SRCS})

qt4_add_translation(grooveoff_I18N ${TRANSLATIONS})

add_executable(grooveoff main.cpp ${grooveoff_SRCS} ${grooveoff_RC_SRCS} ${grooveoff_UI_HEADERS} ${grooveoff_I18N})

target_link_libraries(grooveoff
                      ${QT_QTCORE_LIBRARY}
                      ${QT_QTGUI_LIBRARY}
                      ${QT_QTNETWORK_LIBRARY}
                      ${PHONON_LIBRARY}
                      ${QT_QTSVG_LIBRARY}
                      ${QJSON_LIBRARY})

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
