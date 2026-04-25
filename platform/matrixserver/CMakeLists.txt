add_library(cube_platform_matrixserver STATIC
    src/matrixserver_platform.cpp
    src/matrixserver_display.cpp
    src/matrixserver_input.cpp
    src/matrixserver_system_info.cpp
)

target_include_directories(cube_platform_matrixserver PUBLIC include)

target_link_libraries(cube_platform_matrixserver
    PUBLIC
        cube_shared
        lvgl
        matrixapplication
)

target_compile_features(cube_platform_matrixserver PUBLIC cxx_std_20)

if(NOT MSVC)
    target_compile_options(cube_platform_matrixserver PRIVATE -Wall -Wextra -Wpedantic)
endif()
