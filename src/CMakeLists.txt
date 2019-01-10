set(CMAKE_CXX_FLAGS "-Wall -Wno-reorder -Wno-sign-compare -std=c++11")

if(TOOLCHAIN_PATH)
    set(DEPENDENCIES_PATH ${TOOLCHAIN_PATH}/dependencies)
endif()

include_directories(
    ${DEPENDENCIES_PATH}/rk3399-libs/include
    ${DEPENDENCIES_PATH}/drm/include
    ${DEPENDENCIES_PATH}/realsense2/include
)

find_library(MALI_LIB Mali
            PATHS ${DEPENDENCIES_PATH}/rk3399-libs/lib64)

find_library(DRM_LIB drm
            PATHS ${DEPENDENCIES_PATH}/drm/lib64)

find_library(REALSENSE2_LIB realsense2
            PATHS ${DEPENDENCIES_PATH}/realsense2/lib64)


add_library(GBM_EGL_LIB STATIC 
    gbm_egl_device_interface.cpp
    gbm_egl_device_impl.cpp
    gbm_egl_util.cpp
    gbm_egl_instance.cpp
)

target_link_libraries(GBM_EGL_LIB
    ${MALI_LIB}
    ${DRM_LIB}
    ${REALSENSE2_LIB}
    pthread
    m
)
