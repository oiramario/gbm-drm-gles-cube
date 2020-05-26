#ifndef _gbm_egl_instance_hpp__
#define _gbm_egl_instance_hpp__

#include "gbm_egl_device_impl.hpp"
#include "gbm_egl_util.hpp"
#include <thread>

class gbm_egl_instance : public gbm_egl_device_impl
{
public:
    ~gbm_egl_instance() = default;

private:
    virtual gbm_egl_device_interface* new_instance();
    virtual void begin_impl();
    virtual void end_impl();
    virtual void update_impl();
    virtual void render_impl();

private:
	int u_mvp;

    uint generic_program;
    uint z16_program;
    uint cube_vbo;
    oes_texture color_texture;
    oes_texture depth_texture;

    uint count = 0;
   	ESMatrix color_matrix, depth_matrix;
    ESMatrix projection_matrix;
    ESMatrix mvp_matrix;

    std::thread processing_thread;
};

#endif
