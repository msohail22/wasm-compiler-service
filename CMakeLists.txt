cmake_minimum_required(VERSION 3.16)
project(wasm_compiler_service LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(wasm_compiler_service
    src/main.cpp
    src/db.cpp
    src/exec.cpp
    src/execute_handler.cpp
    src/http_server.cpp
    src/json_utils.cpp
)

target_compile_options(wasm_compiler_service PRIVATE -O2 -Wall -Wextra -Wpedantic)

target_link_libraries(wasm_compiler_service PRIVATE sqlite3 pthread)
