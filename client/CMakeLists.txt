cmake_minimum_required(VERSION 3.29)
project(DrawingRoomClient)

set(CMAKE_CXX_STANDARD  20)

file(GLOB_RECURSE CLIENT_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
file(GLOB_RECURSE CLIENT_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h")

add_executable(${PROJECT_NAME} ${CLIENT_SOURCES} ${CLIENT_HEADERS})

target_include_directories(${PROJECT_NAME}
        PUBLIC
            networking
)

target_link_libraries(${PROJECT_NAME}
        PUBLIC
            DrawingRoomNetworking
)