find_library(SHERPA_ONNX_CXX_LIB sherpa-onnx-cxx-api REQUIRED)
find_library(SHERPA_ONNX_C_LIB sherpa-onnx-c-api REQUIRED)
find_library(ONNXRUNTIME_LIB onnxruntime REQUIRED)
find_path(SHERPA_ONNX_INCLUDE_DIR sherpa-onnx/c-api/c-api.h REQUIRED)
set(VINPUT_ENABLE_VOSK FALSE)
unset(VOSK_LIB CACHE)
unset(VOSK_INCLUDE_DIR CACHE)

if(VINPUT_VOSK_RUNTIME_MODE STREQUAL "disabled")
    message(STATUS "Vosk runtime disabled by VINPUT_VOSK_RUNTIME_MODE")
elseif(VINPUT_VOSK_RUNTIME_MODE STREQUAL "system")
    find_library(VOSK_LIB vosk REQUIRED)
    find_path(VOSK_INCLUDE_DIR vosk_api.h REQUIRED)
    set(VINPUT_ENABLE_VOSK TRUE)
elseif(VINPUT_VOSK_RUNTIME_MODE STREQUAL "bundled")
    find_library(VOSK_LIB vosk REQUIRED)
    find_path(VOSK_INCLUDE_DIR vosk_api.h REQUIRED)
    set(VINPUT_ENABLE_VOSK TRUE)
else()
    find_library(VOSK_LIB vosk)
    find_path(VOSK_INCLUDE_DIR vosk_api.h)
    if(VOSK_LIB AND VOSK_INCLUDE_DIR)
        set(VINPUT_ENABLE_VOSK TRUE)
    endif()
endif()

pkg_check_modules(PIPEWIRE REQUIRED libpipewire-0.3)
set(VINPUT_SYSTEMD_SERVICE_FILE
    "${CMAKE_CURRENT_BINARY_DIR}/vinput-daemon.service")
set(VINPUT_DBUS_SERVICE_FILE
    "${CMAKE_CURRENT_BINARY_DIR}/org.fcitx.Vinput.service")

get_filename_component(SHERPA_ONNX_RUNTIME_DIR "${SHERPA_ONNX_CXX_LIB}" DIRECTORY)
if(VINPUT_VOSK_RUNTIME_MODE STREQUAL "bundled" AND VOSK_LIB)
    get_filename_component(VOSK_RUNTIME_DIR "${VOSK_LIB}" DIRECTORY)
    file(GLOB VINPUT_BUNDLED_VOSK_PRIVATE_LIBS
        "${VOSK_RUNTIME_DIR}/libkaldi*.so*"
        "${VOSK_RUNTIME_DIR}/libfst*.so*"
    )
endif()
if(VINPUT_RUNTIME_MODE STREQUAL "bundled")
    if(IS_ABSOLUTE "${VINPUT_RUNTIME_LIBDIR}")
        set(VINPUT_DAEMON_INSTALL_RPATH "${VINPUT_RUNTIME_LIBDIR}")
    else()
        set(VINPUT_DAEMON_INSTALL_RPATH "\$ORIGIN/../${VINPUT_RUNTIME_LIBDIR}")
    endif()
endif()

set(VINPUT_VAD_MODEL_DIR "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_DATADIR}/fcitx5-vinput/vad")

add_executable(vinput-daemon
    main.cpp
    asr/runtime/backend_factory.cpp
    asr/runtime/recognition_session_manager.cpp
    asr/backends/sherpa_offline_backend.cpp
    asr/backends/sherpa_streaming_backend.cpp
    asr/backends/command_batch_backend.cpp
    asr/backends/command_streaming_backend.cpp
    audio/audio_capture.cpp
    audio/audio_utils.cpp
    asr/vad_trimmer.cpp
    runtime/dbus_service.cpp
    runtime/daemon_runtime_controller.cpp
    runtime/recognition_pipeline.cpp
    postprocess/post_processor.cpp
)

if(VINPUT_ENABLE_VOSK)
    target_sources(vinput-daemon PRIVATE
        asr/backends/vosk_offline_backend.cpp
        asr/backends/vosk_streaming_backend.cpp
    )
    target_compile_definitions(vinput-daemon PRIVATE
        VINPUT_HAVE_VOSK=1
    )

    include(CheckSymbolExists)
    set(CMAKE_REQUIRED_INCLUDES "${VOSK_INCLUDE_DIR}")
    check_symbol_exists(VOSK_EP_ANSWER_DEFAULT "vosk_api.h" VOSK_HAS_ENDPOINTER)
    if(VOSK_HAS_ENDPOINTER)
        target_compile_definitions(vinput-daemon PRIVATE
            VINPUT_VOSK_HAS_ENDPOINTER=1
        )
    endif()
endif()

target_compile_definitions(vinput-daemon PRIVATE
    VINPUT_VAD_MODEL_PATH="${VINPUT_VAD_MODEL_DIR}/silero_vad.onnx"
)

target_include_directories(vinput-daemon PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${PIPEWIRE_INCLUDE_DIRS}
    ${SHERPA_ONNX_INCLUDE_DIR}
)

if(VINPUT_ENABLE_VOSK)
    target_include_directories(vinput-daemon PRIVATE
        ${VOSK_INCLUDE_DIR}
    )
endif()

target_link_libraries(vinput-daemon PRIVATE
    vinput-common
    ${PIPEWIRE_LIBRARIES}
    ${SDBUS_LIBRARIES}
    CURL::libcurl
    ${SHERPA_ONNX_CXX_LIB}
    ${SHERPA_ONNX_C_LIB}
    pthread
)

if(VINPUT_ENABLE_VOSK)
    target_link_libraries(vinput-daemon PRIVATE
        ${VOSK_LIB}
    )
    if(VINPUT_VOSK_RUNTIME_MODE STREQUAL "bundled" AND
       VINPUT_BUNDLED_VOSK_PRIVATE_LIBS)
        target_link_libraries(vinput-daemon PRIVATE
            ${VINPUT_BUNDLED_VOSK_PRIVATE_LIBS}
        )
    endif()
endif()

set_target_properties(vinput-daemon PROPERTIES
    BUILD_RPATH "${SHERPA_ONNX_RUNTIME_DIR}"
)
if(VINPUT_RUNTIME_MODE STREQUAL "bundled")
    set_target_properties(vinput-daemon PROPERTIES
        INSTALL_RPATH "${VINPUT_DAEMON_INSTALL_RPATH}"
    )
endif()

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
if(VINPUT_RUNTIME_MODE STREQUAL "bundled")
    install(FILES
        "${SHERPA_ONNX_CXX_LIB}"
        "${SHERPA_ONNX_C_LIB}"
        "${ONNXRUNTIME_LIB}"
        DESTINATION "${VINPUT_RUNTIME_LIBDIR}"
    )
    if(VINPUT_VOSK_RUNTIME_MODE STREQUAL "bundled")
        install(FILES
            "${VOSK_LIB}"
            DESTINATION "${VINPUT_RUNTIME_LIBDIR}"
        )
        if(VINPUT_BUNDLED_VOSK_PRIVATE_LIBS)
            install(FILES
                ${VINPUT_BUNDLED_VOSK_PRIVATE_LIBS}
                DESTINATION "${VINPUT_RUNTIME_LIBDIR}"
            )
        endif()
    endif()
endif()
# VAD model (optional — daemon gracefully degrades if missing)
if(EXISTS "${CMAKE_SOURCE_DIR}/data/vad/silero_vad.onnx")
    install(FILES "${CMAKE_SOURCE_DIR}/data/vad/silero_vad.onnx"
            DESTINATION "${VINPUT_VAD_MODEL_DIR}")
endif()
