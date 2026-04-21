cmake_minimum_required(VERSION 3.10)
project(ICPC_Management_System)

set(CMAKE_CXX_STANDARD 17)

add_executable(code code.cpp)

set_target_properties(code PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
set_target_properties(code PROPERTIES RUNTIME_OUTPUT_NAME "code")
