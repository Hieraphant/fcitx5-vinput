cmake_minimum_required(VERSION 3.16)
project(fcitx5-vinput VERSION 0.1.0 LANGUAGES CXX)

include(GNUInstallDirs)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Fcitx5Core REQUIRED)
find_package(Fcitx5Config REQUIRED)
include("${FCITX_INSTALL_CMAKECONFIG_DIR}/Fcitx5Utils/Fcitx5CompilerSettings.cmake")
find_package(PkgConfig REQUIRED)

pkg_check_modules(PIPEWIRE REQUIRED libpipewire-0.3)
pkg_check_modules(SDBUS REQUIRED libsystemd)
find_package(CURL REQUIRED)

set(VINPUT_SYSTEMD_USER_DIR
    "${CMAKE_INSTALL_DATAROOTDIR}/systemd/user"
    CACHE PATH "Install directory for the vinput systemd --user unit")
set(VINPUT_DBUS_SERVICE_DIR
    "${CMAKE_INSTALL_DATAROOTDIR}/dbus-1/services"
    CACHE PATH "Install directory for the vinput D-Bus activation file")
set(VINPUT_RUNTIME_LIBDIR
    "${CMAKE_INSTALL_LIBDIR}/fcitx5-vinput"
    CACHE PATH "Install directory for bundled sherpa-onnx runtime libraries")
set(VINPUT_DAEMON_EXECUTABLE
    "${CMAKE_INSTALL_FULL_BINDIR}/vinput-daemon")

install(FILES data/vinput-scenes.json
        DESTINATION "${FCITX_INSTALL_PKGDATADIR}/vinput")

add_subdirectory(src/daemon)
add_subdirectory(src/addon)
