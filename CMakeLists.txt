cmake_minimum_required(VERSION 3.0.0)
project(gbm-drm-gles-cube VERSION 0.1.0)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=c++11")

add_executable(${PROJECT_NAME}
    main.cpp
)

SUBDIRS(src)

target_link_libraries(${PROJECT_NAME}
    GBM_EGL_LIB
)
