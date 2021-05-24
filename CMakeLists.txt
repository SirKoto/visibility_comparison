cmake_minimum_required(VERSION 3.10)

project (visibility)

if("${CMAKE_BUILD_TYPE}" STREQUAL "Debug") # icc
    set(CMAKE_CXX_FLAGS "-O0 -g -std=c++17")
else()
    set(CMAKE_CXX_FLAGS "-O3 -std=c++17")
endif()

set(SRC_FILES 
    src/main.cpp
    src/Mesh.cpp    src/Mesh.hpp
    src/Args.cpp    src/Args.hpp
    src/testAABBoxInFrustum.h
    src/glad.c
)

set(COPY_DATA
    models/cube.ply
    models/Armadillo.ply
    shaders/norm.vert   shaders/norm.frag
)



include_directories("include/")


if (POLICY CMP0072)
    cmake_policy (SET CMP0072 OLD)
endif(POLICY CMP0072)

find_package(OpenGL REQUIRED)
if(OPENGL_FOUND)
    include_directories(${OPENGL_INCLUDE_DIR})
    link_libraries(${OPENGL_LIBRARIES} ${CMAKE_DL_LIBS})
endif()

find_package(glfw3 3.3 REQUIRED)
link_libraries(glfw)

find_package(glm REQUIRED)
include_directories(${GLM_INCLUDE_DIRS})
link_directories(${GLM_LIBRARY_DIRS})
add_definitions(${GLM_DEFINITIONS})
add_executable(visibility ${SRC_FILES})

foreach(data ${COPY_DATA})
    configure_file(${data} ${data} COPYONLY)
endforeach()


