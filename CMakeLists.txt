cmake_minimum_required(VERSION 3.16)
project(pangolin-frp)

include_directories(include)
include_directories(readini)

add_subdirectory(src)
add_subdirectory(readini)

add_executable(pangolin src/main/server.cpp)
add_executable(pangolin-cli src/main/client.cpp)
add_executable(mytest test/test.cpp)

target_link_libraries(pangolin SRC READINI )
target_link_libraries(pangolin-cli SRC READINI )
target_link_libraries(mytest SRC READINI)