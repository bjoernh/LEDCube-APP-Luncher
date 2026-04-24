find_package(SDL2 REQUIRED)

add_library(cube_platform_sdl STATIC
    src/sdl_platform.cpp
    src/sdl_display.cpp
    src/sdl_input.cpp
    src/sdl_system_info.cpp
)

target_include_directories(cube_platform_sdl PUBLIC include)

if(TARGET SDL2::SDL2)
    target_link_libraries(cube_platform_sdl PUBLIC cube_shared SDL2::SDL2)
else()
    target_include_directories(cube_platform_sdl PUBLIC ${SDL2_INCLUDE_DIRS})
    target_link_libraries(cube_platform_sdl PUBLIC cube_shared ${SDL2_LIBRARIES})
endif()

if(NOT MSVC)
    target_compile_options(cube_platform_sdl PRIVATE -Wall -Wextra -Wpedantic)
endif()
