cmake_minimum_required(VERSION 3.29)
project(DrawingRoom)

set(JSON_BuildTests OFF CACHE INTERNAL "")
add_subdirectory(dependencies/json)

add_subdirectory(gui)
add_subdirectory(networking)
add_subdirectory(server)
add_subdirectory(client)