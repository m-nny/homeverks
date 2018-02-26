cmake_minimum_required(VERSION 3.9)

add_executable(simple_shell HW3/simple_shell.c)
set_target_properties(simple_shell PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/HW3/")