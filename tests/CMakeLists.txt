add_executable(vinput-model-manager-moonshine-test
    model_manager_moonshine_test.cpp
)

target_include_directories(vinput-model-manager-moonshine-test PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(vinput-model-manager-moonshine-test PRIVATE
    vinput-common
)

add_test(NAME model_manager_moonshine
    COMMAND vinput-model-manager-moonshine-test
)
