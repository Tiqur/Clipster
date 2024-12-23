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

# Include directories for OpenGL, GLEW, GLFW, and FFmpeg
include_directories(
    ${OPENGL_INCLUDE_DIRS}
    ${GLEW_INCLUDE_DIRS}
    ${GLFW_INCLUDE_DIRS}
)

# Create the executable
add_executable(proj main.cpp)

# Link the necessary libraries
target_link_libraries(proj
    ${OPENGL_LIBRARIES}
    ${GLEW_LIBRARIES}
    glfw
)

