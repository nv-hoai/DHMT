cmake_minimum_required(VERSION 3.10)
project(DelaunayVisualizer)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find OpenGL and GLUT packages
find_package(OpenGL REQUIRED)
find_package(GLUT REQUIRED)

# Include directories
include_directories(${OPENGL_INCLUDE_DIRS} ${GLUT_INCLUDE_DIRS})

# Add source files
set(SOURCES
    main.cpp
)

# Add executable
add_executable(delaunay_visualizer ${SOURCES})

# Link libraries
target_link_libraries(delaunay_visualizer ${OPENGL_LIBRARIES} ${GLUT_LIBRARIES})

# Create sample points file
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/sample_points.txt 
    "# Sample 3D points (x y z)
    -0.5 -0.5 0.1
    -0.5  0.5 0.2
    0.5  0.5 0.3
    0.5 -0.5 0.4
    0.0  0.0 0.5
    -0.3  0.2 0.6
    0.2  0.3 0.7
    -0.2 -0.4 0.2
    0.4 -0.2 0.1
    0.1  0.4 0.3
    -0.4 -0.1 0.2
    0.3 -0.3 0.4")