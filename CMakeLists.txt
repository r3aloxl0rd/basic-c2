cmake_minimum_required(VERSION 3.20)
project(C2)
set(CMAKE_CXX_STANDARD 17)

add_executable(server
    server/main.cpp
    shared/utils.cpp
    shared/crypto.cpp
)

add_executable(client
    client/main.cpp
    shared/utils.cpp
    shared/crypto.cpp
)

find_package(Threads REQUIRED)
target_link_libraries(server PRIVATE Threads::Threads)
target_link_libraries(client PRIVATE Threads::Threads)

target_include_directories(server PRIVATE ${CMAKE_SOURCE_DIR}/shared)
target_include_directories(client PRIVATE ${CMAKE_SOURCE_DIR}/shared)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
