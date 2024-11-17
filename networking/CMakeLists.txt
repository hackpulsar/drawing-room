cmake_minimum_required(VERSION 3.29)
project(DrawingRoomNetworking)

set(CMAKE_CXX_STANDARD 20)

find_package(Boost 1.86 REQUIRED)

file(GLOB_RECURSE NETWORKING_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
file(GLOB_RECURSE NETWORKING_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/include/*/*.*")

add_library(${PROJECT_NAME} ${NETWORKING_SOURCES})

target_include_directories(${PROJECT_NAME}
        PUBLIC
            $<INSTALL_INTERFACE:include>
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        PUBLIC
            ${Boost_INCLUDE_DIRS}
)

target_link_libraries(${PROJECT_NAME}
        PUBLIC
            ${Boost_LIBRARIES}
            nlohmann_json::nlohmann_json
)