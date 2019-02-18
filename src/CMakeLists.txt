set(CMAKE_CXX_FLAGS "-Wall -Wno-reorder -Wno-sign-compare -std=c++11")

if(TOOLCHAIN_PATH)
    set(DEPENDENCIES_PATH ${TOOLCHAIN_PATH}/dependencies)
endif()

find_package(libdrm REQUIRED)
#find_package(libmali REQUIRED)
find_package(realsense2 REQUIRED)

include_directories(
    ${LIBDRM_INCLUDE_DIRS}
#    ${LIBMALI_INCLUDE_DIRS}
    ${LIBREALSENSE2_INCLUDE_DIRS}
)

add_library(GBM_EGL_LIB STATIC 
    gbm_egl_device_interface.cpp
    gbm_egl_device_impl.cpp
    gbm_egl_util.cpp
    gbm_egl_instance.cpp
)

target_link_libraries(GBM_EGL_LIB
    ${LIBDRM_LIBRARIES}
#    ${LIBMALI_LIBRARIES}
    ${LIBREALSENSE_LIBRARIES}
    pthread
    m
)
