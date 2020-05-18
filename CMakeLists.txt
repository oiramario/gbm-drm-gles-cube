cmake_minimum_required(VERSION 3.0.0)
project(gbm-drm-gles-cube VERSION 0.1.0)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")

set(ENV{PKG_CONFIG_PATH} /opt/devkit/lib/pkgconfig)
link_directories(/opt/devkit/lib)

SET(CMAKE_SYSTEM_NAME Linux)
SET(CMAKE_SYSTEM_PROCESSOR aarch64)

SET(CMAKE_C_COMPILER    aarch64-linux-gnu-gcc)
SET(CMAKE_CXX_COMPILER  aarch64-linux-gnu-g++)

SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

add_executable(${PROJECT_NAME}
    main.cpp
)

SUBDIRS(src)

target_link_libraries(${PROJECT_NAME}
    GBM_EGL_LIB
)
