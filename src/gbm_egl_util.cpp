#include "gbm_egl_util.hpp"
#include <GLES2/gl2.h>
#include <iostream>
#include <memory.h>
#include <math.h>

#define PI 3.1415926535897932384626433832795f

void ESMatrix::identity()
{
    memset(m, 0x0, sizeof(ESMatrix));
    m[0][0] = 1.0f;
    m[1][1] = 1.0f;
    m[2][2] = 1.0f;
    m[3][3] = 1.0f;
}


void ESMatrix::scale(float sx, float sy, float sz)
{
    m[0][0] *= sx;
    m[0][1] *= sx;
    m[0][2] *= sx;
    m[0][3] *= sx;

    m[1][0] *= sy;
    m[1][1] *= sy;
    m[1][2] *= sy;
    m[1][3] *= sy;

    m[2][0] *= sz;
    m[2][1] *= sz;
    m[2][2] *= sz;
    m[2][3] *= sz;
}


void ESMatrix::translate(float tx, float ty, float tz)
{
    m[3][0] += (m[0][0] * tx + m[1][0] * ty + m[2][0] * tz);
    m[3][1] += (m[0][1] * tx + m[1][1] * ty + m[2][1] * tz);
    m[3][2] += (m[0][2] * tx + m[1][2] * ty + m[2][2] * tz);
    m[3][3] += (m[0][3] * tx + m[1][3] * ty + m[2][3] * tz);
}


void ESMatrix::rotate(float angle, float x, float y, float z)
{
   float sinAngle, cosAngle;
   float mag = sqrtf(x * x + y * y + z * z);
      
   sinAngle = sinf ( angle * PI / 180.0f );
   cosAngle = cosf ( angle * PI / 180.0f );
   if ( mag > 0.0f )
   {
      float xx, yy, zz, xy, yz, zx, xs, ys, zs;
      float oneMinusCos;
      ESMatrix rotMat;
   
      x /= mag;
      y /= mag;
      z /= mag;

      xx = x * x;
      yy = y * y;
      zz = z * z;
      xy = x * y;
      yz = y * z;
      zx = z * x;
      xs = x * sinAngle;
      ys = y * sinAngle;
      zs = z * sinAngle;
      oneMinusCos = 1.0f - cosAngle;

      rotMat.m[0][0] = (oneMinusCos * xx) + cosAngle;
      rotMat.m[0][1] = (oneMinusCos * xy) - zs;
      rotMat.m[0][2] = (oneMinusCos * zx) + ys;
      rotMat.m[0][3] = 0.0F; 

      rotMat.m[1][0] = (oneMinusCos * xy) + zs;
      rotMat.m[1][1] = (oneMinusCos * yy) + cosAngle;
      rotMat.m[1][2] = (oneMinusCos * yz) - xs;
      rotMat.m[1][3] = 0.0F;

      rotMat.m[2][0] = (oneMinusCos * zx) - ys;
      rotMat.m[2][1] = (oneMinusCos * yz) + xs;
      rotMat.m[2][2] = (oneMinusCos * zz) + cosAngle;
      rotMat.m[2][3] = 0.0F; 

      rotMat.m[3][0] = 0.0F;
      rotMat.m[3][1] = 0.0F;
      rotMat.m[3][2] = 0.0F;
      rotMat.m[3][3] = 1.0F;

      *this = multiply(rotMat, *this);
   }
}


void ESMatrix::frustum(float left, float right, float bottom, float top, float nearZ, float farZ)
{
    float       deltaX = right - left;
    float       deltaY = top - bottom;
    float       deltaZ = farZ - nearZ;

    if ( (nearZ <= 0.0f) || (farZ <= 0.0f) ||
         (deltaX <= 0.0f) || (deltaY <= 0.0f) || (deltaZ <= 0.0f) )
         return;

    m[0][0] = 2.0f * nearZ / deltaX;
    m[0][1] = m[0][2] = m[0][3] = 0.0f;

    m[1][1] = 2.0f * nearZ / deltaY;
    m[1][0] = m[1][2] = m[1][3] = 0.0f;

    m[2][0] = (right + left) / deltaX;
    m[2][1] = (top + bottom) / deltaY;
    m[2][2] = -(nearZ + farZ) / deltaZ;
    m[2][3] = -1.0f;

    m[3][2] = -2.0f * nearZ * farZ / deltaZ;
    m[3][0] = m[3][1] = m[3][3] = 0.0f;
}


void ESMatrix::perspective(float fovy, float aspect, float nearZ, float farZ)
{
   float frustumW, frustumH;
   
   frustumH = tanf( fovy / 360.0f * PI ) * nearZ;
   frustumW = frustumH * aspect;

   frustum(-frustumW, frustumW, -frustumH, frustumH, nearZ, farZ);
}


void ESMatrix::ortho(float left, float right, float bottom, float top, float nearZ, float farZ)
{
    float       deltaX = right - left;
    float       deltaY = top - bottom;
    float       deltaZ = farZ - nearZ;
    ESMatrix    ortho;

    if ( (deltaX == 0.0f) || (deltaY == 0.0f) || (deltaZ == 0.0f) )
        return;

    ortho.identity();
    ortho.m[0][0] = 2.0f / deltaX;
    ortho.m[3][0] = -(right + left) / deltaX;
    ortho.m[1][1] = 2.0f / deltaY;
    ortho.m[3][1] = -(top + bottom) / deltaY;
    ortho.m[2][2] = -2.0f / deltaZ;
    ortho.m[3][2] = -(nearZ + farZ) / deltaZ;

    *this = multiply(ortho, *this);
}


ESMatrix ESMatrix::multiply(const ESMatrix& srcA, const ESMatrix& srcB)
{
    ESMatrix tmp;
	for (int i=0; i<4; i++)
	{
		tmp.m[i][0] =	(srcA.m[i][0] * srcB.m[0][0]) +
						(srcA.m[i][1] * srcB.m[1][0]) +
						(srcA.m[i][2] * srcB.m[2][0]) +
						(srcA.m[i][3] * srcB.m[3][0]) ;

		tmp.m[i][1] =	(srcA.m[i][0] * srcB.m[0][1]) + 
						(srcA.m[i][1] * srcB.m[1][1]) +
						(srcA.m[i][2] * srcB.m[2][1]) +
						(srcA.m[i][3] * srcB.m[3][1]) ;

		tmp.m[i][2] =	(srcA.m[i][0] * srcB.m[0][2]) + 
						(srcA.m[i][1] * srcB.m[1][2]) +
						(srcA.m[i][2] * srcB.m[2][2]) +
						(srcA.m[i][3] * srcB.m[3][2]) ;

		tmp.m[i][3] =	(srcA.m[i][0] * srcB.m[0][3]) + 
						(srcA.m[i][1] * srcB.m[1][3]) +
						(srcA.m[i][2] * srcB.m[2][3]) +
						(srcA.m[i][3] * srcB.m[3][3]) ;
	}

    return tmp;
}


uint create_program(const char* vs_src, const char* fs_src)
{
	GLint ret;

	GLuint vs = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vs, 1, &vs_src, NULL);
	glCompileShader(vs);
	glGetShaderiv(vs, GL_COMPILE_STATUS, &ret);
	if (!ret)
    {
		glGetShaderiv(vs, GL_INFO_LOG_LENGTH, &ret);
		if (ret > 1)
        {
			char log[512];
			glGetShaderInfoLog(vs, ret, NULL, log);
            std::cerr << "vertex shader compilation failed:\n" << log << std::endl;
		}
		return -1;
	}

	GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fs, 1, &fs_src, NULL);
	glCompileShader(fs);
	glGetShaderiv(fs, GL_COMPILE_STATUS, &ret);
	if (!ret)
    {
		glGetShaderiv(fs, GL_INFO_LOG_LENGTH, &ret);
		if (ret > 1)
        {
			char log[512];
			glGetShaderInfoLog(fs, ret, NULL, log);
            std::cerr << "fragment shader compilation failed:\n" << log << std::endl;
		}

        glDeleteShader(vs);
		return -1;
	}

	GLuint program = glCreateProgram();
	glAttachShader(program, vs);
	glAttachShader(program, fs);
	glLinkProgram(program);
	glGetProgramiv(program, GL_LINK_STATUS, &ret);
	if (!ret)
    {
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &ret);
		if (ret > 1)
        {
			char log[512];
			glGetProgramInfoLog(program, ret, NULL, log);
            std::cerr << "program linking failed:\n" << log << std::endl;
		}

        glDeleteShader(vs);
        glDeleteShader(fs);
        glDeleteProgram(program);
		return -1;
	}

    return program;
}


void destroy_program(uint program)
{
    if (program != uint(-1))
    {
        GLuint shaders[8] = { uint(-1) };
        GLsizei count = 0;
        glGetAttachedShaders(program, sizeof(shaders) / sizeof(shaders[0]), &count, shaders);
        for (GLsizei i = 0; i < count; ++i)
        {
            GLuint shader = shaders[i];
            glDetachShader(program, shader);
            glDeleteShader(shader);
        }
        glDeleteProgram(program);
    }
}


uint create_generic_program()
{
    static const char *vertex_shader_source =
		"uniform mat4 modelviewprojectionMatrix;\n"
		"                                   \n"
		"attribute vec4 in_position;        \n"
		"attribute vec2 in_TexCoord;        \n"
		"                                   \n"
		"varying vec2 vTexCoord;            \n"
		"                                   \n"
		"void main()                        \n"
		"{                                  \n"
		"    gl_Position = modelviewprojectionMatrix * in_position;\n"
		"    vTexCoord = in_TexCoord; \n"
		"}                            \n";

    static const char *fragment_shader_source =
		"#extension GL_OES_EGL_image_external : require\n"
		"precision mediump float;           \n"
		"                                   \n"
		"uniform samplerExternalOES uTex;   \n"
//		"uniform sampler2D uTex;   \n"
		"                                   \n"
		"varying vec2 vTexCoord;            \n"
		"                                   \n"
		"void main()                        \n"
		"{                                  \n"
		"    gl_FragColor = texture2D(uTex, vTexCoord);\n"
		"}                                  \n";

    return create_program(vertex_shader_source, fragment_shader_source);
}


uint create_geometry_cube()
{
    static const float vVertices[] = {
            // front
            -1.0f, -1.0f, +1.0f,
            +1.0f, -1.0f, +1.0f,
            -1.0f, +1.0f, +1.0f,
            +1.0f, +1.0f, +1.0f,
            // back
            +1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f, -1.0f,
            +1.0f, +1.0f, -1.0f,
            -1.0f, +1.0f, -1.0f,
            // right
            +1.0f, -1.0f, +1.0f,
            +1.0f, -1.0f, -1.0f,
            +1.0f, +1.0f, +1.0f,
            +1.0f, +1.0f, -1.0f,
            // left
            -1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f, +1.0f,
            -1.0f, +1.0f, -1.0f,
            -1.0f, +1.0f, +1.0f,
            // top
            -1.0f, +1.0f, +1.0f,
            +1.0f, +1.0f, +1.0f,
            -1.0f, +1.0f, -1.0f,
            +1.0f, +1.0f, -1.0f,
            // bottom
            -1.0f, -1.0f, -1.0f,
            +1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f, +1.0f,
            +1.0f, -1.0f, +1.0f,
    };

    static const float vTexCoords[] = {
		//front
		1.0f, 1.0f,
		0.0f, 1.0f,
		1.0f, 0.0f,
		0.0f, 0.0f,
		//back
		1.0f, 1.0f,
		0.0f, 1.0f,
		1.0f, 0.0f,
		0.0f, 0.0f,
		//right
		1.0f, 1.0f,
		0.0f, 1.0f,
		1.0f, 0.0f,
		0.0f, 0.0f,
		//left
		1.0f, 1.0f,
		0.0f, 1.0f,
		1.0f, 0.0f,
		0.0f, 0.0f,
		//top
		1.0f, 1.0f,
		0.0f, 1.0f,
		1.0f, 0.0f,
		0.0f, 0.0f,
		//bottom
		1.0f, 0.0f,
		0.0f, 0.0f,
		1.0f, 1.0f,
		0.0f, 1.0f,
    };

    GLuint cube_vbo = -1;
    GLuint positionsoffset = 0;
    GLuint texcoordsoffset = sizeof(vVertices);
        
    glGenBuffers(1, &cube_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, cube_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vVertices) + sizeof(vTexCoords), 0, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, positionsoffset, sizeof(vVertices), &vVertices[0]);
    glBufferSubData(GL_ARRAY_BUFFER, texcoordsoffset, sizeof(vTexCoords), &vTexCoords[0]);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (const GLvoid *)(intptr_t)positionsoffset);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (const GLvoid *)(intptr_t)texcoordsoffset);
    glEnableVertexAttribArray(1);

    return cube_vbo;
}


void destroy_geometry(uint geometry)
{
    glDeleteBuffers(1, &geometry);
}


void draw_cube(uint cube_vbo)
{
    glBindBuffer(GL_ARRAY_BUFFER, cube_vbo);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glDrawArrays(GL_TRIANGLE_STRIP, 4, 4);
	glDrawArrays(GL_TRIANGLE_STRIP, 8, 4);
	glDrawArrays(GL_TRIANGLE_STRIP, 12, 4);
	glDrawArrays(GL_TRIANGLE_STRIP, 16, 4);
	glDrawArrays(GL_TRIANGLE_STRIP, 20, 4);
}
