#include "gbm_egl_instance.hpp"
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <iostream>
#include <librealsense2/rs.hpp>

#define RS_COLOR_WIDTH 1920
#define RS_COLOR_HEIGHT 1080
#define RS_DEPTH_WIDTH 1280
#define RS_DEPTH_HEIGHT 720
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
	projection_matrix.frustum(-2.8f, +2.8f, -2.8f * aspect, +2.8f * aspect, 6.0f, 10.0f);

    generic_program = create_generic_program();
	{
		glUseProgram(generic_program);
//		u_mvp = glGetUniformLocation(generic_program, "mvp");
		GLuint samplerLoc = glGetUniformLocation(generic_program, "uTex");
		glUniform1i(samplerLoc, 0);
		if (create_texture(RS_COLOR_WIDTH, RS_COLOR_HEIGHT, color_texture, TextureFormat::YUYV))
		{
			glBindTexture(GL_TEXTURE_EXTERNAL_OES, color_texture.id);
			glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameterf(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameterf(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		}
	}

	z16_program = create_z16_program();
	{
		glUseProgram(z16_program);
//		u_mvp = glGetUniformLocation(z16_program, "mvp");
		GLuint samplerLoc = glGetUniformLocation(z16_program, "uTex");
		glUniform1i(samplerLoc, 0);
		if (create_texture(RS_DEPTH_WIDTH, RS_DEPTH_HEIGHT, depth_texture, TextureFormat::Depth16))
		{
			glBindTexture(GL_TEXTURE_EXTERNAL_OES, depth_texture.id);
			glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameterf(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameterf(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		}
	}

    cube_vbo = create_geometry_cube();

	// realsense
	rs2::context ctx;
	auto devicelist = ctx.query_devices();
	if (devicelist.size() > 0)
	{
		rs2::log_to_console(RS2_LOG_SEVERITY_WARN);
		rs2::device dev = devicelist.front();
		fprintf(stdout, "\nRealsense Device info---\n"
		                "    Name              : %s\n"
						"    Serial Number     : %s\n"
						"    Firmware Version  : %s\n"
						"    USB Type          : %s\n"
						"    Stream Color      : %d, %d\n"
						"    Stream Depth      : %d, %d\n"
						"    FPS               : %d\n",
							dev.get_info(RS2_CAMERA_INFO_NAME),
							dev.get_info(RS2_CAMERA_INFO_SERIAL_NUMBER),
							dev.get_info(RS2_CAMERA_INFO_FIRMWARE_VERSION),
							dev.get_info(RS2_CAMERA_INFO_USB_TYPE_DESCRIPTOR),
							RS_COLOR_WIDTH, RS_COLOR_HEIGHT, 
							RS_DEPTH_WIDTH, RS_DEPTH_HEIGHT, 
							RS_FPS);

		rs2::config cfg;
		cfg.enable_stream(RS2_STREAM_COLOR, RS_COLOR_WIDTH, RS_COLOR_HEIGHT, RS2_FORMAT_YUYV, RS_FPS);
		cfg.enable_stream(RS2_STREAM_DEPTH, RS_DEPTH_WIDTH, RS_DEPTH_HEIGHT, RS2_FORMAT_Z16, RS_FPS);
		pipe.start(cfg);

    	processing_thread = std::thread([&](){
			while (running)
			{
				rs2::frameset fs;
				if (pipe.try_wait_for_frames(&fs, 1000))
				{
					rs2::frame color_frame = fs.get_color_frame();
					update_texture(color_texture, color_frame.get_data());

					rs2::frame depth_frame = fs.get_depth_frame();
					update_texture(depth_texture, depth_frame.get_data());
				}
			}
		});
	}
}


void gbm_egl_instance::end_impl()
{
	processing_thread.join();
	pipe.stop();

	destroy_geometry(cube_vbo);
	destroy_texture(color_texture);
	destroy_texture(depth_texture);
	destroy_program(generic_program);
	destroy_program(z16_program);
}


void gbm_egl_instance::update_impl()
{
    ++count;

    color_matrix.identity();
    color_matrix.translate(-2.0f, 0.0f, -9.0f);
	color_matrix.rotate(0.25f * count, 1.0f, 0.0f, 0.0f);
	color_matrix.rotate(0.50f * count, 0.0f, 1.0f, 0.0f);
	color_matrix.rotate(0.15f * count, 0.0f, 0.0f, 1.0f);

    depth_matrix.identity();
    depth_matrix.translate(+2.0f, 0.0f, -9.0f);
	depth_matrix.rotate(0.25f * count, 1.0f, 0.0f, 0.0f);
	depth_matrix.rotate(0.50f * count, 0.0f, 1.0f, 0.0f);
	depth_matrix.rotate(0.15f * count, 0.0f, 0.0f, 1.0f);
}


void gbm_egl_instance::render_impl()
{
    glClearColor(0.2f, 0.3f, 0.5f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glActiveTexture(GL_TEXTURE0);

	ESMatrix mvp;

	glUseProgram(generic_program);
	mvp = ESMatrix::multiply(color_matrix, projection_matrix);
	u_mvp = glGetUniformLocation(generic_program, "mvp");
	glUniformMatrix4fv(u_mvp, 1, GL_FALSE, &mvp.m[0][0]);
	glBindTexture(GL_TEXTURE_EXTERNAL_OES, color_texture.id);
    draw_cube(cube_vbo);

	glUseProgram(z16_program);
	mvp = ESMatrix::multiply(depth_matrix, projection_matrix);
	u_mvp = glGetUniformLocation(z16_program, "mvp");
	glUniformMatrix4fv(u_mvp, 1, GL_FALSE, &mvp.m[0][0]);
	glBindTexture(GL_TEXTURE_EXTERNAL_OES, depth_texture.id);
    draw_cube(cube_vbo);
}
