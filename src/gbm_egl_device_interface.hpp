#ifndef _gbm_egl_device_interface_hpp__
#define _gbm_egl_device_interface_hpp__

#include <stddef.h>
#include <memory>

class gbm_egl_device_interface
{
public:
    static std::shared_ptr<class gbm_egl_device_interface> get_instance();
    bool create(uint16_t resolution_w, uint16_t resolution_h);

protected:
    virtual ~gbm_egl_device_interface() = default;

private:
    virtual bool create_impl(uint16_t resolution_w, uint16_t resolution_h) = 0;
    virtual void main_loop_impl() = 0;
};

#endif
