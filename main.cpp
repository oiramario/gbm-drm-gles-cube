#include "src/gbm_egl_device_interface.hpp"

int main()
{
	auto device = gbm_egl_device_interface::get_instance();
	device->create(1280, 720);
}
