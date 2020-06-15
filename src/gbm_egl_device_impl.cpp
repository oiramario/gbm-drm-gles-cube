#include "gbm_egl_device_impl.hpp"
#include <fcntl.h>
#include <string.h>
#include <iostream>
#include <signal.h>
#include <sys/mman.h>
#define EGL_EGLEXT_PROTOTYPES
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#define GL_GLEXT_PROTOTYPES
#include <GLES2/gl2ext.h>
#include <drm_fourcc.h>

std::atomic_bool gbm_egl_device_impl::running {true};

uint16_t gbm_egl_device_impl::get_resolution_width()
{
    return drm.mode->hdisplay;
}


uint16_t gbm_egl_device_impl::get_resolution_height()
{
    return drm.mode->vdisplay;
}


bool gbm_egl_device_impl::create_texture(int width, int height, oes_texture& out_texture, TextureFormat format)
{
    bool ret = false;
    int32_t bpp = 0;
    EGLint egl_format = 0;
    uint32_t gbm_format = 0;
    switch (format)
    {
        case TextureFormat::Depth16:
            egl_format = DRM_FORMAT_RGB565;
            gbm_format = GBM_FORMAT_RGB565;
            bpp = 2;
            break;

        case TextureFormat::YUYV:
            egl_format = DRM_FORMAT_YUYV;
            gbm_format = GBM_FORMAT_YUYV;
            bpp = 2;
            break;

        case TextureFormat::RGB8:
            egl_format = DRM_FORMAT_RGB888;
            gbm_format = GBM_FORMAT_RGB888;
            bpp = 3;
            break;

        default:
            return false;
            break;
    }

    gbm_bo* bo = gbm_bo_create(gbm.dev, width, height, gbm_format, GBM_BO_USE_RENDERING | GBM_BO_USE_SCANOUT);
    if (bo)
    {
        const int fd = gbm_bo_get_fd(bo);
        const EGLint khr_image_attrs[] = {EGL_DMA_BUF_PLANE0_FD_EXT, fd,
                                          EGL_WIDTH, width,
                                          EGL_HEIGHT, height,
                                          EGL_LINUX_DRM_FOURCC_EXT, egl_format,
                                          EGL_DMA_BUF_PLANE0_PITCH_EXT, width * bpp,
                                          EGL_DMA_BUF_PLANE0_OFFSET_EXT, 0,
                                          EGL_NONE};

        EGLImageKHR eglImage = eglCreateImageKHR(gl.display, EGL_NO_CONTEXT, EGL_LINUX_DMA_BUF_EXT, nullptr, khr_image_attrs);
        if (eglImage != EGL_NO_IMAGE_KHR)
        {
            GLuint glTexture = 0;
            glGenTextures(1, &glTexture);
            glBindTexture(GL_TEXTURE_EXTERNAL_OES, glTexture);
            glEGLImageTargetTexture2DOES(GL_TEXTURE_EXTERNAL_OES, eglImage);

            struct drm_mode_map_dumb arg = {0};
            arg.handle = gbm_bo_get_handle(bo).u32;
            if (drmIoctl(drm.fd, DRM_IOCTL_MODE_MAP_DUMB, &arg) == 0)
            {
                int size = width * height * bpp;
                void* address = mmap(0, size, PROT_WRITE, MAP_SHARED, drm.fd, arg.offset);
                if (address != MAP_FAILED)
                {
                    out_texture.format = format;
                    out_texture.bpp = bpp;
                    out_texture.bo = bo;
                    out_texture.image = eglImage;
                    out_texture.id = glTexture;
                    out_texture.offset = arg.offset;
                    out_texture.dma = address;
                    ret = true;
                }
            }
            else
            {
                eglDestroyImageKHR(gl.display, eglImage);
                fprintf(stderr, "failed to map dma buffer.\n");
            }
        }
        else
        {
            fprintf(stderr, "failed to make image from buffer object.\n");
            gbm_bo_destroy(bo);
        }
    }
    else
    {
        fprintf(stderr, "failed to create a gbm buffer.\n");
    }

    return ret;
}


void gbm_egl_device_impl::destroy_texture(oes_texture& texture)
{
    if (texture.id)
        glDeleteTextures(1, &texture.id), texture.id = 0;

    if (texture.image)
        eglDestroyImageKHR(gl.display, texture.image), texture.image = nullptr;

    if (texture.dma && texture.bo)
    {
        const uint32_t stride = gbm_bo_get_stride(texture.bo);
        const uint32_t height = gbm_bo_get_height(texture.bo);
        munmap(texture.dma, stride * height);
        texture.dma = nullptr;
    }

    if (texture.bo)
        gbm_bo_destroy(texture.bo), texture.bo = nullptr;
}


bool gbm_egl_device_impl::update_texture(oes_texture& texture, const void* data)
{
#if 1
    // mmap fast
    if (data && texture.bo)
    {
        const uint32_t height = gbm_bo_get_height(texture.bo);
        const uint32_t stride = gbm_bo_get_stride(texture.bo);
        int size = stride * height;
        if (texture.format == TextureFormat::YUYV)
            size /= 2;
        void* address = mmap(0, size, PROT_WRITE, MAP_SHARED, drm.fd, texture.offset);
        if (address != MAP_FAILED)
        {
            memcpy(address, data, size);
            munmap(address, size);
            return true;
        }
    }
#else
    // memcpy slow
    if (data && texture.dma && texture.bo)
    {
        const uint32_t height = gbm_bo_get_height(texture.bo);
        const uint32_t width = gbm_bo_get_width(texture.bo);
        const uint32_t bpp = 2;
        const uint32_t size = width * height * bpp;
        memcpy(texture.dma, data, size);

        return true;
    }
#endif
    return false;
}


gbm_egl_device_impl::~gbm_egl_device_impl()
{
    if (drm.crtc)
    {
        drmModeSetCrtc(drm.fd, drm.crtc->crtc_id, drm.crtc->buffer_id,
                       drm.crtc->x, drm.crtc->y, &drm.connector->connector_id,
                                    1, &drm.crtc->mode);
        drmModeFreeCrtc(drm.crtc);
    }

    if (gl.display)
    {
        if (gl.surface)
	        eglDestroySurface(gl.display, gl.surface);
        if (gl.context)
            eglDestroyContext(gl.display, gl.context);
        eglTerminate(gl.display);
    }

    if (gbm.surface)
        gbm_surface_destroy(gbm.surface);
    if (gbm.dev)
        gbm_device_destroy(gbm.dev);

    if (drm.connector)
        drmModeFreeConnector(drm.connector);
    if (drm.encoder)
        drmModeFreeEncoder(drm.encoder);
    if (drm.resources)
        drmModeFreeResources(drm.resources);
    if (drm.fd > 0)
        drmClose(drm.fd);
}


bool gbm_egl_device_impl::create_impl(uint16_t resolution_w, uint16_t resolution_h)
{
    return init_drm(resolution_w, resolution_h) && 
           init_gbm() && 
           init_gl();
}


void gbm_egl_device_impl::main_loop_impl()
{
    std::cout << "main_loop_impl" << std::endl;
    eglSwapBuffers(gl.display, gl.surface);

    gbm_bo* bo = gbm_surface_lock_front_buffer(gbm.surface);
    drm_fb* fb = drm_fb_get_from_bo(bo);

    // handle Ctrl+C
    signal(SIGINT, [](int){ running = false; });

    // set mode:
    std::cout << "drmModeSetCrtc" << std::endl;
    if (!drmModeSetCrtc(drm.fd, drm.encoder->crtc_id, fb->fb_id, 
        0, 0, &drm.connector->connector_id, 1, drm.mode))
    {
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(0, &fds);
        FD_SET(drm.fd, &fds);

        drmEventContext evctx = { DRM_EVENT_CONTEXT_VERSION, 0, 
                                  [](int fd, unsigned int frame, unsigned int sec, unsigned int usec, void* data){
                                      int* waiting_for_flip = (int*)data;
                                      *waiting_for_flip = 0;
                                }};
        begin_impl();

        bool local_working = true;
        while (local_working && running)
        {
            update_impl();
            render_impl();

            eglSwapBuffers(gl.display, gl.surface);
            gbm_bo* next_bo = gbm_surface_lock_front_buffer(gbm.surface);
            fb = drm_fb_get_from_bo(next_bo);

            // Here you could also update drm plane layers if you want hw composition

            int waiting_for_flip = 1;
            if (drmModePageFlip(drm.fd, drm.encoder->crtc_id, fb->fb_id, 
                DRM_MODE_PAGE_FLIP_EVENT, &waiting_for_flip))
            {
                std::cerr << "failed to queue page flip: " << strerror(errno) << std::endl;
                break;
            }

            while (waiting_for_flip)
            {
                int ret = select(drm.fd + 1, &fds, nullptr, nullptr, nullptr);
                if (ret < 0)
                {
                    std::cerr << "select err: " << strerror(errno) << std::endl;
                    running = false;
                    break;
                }
                else if (ret == 0)
                {
                    std::cerr << "select timeout!" << std::endl;
                    running = false;
                    break;
                }
                else if (FD_ISSET(0, &fds))
                {
                    std::cout << "user interrupted!" << std::endl;
                    running = false;
                    break;
                }
                else
                {
                    drmHandleEvent(drm.fd, &evctx);
                }
            }

            // release last buffer to render on again:
            gbm_surface_release_buffer(gbm.surface, bo);
            bo = next_bo;
        }

        end_impl();
    }
    else
    {
        std::cerr << "failed to set mode: " << strerror(errno) << std::endl;
    }
}


bool gbm_egl_device_impl::init_drm(uint16_t resolution_w, uint16_t resolution_h)
{
    bool ret = false;
	int fd = open("/dev/dri/card0", O_RDWR);
	if (fd >= 0)
    {
        drmModeRes* resources = drmModeGetResources(fd);
        if (resources)
        {
            // find a connected connector:
            std::cout << "resources connectors num: " << resources->count_connectors << std::endl;
            for (int c = 0; c < resources->count_connectors && !ret; c++)
            {
                drmModeConnector* connector = drmModeGetConnector(fd, resources->connectors[c]);
                std::cout << "connector modes num: " << connector->count_modes << std::endl;
                if (connector->connection == DRM_MODE_CONNECTED)
                {
                    for (int m = 0; m < connector->count_modes && !ret; m++)
                    {
                        drmModeModeInfo* mode = &connector->modes[m];
                        if (resolution_w == mode->hdisplay && 
                            resolution_h == mode->vdisplay)
                        {
                            for (int e = 0; e < resources->count_encoders && !ret; e++)
                            {
                                drmModeEncoder* encoder = drmModeGetEncoder(fd, resources->encoders[e]);
                                if (encoder->encoder_id == connector->encoder_id)
                                {
                                    drm.fd = fd;
                                    drm.resources = resources;
                                    drm.connector = connector;
                                    drm.encoder = encoder;
                                    drm.crtc = drmModeGetCrtc(drm.fd, encoder->crtc_id);
                                    drm.mode = mode;
                                    ret = true;
                                    std::cout << "find encoder." << std::endl;
                                    break;
                                }
                                if (!ret)
                                    drmModeFreeEncoder(encoder);
                            }
                        }
                    }
                }
                if (!ret)
                    drmModeFreeConnector(connector);
            }
        }
    }

    return ret;
}


bool gbm_egl_device_impl::init_gbm()
{
    bool ret = false;
    gbm_device* dev = gbm_create_device(drm.fd);
    if (dev)
    {
        gbm_surface* surf = gbm_surface_create(dev, 
            drm.mode->hdisplay, drm.mode->vdisplay, 
            GBM_FORMAT_XRGB8888, 
            GBM_BO_USE_SCANOUT | GBM_BO_USE_RENDERING);
        if (surf)
        {
            gbm.dev = dev;
            gbm.surface = surf;
            ret = true;
        }
        else
        {
            gbm_device_destroy(dev);
            std::cerr << "failed to create gbm surface!" << std::endl;
        }
    }
    else
    {
        std::cerr << "failed to create gbm device!" << std::endl;
    }

    return ret;
}


bool gbm_egl_device_impl::init_gl()
{
	bool ret = false;

	EGLDisplay eglDisplay = eglGetPlatformDisplayEXT(EGL_PLATFORM_GBM_KHR, gbm.dev, nullptr);

	EGLint major, minor;
	if (eglInitialize(eglDisplay, &major, &minor))
    {
        std::cout << "EGL Version: "    << eglQueryString(eglDisplay, EGL_VERSION)    << std::endl;
        std::cout << "EGL Vendor: "     << eglQueryString(eglDisplay, EGL_VENDOR)     << std::endl;
        std::cout << "EGL Extensions: " << eglQueryString(eglDisplay, EGL_EXTENSIONS) << std::endl;

        if (eglBindAPI(EGL_OPENGL_ES_API))
        {
            EGLint n = 0;
            const EGLint config_attribs[] = {
                EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
                EGL_RED_SIZE, 1,
                EGL_GREEN_SIZE, 1,
                EGL_BLUE_SIZE, 1,
                EGL_ALPHA_SIZE, 0,
                EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
                EGL_NONE
            };
            EGLConfig eglConfig = nullptr;
            if (eglChooseConfig(eglDisplay, config_attribs, &eglConfig, 1, &n) && n == 1)
            {
                EGLContext eglContext = nullptr;
                const EGLint context_attribs[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };
                if ((eglContext = eglCreateContext(eglDisplay, eglConfig, EGL_NO_CONTEXT, context_attribs)))
                {
                    EGLSurface eglSurface = nullptr;
                    if ((eglSurface = eglCreateWindowSurface(eglDisplay, eglConfig, (EGLNativeWindowType)gbm.surface, nullptr)) != EGL_NO_SURFACE)
                    {
                        // connect the context to the surface
                        if (eglMakeCurrent(eglDisplay, eglSurface, eglSurface, eglContext))
                        {
                            std::cout << std::endl;
                            std::cout << "GL Version: "    << glGetString(GL_VERSION)                   << std::endl;
                            std::cout << "GL Vendor: "     << glGetString(GL_VENDOR)                    << std::endl;
                            std::cout << "GL Renderer: "   << glGetString(GL_RENDERER)                  << std::endl;
                            std::cout << "GLSL Version: "  << glGetString(GL_SHADING_LANGUAGE_VERSION)  << std::endl;
                            std::cout << "GL Extensions: " << glGetString(GL_EXTENSIONS)                << std::endl;

                            gl.display = eglDisplay;
                            gl.config = eglConfig;
                            gl.context = eglContext;
                            gl.surface = eglSurface;
                            ret = true;

                            //create_texture(512, 512);
                        }
                        else
                        {
                            std::cerr << "failed to make current egl context to surface" << std::endl;
                        }
                    }
                    else
                    {
                        eglDestroyContext(eglDisplay, eglContext);
                        eglTerminate(eglDisplay);
                        std::cerr << "failed to create egl surface" << std::endl;
                    }
                }
                else
                {
                    std::cerr << "failed to create egl context" << std::endl;
                }
            }
            else
            {
                std::cerr << "failed to choose egl config" << std::endl;
            }
        }
        else
        {
            std::cerr << "failed to bind api EGL_OPENGL_ES_API" << std::endl;
        }
    }
    else
    {
        std::cerr << "failed to initialize EGL!" << std::endl;
    }

	return ret;
}


gbm_egl_device_impl::drm_fb* gbm_egl_device_impl::drm_fb_get_from_bo(gbm_bo* bo)
{
	drm_fb* fb = (drm_fb*)gbm_bo_get_user_data(bo);
	if (!fb)
    {
        fb = new drm_fb;
        fb->bo = bo;
        fb->fd = drm.fd;

        uint32_t width = gbm_bo_get_width(bo);
	    uint32_t height = gbm_bo_get_height(bo);
        uint32_t stride = gbm_bo_get_stride(bo);
        uint32_t handle = gbm_bo_get_handle(bo).u32;

        if (drmModeAddFB(fb->fd, width, height, 24, 32, stride, handle, &fb->fb_id) == 0)
        {
            gbm_bo_set_user_data(bo, fb, [](gbm_bo* bo, void* data){
                drm_fb *fb = (drm_fb*)data;
                if (fb->fb_id)
                	drmModeRmFB(fb->fd, fb->fb_id);
                delete fb;
            });
        }
        else
        {
            delete fb;
            std::cout << "failed to create fb: " << strerror(errno) << std::endl;
            return nullptr;
        }
    }

	return fb;
}
