#ifndef _gbm_egl_device_impl_hpp__
#define _gbm_egl_device_impl_hpp__

#include <xf86drm.h>
#include <xf86drmMode.h>
#include <gbm.h>
#include <EGL/egl.h>
#include <atomic>
#include "gbm_egl_device_interface.hpp"

class gbm_egl_device_impl : public gbm_egl_device_interface
{
protected:
    uint16_t get_resolution_width();
    uint16_t get_resolution_height();

    struct oes_texture
    {
        struct gbm_bo* bo = nullptr;
        void* image = nullptr;
        uint32_t id = 0;
        __u64 offset = 0;
        void* dma = nullptr;
    };
    bool create_texture(int width, int height, oes_texture& out_texture);
    void destroy_texture(oes_texture& texture);
    bool update_texture(oes_texture& texture, const void* data);

    virtual ~gbm_egl_device_impl();

    static std::atomic_bool running;

private:
    virtual void begin_impl() = 0;
    virtual void end_impl() = 0;
    virtual void update_impl() = 0;
    virtual void render_impl() = 0;

    virtual bool create_impl(uint16_t resolution_w, uint16_t resolution_h);
    virtual void main_loop_impl();

    struct drm_info
    {
        int fd = -1;
        uint32_t connector_id = 0;
        uint32_t crtc_id = 0;
        drmModeModeInfo mode = { 0 };
    };
    bool init_drm(uint16_t resolution_w, uint16_t resolution_h);

    struct gbm_info
    {
        struct gbm_device* dev = nullptr;
        struct gbm_surface* surface = nullptr;
    };
    bool init_gbm();

    struct egl_info
    {
        EGLDisplay display = nullptr;
        EGLConfig config = nullptr;
        EGLContext context = nullptr;
        EGLSurface surface = nullptr;
    };
    bool init_gl();

    struct drm_fb
    {
        int fd = -1;
        struct gbm_bo* bo = nullptr;
        uint32_t fb_id = 0;
    };
    drm_fb* drm_fb_get_from_bo(gbm_bo* bo);

    drm_info drm;
    gbm_info gbm;
    egl_info gl;
};

#endif
