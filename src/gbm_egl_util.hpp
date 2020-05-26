#ifndef _gbm_egl_util_hpp__
#define _gbm_egl_util_hpp__

#include <sys/types.h>

struct ESMatrix
{
    float m[4][4];

    void identity();
    void scale(float sx, float sy, float sz);
    void translate(float tx, float ty, float tz);
    void rotate(float angle, float x, float y, float z);

    void frustum(float left, float right, float bottom, float top, float nearZ, float farZ);
    void perspective(float fovy, float aspect, float nearZ, float farZ);
    void ortho(float left, float right, float bottom, float top, float nearZ, float farZ);
    
    static ESMatrix multiply(const ESMatrix& srcA, const ESMatrix& srcB);
};


uint create_program(const char* vs_src, const char* fs_src);
void destroy_program(uint program);
uint create_generic_program();
uint create_z16_program();
uint create_geometry_cube();
void destroy_geometry(uint geometry);
void draw_cube(uint cube_vbo);

#endif
