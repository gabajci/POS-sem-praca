cmake_minimum_required(VERSION 3.18)
project(semkaPong C)

set(CMAKE_C_STANDARD 99)

add_executable(client client.c)
add_executable(server server.c)
target_link_libraries(client pthread ncurses)
target_link_libraries(server pthread ncurses)
