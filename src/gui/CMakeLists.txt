cmake_minimum_required(VERSION 3.16)

find_package(Qt6 COMPONENTS Core Gui Widgets Network QUIET)
if(Qt6_FOUND)
    set(QT_MAJOR 6)
    find_package(Qt6LinguistTools QUIET)
    set(QT_LINGUIST_FOUND ${Qt6LinguistTools_FOUND})
else()
    find_package(Qt5 COMPONENTS Core Gui Widgets Network REQUIRED)
    set(QT_MAJOR 5)
    find_package(Qt5LinguistTools QUIET)
    set(QT_LINGUIST_FOUND ${Qt5LinguistTools_FOUND})
endif()

pkg_check_modules(PIPEWIRE REQUIRED libpipewire-0.3)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(VINPUT_GUI_SOURCES
    app/main.cpp
    app/mainwindow.cpp
    utils/gui_helpers.cpp
    utils/config_manager.cpp
    utils/download_worker.cpp
    utils/i18n_cache.cpp
    dialogs/asr_provider_dialog.cpp
    dialogs/adapter_dialog.cpp
    pages/control/control_page.cpp
    pages/resources/resource_page.cpp
    pages/llm/llm_page.cpp
    pages/hotwords/hotword_page.cpp
    ${CMAKE_SOURCE_DIR}/src/common/audio/pipewire_device.cpp
    ${CMAKE_SOURCE_DIR}/src/cli/runtime/dbus_client.cpp
    ${CMAKE_SOURCE_DIR}/src/cli/runtime/systemd_client.cpp
)

set(VINPUT_GUI_HEADERS
    app/mainwindow.h
    utils/gui_helpers.h
    utils/config_manager.h
    utils/download_worker.h
    utils/i18n_cache.h
    pages/control/control_page.h
    pages/resources/resource_page.h
    pages/llm/llm_page.h
    pages/hotwords/hotword_page.h
)

add_executable(vinput-gui
    ${VINPUT_GUI_SOURCES}
    ${VINPUT_GUI_HEADERS}
)

set(VINPUT_GUI_DESKTOP_FILE
    "${CMAKE_CURRENT_BINARY_DIR}/vinput-gui.desktop")
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/vinput-gui.desktop.in"
    "${VINPUT_GUI_DESKTOP_FILE}"
    @ONLY)

set(VINPUT_GUI_TS
    ${CMAKE_SOURCE_DIR}/i18n/vinput-gui_zh_CN.ts
)

if(QT_LINGUIST_FOUND)
    if(QT_MAJOR EQUAL 6)
        qt_add_translation(VINPUT_GUI_QM ${VINPUT_GUI_TS})
    else()
        qt5_add_translation(VINPUT_GUI_QM ${VINPUT_GUI_TS})
    endif()
else()
    find_program(VINPUT_LRELEASE_EXECUTABLE
        NAMES lrelease-qt6 lrelease lrelease-qt5
    )
    if(VINPUT_LRELEASE_EXECUTABLE)
        set(VINPUT_GUI_QM
            ${CMAKE_CURRENT_BINARY_DIR}/vinput-gui_zh_CN.qm
        )
        add_custom_command(
            OUTPUT ${VINPUT_GUI_QM}
            COMMAND ${VINPUT_LRELEASE_EXECUTABLE}
                ${CMAKE_SOURCE_DIR}/i18n/vinput-gui_zh_CN.ts
                -qm ${VINPUT_GUI_QM}
            DEPENDS ${CMAKE_SOURCE_DIR}/i18n/vinput-gui_zh_CN.ts
            VERBATIM
        )
    endif()
endif()

if(VINPUT_GUI_QM)
    add_custom_target(vinput-gui-translations ALL DEPENDS ${VINPUT_GUI_QM})
endif()

target_include_directories(vinput-gui PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src
    ${PIPEWIRE_INCLUDE_DIRS}
    ${SDBUS_INCLUDE_DIRS}
)

target_link_libraries(vinput-gui
    PRIVATE
    Qt${QT_MAJOR}::Core
    Qt${QT_MAJOR}::Gui
    Qt${QT_MAJOR}::Widgets
    Qt${QT_MAJOR}::Network
    vinput-common
    ${PIPEWIRE_LIBRARIES}
    ${SDBUS_LIBRARIES}
)

install(TARGETS vinput-gui
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(FILES ${VINPUT_GUI_DESKTOP_FILE}
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/applications
)

if(VINPUT_GUI_QM)
    install(FILES ${VINPUT_GUI_QM}
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/fcitx5-vinput/i18n
    )
endif()
