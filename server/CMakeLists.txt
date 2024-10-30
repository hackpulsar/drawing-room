cmake_minimum_required(VERSION 3.29)
project(DrawingRoomServer)

set(CMAKE_CXX_STANDARD  20)

file(GLOB_RECURSE SERVER_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

add_executable(${PROJECT_NAME} ${SERVER_SOURCES})

target_include_directories(${PROJECT_NAME}
        PUBLIC
            networking
)

target_link_libraries(${PROJECT_NAME}
        PUBLIC
            DrawingRoomNetworking
)