add_executable(cube_tests
    test_normalize_hostname.cpp
    test_hsv_to_color.cpp
)

target_link_libraries(cube_tests PRIVATE
    cube_shared
    Catch2::Catch2WithMain
)

include(Catch)
catch_discover_tests(cube_tests)
