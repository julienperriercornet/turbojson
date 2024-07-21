project(tjson LANGUAGES CXX)

add_executable(tjson main.cpp)

target_link_libraries(tjson PRIVATE turbojson)
