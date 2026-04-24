add_library(cube_shared STATIC
    src/launcher.cpp
    src/hue_focus.cpp
    src/mock_app.cpp
    src/tick_bridge.cpp
    src/lv_font_cube_6px.c
    src/hostname_utils.cpp
    src/hue_utils.cpp
    src/settings.cpp
    src/startup_anim.cpp
    src/startup_anim_phosphor.cpp
)

target_include_directories(cube_shared PUBLIC include)
target_link_libraries(cube_shared PUBLIC lvgl)

if(NOT MSVC)
    target_compile_options(cube_shared PRIVATE -Wall -Wextra -Wpedantic)
endif()
