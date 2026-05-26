cmake_minimum_required(VERSION 3.20)
project(C2)

set(CMAKE_CXX_STANDARD 23)

add_executable(server server/main.cpp)
add_executable(client client/main.cpp)
