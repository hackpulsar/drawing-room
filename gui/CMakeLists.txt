cmake_minimum_required(VERSION 3.29)
project(DrawingRoomGUI)

set(CMAKE_CXX_STANDARD 20)

find_package(glfw3 3.4 REQUIRED)
find_package(OpenGL 3 REQUIRED)

file(GLOB_RECURSE GUI_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
file(GLOB_RECURSE GUI_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/include/*/*.*")
file(GLOB IMGUI_SOURCES
        "${CMAKE_SOURCE_DIR}/dependencies/imgui/*.cpp"
        "${CMAKE_SOURCE_DIR}/dependencies/imgui/backends/imgui_impl_glfw.cpp"
        "${CMAKE_SOURCE_DIR}/dependencies/imgui/backends/imgui_impl_opengl3.cpp"
        "${CMAKE_SOURCE_DIR}/dependencies/imgui/misc/cpp/imgui_stdlib.cpp"
)
file(GLOB IMGUI_HEADERS
        "${CMAKE_SOURCE_DIR}/dependencies/imgui/*.h"
        "${CMAKE_SOURCE_DIR}/dependencies/imgui/backends/imgui_impl_glfw.h"
        "${CMAKE_SOURCE_DIR}/dependencies/imgui/backends/imgui_impl_opengl3.h"
        "${CMAKE_SOURCE_DIR}/dependencies/imgui/backends/imgui_impl_opengl3_loader.h"
        "${CMAKE_SOURCE_DIR}/dependencies/imgui/misc/cpp/imgui_stdlib.h"
)

add_library(${PROJECT_NAME} ${GUI_SOURCES} ${IMGUI_SOURCES} ${IMGUI_HEADERS})

target_include_directories(${PROJECT_NAME}
        PUBLIC
            $<INSTALL_INTERFACE:include>
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        PUBLIC
            "${CMAKE_SOURCE_DIR}/dependencies/imgui/"
            "${CMAKE_SOURCE_DIR}/dependencies/imgui/backends"
)

target_link_libraries(${PROJECT_NAME}
        PUBLIC
            ${OPENGL_LIBRARIES}
            glfw
)