set(CMAKE_CXX_FLAGS "-Wall -Wno-reorder -Wno-sign-compare -std=c++11")

find_package(PkgConfig REQUIRED)
pkg_check_modules(DRM REQUIRED libdrm)
pkg_check_modules(MALI REQUIRED mali)
pkg_check_modules(REALSENSE2 REQUIRED realsense2)

include_directories(
    ${DRM_INCLUDE_DIRS}
    ${MALI_INCLUDE_DIRS}
    ${REALSENSE2_INCLUDE_DIRS}
)

add_library(GBM_EGL_LIB STATIC 
    gbm_egl_device_interface.cpp
    gbm_egl_device_impl.cpp
    gbm_egl_util.cpp
    gbm_egl_instance.cpp
)

target_link_libraries(GBM_EGL_LIB
    ${DRM_LIBRARIES}
    ${MALI_LIBRARIES}
    ${REALSENSE2_LIBRARIES}
    pthread
    m
)
