cmake_minimum_required(VERSION 3.16)
project(cube_launcher LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(LV_CONF_PATH "${CMAKE_SOURCE_DIR}/shared/include/lv_conf.h" CACHE STRING "" FORCE)
add_subdirectory(lib/lvgl)

add_subdirectory(shared)
add_subdirectory(platform/sdl)
add_subdirectory(apps/desktop)

option(BUILD_TESTING "Build unit tests" OFF)
if(BUILD_TESTING)
    enable_testing()
    include(FetchContent)
    FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG        v3.7.1
        GIT_SHALLOW    TRUE
    )
    FetchContent_MakeAvailable(Catch2)
    list(APPEND CMAKE_MODULE_PATH "${catch2_SOURCE_DIR}/extras")
    add_subdirectory(tests)
endif()
