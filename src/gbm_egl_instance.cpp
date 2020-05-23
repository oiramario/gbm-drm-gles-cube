#include "gbm_egl_instance.hpp"
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <iostream>
#include <librealsense2/rs.hpp>

#define RS_WIDTH 1920
#define RS_HEIGHT 1080
#define RS_FPS 30

static rs2::pipeline pipe;

gbm_egl_device_interface* gbm_egl_instance::new_instance()
{
	return new gbm_egl_instance;
}


void gbm_egl_instance::begin_impl()
{
	std::cout << "begin_impl" << std::endl;
	glViewport(0, 0, get_resolution_width(), get_resolution_height());
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);


    float aspect = (float)get_resolution_height() / (float)get_resolution_width();
	projection.frustum(-2.8f, +2.8f, -2.8f * aspect, +2.8f * aspect, 6.0f, 10.0f);

    program = create_generic_program();
	glUseProgram(program);
	modelviewprojectionmatrix = glGetUniformLocation(program, "modelviewprojectionMatrix");

	GLuint samplerLoc = glGetUniformLocation(program, "uTex");
	glUniform1i(samplerLoc, 0);

	if (create_texture(RS_WIDTH, RS_HEIGHT, texture))
	{
		glBindTexture(GL_TEXTURE_EXTERNAL_OES, texture.id);
		glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameterf(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameterf(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	}

    cube_vbo = create_geometry_cube();

	// realsense
	rs2::context ctx;
	auto devicelist = ctx.query_devices();
	if (devicelist.size() > 0)
	{
		rs2::log_to_console(RS2_LOG_SEVERITY_DEBUG);
		rs2::device dev = devicelist.front();
		fprintf(stdout, "\nRealsense Device info---\n"
		                "    Name              : %s\n"
						"    Serial Number     : %s\n"
						"    Firmware Version  : %s\n"
						"    USB Type          : %s\n"
						"    Stream Color      : %d, %d, %d\n",
							dev.get_info(RS2_CAMERA_INFO_NAME),
							dev.get_info(RS2_CAMERA_INFO_SERIAL_NUMBER),
							dev.get_info(RS2_CAMERA_INFO_FIRMWARE_VERSION),
							dev.get_info(RS2_CAMERA_INFO_USB_TYPE_DESCRIPTOR),
							RS_WIDTH, RS_HEIGHT, RS_FPS);

		rs2::config cfg;
		cfg.enable_stream(RS2_STREAM_COLOR, RS_WIDTH, RS_HEIGHT, RS2_FORMAT_YUYV, RS_FPS);
		pipe.start(cfg);

    	processing_thread = std::thread([&](){
			while (running)
			{
				rs2::frameset fs;
				if (pipe.try_wait_for_frames(&fs, 1000))
				{
					rs2::frame color_frame = fs.get_color_frame();
					update_texture(texture, color_frame.get_data());
				}
			}
		});
	}
}


void gbm_egl_instance::end_impl()
{
	processing_thread.join();

	destroy_geometry(cube_vbo);
	destroy_texture(texture);
	destroy_program(program);
}


void gbm_egl_instance::update_impl()
{
    ++count;

    modelview.identity();
    modelview.translate(0.0f, 0.0f, -8.0f);
	modelview.rotate(45.0f + (0.25f * count), 1.0f, 0.0f, 0.0f);
	modelview.rotate(45.0f - (0.50f * count), 0.0f, 1.0f, 0.0f);
	modelview.rotate(10.0f + (0.15f * count), 0.0f, 0.0f, 1.0f);

	modelviewprojection = ESMatrix::multiply(modelview, projection);
}


void gbm_egl_instance::render_impl()
{
    glClearColor(0.2f, 0.3f, 0.5f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glUseProgram(program);
	glUniformMatrix4fv(modelviewprojectionmatrix, 1, GL_FALSE, &modelviewprojection.m[0][0]);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_EXTERNAL_OES, texture.id);
    draw_cube(cube_vbo);
}
