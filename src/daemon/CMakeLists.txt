find_library(SHERPA_ONNX_CXX_LIB sherpa-onnx-cxx-api REQUIRED)
find_library(SHERPA_ONNX_C_LIB sherpa-onnx-c-api REQUIRED)
find_path(SHERPA_ONNX_INCLUDE_DIR sherpa-onnx/c-api/c-api.h REQUIRED)

pkg_check_modules(PIPEWIRE REQUIRED libpipewire-0.3)
set(VINPUT_SYSTEMD_SERVICE_FILE
    "${CMAKE_CURRENT_BINARY_DIR}/vinput-daemon.service")
set(VINPUT_DBUS_SERVICE_FILE
    "${CMAKE_CURRENT_BINARY_DIR}/org.fcitx.Vinput.service")

set(VINPUT_VAD_MODEL_DIR "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_DATADIR}/fcitx5-vinput/vad")

add_executable(vinput-daemon
    main.cpp
    audio_capture.cpp
    asr_engine.cpp
    audio_utils.cpp
    vad_trimmer.cpp
    dbus_service.cpp
    post_processor.cpp
)

target_compile_definitions(vinput-daemon PRIVATE
    VINPUT_VAD_MODEL_PATH="${VINPUT_VAD_MODEL_DIR}/silero_vad.onnx"
)

target_include_directories(vinput-daemon PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${PIPEWIRE_INCLUDE_DIRS}
    ${SHERPA_ONNX_INCLUDE_DIR}
)

target_link_libraries(vinput-daemon PRIVATE
    vinput-common
    ${PIPEWIRE_LIBRARIES}
    ${SDBUS_LIBRARIES}
    CURL::libcurl
    ${SHERPA_ONNX_CXX_LIB}
    ${SHERPA_ONNX_C_LIB}
    pthread
)

configure_file("${CMAKE_SOURCE_DIR}/systemd/vinput-daemon.service.in"
               "${VINPUT_SYSTEMD_SERVICE_FILE}"
               @ONLY)
configure_file("${CMAKE_SOURCE_DIR}/data/org.fcitx.Vinput.service.in"
               "${VINPUT_DBUS_SERVICE_FILE}"
               @ONLY)

install(TARGETS vinput-daemon DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES
    "${VINPUT_SYSTEMD_SERVICE_FILE}"
    DESTINATION "${VINPUT_SYSTEMD_USER_DIR}"
)
install(FILES
    "${VINPUT_DBUS_SERVICE_FILE}"
    DESTINATION "${VINPUT_DBUS_SERVICE_DIR}"
)
# VAD model (optional — daemon gracefully degrades if missing)
if(EXISTS "${CMAKE_SOURCE_DIR}/data/vad/silero_vad.onnx")
    install(FILES "${CMAKE_SOURCE_DIR}/data/vad/silero_vad.onnx"
            DESTINATION "${VINPUT_VAD_MODEL_DIR}")
endif()
