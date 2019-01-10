#include "gbm_egl_device_interface.hpp"
#include "gbm_egl_instance.hpp"

bool gbm_egl_device_interface::create(uint16_t resolution_w, uint16_t resolution_h)
{
	bool ret = create_impl(resolution_w, resolution_h);
	if (ret)
		main_loop_impl();
	return ret;
}


std::shared_ptr<class gbm_egl_device_interface> gbm_egl_device_interface::get_instance()
{
	static auto singleton = std::make_shared<gbm_egl_instance>();
	return singleton;
}
