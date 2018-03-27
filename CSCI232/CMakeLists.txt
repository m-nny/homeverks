cmake_minimum_required(VERSION 3.9)

add_executable(client.out HW4/client.c HW4/connectsock.c)
set_target_properties(client.out PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/HW4/")
add_executable(gameserver.out HW4/gameserver.c HW4/passivesock.c)
set_target_properties(gameserver.out PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/HW4/")