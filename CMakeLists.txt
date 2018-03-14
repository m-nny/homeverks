cmake_minimum_required(VERSION 3.9)

add_executable(simple_shell.out HW3/simple_shell.c)
set_target_properties(simple_shell.out PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/HW3/")

add_executable(client.out echoserver/client.c echoserver/connectsock.c)
set_target_properties(client.out PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/echoserver/")

add_executable(echoserver.out echoserver/echoserver.c echoserver/passivesock.c)
set_target_properties(echoserver.out PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/echoserver/")

add_executable(gameserver.out echoserver/gameserver.c echoserver/passivesock.c)
set_target_properties(gameserver.out PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/echoserver/")