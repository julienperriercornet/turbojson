
project(testturbojson LANGUAGES CXX)

add_executable(testturbojson test.cpp)

target_link_libraries(testturbojson PRIVATE turbojson)

add_test(NAME test_json_parse_1 COMMAND testturbojson test_json_parse_1)
