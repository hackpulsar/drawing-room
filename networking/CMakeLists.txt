cmake_minimum_required(VERSION 3.29)
project(DrawingRoomNetworking)

set(CMAKE_CXX_STANDARD 20)

find_package(Boost 1.86 REQUIRED)
find_package(glfw3 3.4 REQUIRED)
find_package(OpenGL 3 REQUIRED)

file(GLOB_RECURSE NETWORKING_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
file(GLOB_RECURSE NETWORKING_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/include/*/*.*")
file(GLOB IMGUI_SOURCES
        "${CMAKE_SOURCE_DIR}/imgui/*.cpp"
        "${CMAKE_SOURCE_DIR}/imgui/backends/imgui_impl_glfw.cpp"
        "${CMAKE_SOURCE_DIR}/imgui/backends/imgui_impl_opengl3.cpp"
        "${CMAKE_SOURCE_DIR}/imgui/misc/cpp/imgui_stdlib.cpp"
)
file(GLOB IMGUI_HEADERS
        "${CMAKE_SOURCE_DIR}/imgui/*.h"
        "${CMAKE_SOURCE_DIR}/imgui/backends/imgui_impl_glfw.h"
        "${CMAKE_SOURCE_DIR}/imgui/backends/imgui_impl_opengl3.h"
        "${CMAKE_SOURCE_DIR}/imgui/backends/imgui_impl_opengl3_loader.h"
        "${CMAKE_SOURCE_DIR}/imgui/misc/cpp/imgui_stdlib.h"
)

add_library(${PROJECT_NAME} ${NETWORKING_SOURCES} ${IMGUI_SOURCES} ${IMGUI_HEADERS})

target_include_directories(${PROJECT_NAME}
        PUBLIC
            $<INSTALL_INTERFACE:include>
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        PUBLIC
            ${Boost_INCLUDE_DIRS}
            "${CMAKE_SOURCE_DIR}/imgui/"
            "${CMAKE_SOURCE_DIR}/imgui/backends"
)

target_link_libraries(${PROJECT_NAME}
        PUBLIC
            ${Boost_LIBRARIES}
            ${OPENGL_LIBRARIES}
            glfw
)