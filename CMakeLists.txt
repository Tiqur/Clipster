cmake_minimum_required(VERSION 3.10)

# Set the project name
project(proj)

# Specify the C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find OpenGL
find_package(OpenGL REQUIRED)

# Find GLEW
find_package(GLEW REQUIRED)

# Find GLFW
find_package(glfw3 REQUIRED)

# Include directories for OpenGL, GLEW, GLFW, and ImGui
include_directories(
    ${OPENGL_INCLUDE_DIRS}
    ${GLEW_INCLUDE_DIRS}
    ${GLFW_INCLUDE_DIRS}
    imgui
    lib
)

set(LIB_SRC
    lib/video_manager.cpp
)

# Add ImGui source files
set(IMGUI_SRC
    imgui/imgui.cpp
    imgui/imgui_draw.cpp
    imgui/imgui_tables.cpp
    imgui/imgui_widgets.cpp
    imgui/imgui_demo.cpp
    imgui/imgui_impl_glfw.cpp
    imgui/imgui_impl_opengl3.cpp
)

# Create the executable
add_executable(proj main.cpp ${IMGUI_SRC} ${LIB_SRC})

# Link the necessary libraries
target_link_libraries(proj
    ${OPENGL_LIBRARIES}
    ${GLEW_LIBRARIES}
    glfw
)

