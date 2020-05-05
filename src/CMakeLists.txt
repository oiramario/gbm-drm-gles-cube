set(CMAKE_CXX_FLAGS "-Wall")

find_package(PkgConfig REQUIRED)
pkg_check_modules(DRM REQUIRED libdrm)
pkg_check_modules(MALI REQUIRED mali)
#pkg_check_modules(REALSENSE2 REQUIRED realsense2)
#pkg_check_modules(USB REQUIRED libusb-1.0)

include_directories(
    ${DRM_INCLUDE_DIRS}
    ${MALI_INCLUDE_DIRS}
#    ${REALSENSE2_INCLUDE_DIRS}
)

link_directories(
    ${DRM_LIBRARY_DIRS}
    ${MALI_LIBRARY_DIRS}
#    ${REALSENSE2_LIBRARY_DIRS}
#    ${USB_LIBRARY_DIRS}
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
#    ${REALSENSE2_LIBRARIES}
#    ${USB_LIBRARIES}
    pthread
    m
)
