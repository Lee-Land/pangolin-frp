cmake_minimum_required(VERSION 3.16)
project(pangolin-frp)

include_directories(include)
include_directories(readini)
include_directories(server)
include_directories(client)

add_subdirectory(src)
add_subdirectory(server)
add_subdirectory(client)
add_subdirectory(readini)

add_executable(pangolin server/server.cpp)
add_executable(pangolin-cli client/client.cpp)

target_link_libraries(pangolin SRC READINI SERVER_LIB)
target_link_libraries(pangolin-cli SRC READINI CLIENT_LIB)
