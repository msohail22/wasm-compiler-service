cmake_minimum_required(VERSION 3.10)
project(cpp_compiler_service)

set(CMAKE_CXX_STANDARD 17)

add_executable(server
    src/main.cpp
    src/http_server.cpp
    src/compiler.cpp
    src/utils.cpp
)

find_package(Threads REQUIRED)
target_link_libraries(server PRIVATE Threads::Threads)