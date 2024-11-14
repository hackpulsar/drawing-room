cmake_minimum_required(VERSION 3.29)
project(DrawingRoom)

set(JSON_BuildTests OFF CACHE INTERNAL "")
add_subdirectory(json)

add_subdirectory(networking)
add_subdirectory(server)
add_subdirectory(client)