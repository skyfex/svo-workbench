
#ifndef Octree_glutils_h
#define Octree_glutils_h

#include <OpenGL/gl.h>
#include "vmath.h"

static inline void glQuad(Vector3f p0, Vector3f p1, Vector3f p2, Vector3f p3)
{
    glBegin(GL_QUADS);
	glVertex3f(p0.x, p0.y, p0.z);
	glVertex3f(p1.x, p1.y, p1.z);
	glVertex3f(p2.x, p2.y, p2.z);
	glVertex3f(p3.x, p3.y, p3.z);

	glEnd();
}

static inline void glAxisCube(Vector3f p0, Vector3f p1)
{
    glBegin(GL_QUADS);
    
	glVertex3f(p0.x, p0.y, p0.z);
	glVertex3f(p1.x, p0.y, p0.z);
    glVertex3f(p1.x, p1.y, p0.z);
	glVertex3f(p0.x, p1.y, p0.z);
    
	glVertex3f(p1.x, p0.y, p0.z);
	glVertex3f(p1.x, p0.y, p1.z);
	glVertex3f(p1.x, p1.y, p1.z);
	glVertex3f(p1.x, p1.y, p0.z);

    glVertex3f(p0.x, p0.y, p1.z);
	glVertex3f(p1.x, p0.y, p1.z);
    glVertex3f(p1.x, p1.y, p1.z);
	glVertex3f(p0.x, p1.y, p1.z);
    
    glVertex3f(p0.x, p0.y, p0.z);
	glVertex3f(p0.x, p0.y, p1.z);
	glVertex3f(p0.x, p1.y, p1.z);
	glVertex3f(p0.x, p1.y, p0.z);
    
    glVertex3f(p0.x, p1.y, p0.z);
	glVertex3f(p1.x, p1.y, p0.z);
	glVertex3f(p1.x, p1.y, p1.z);
	glVertex3f(p0.x, p1.y, p1.z);

    glVertex3f(p0.x, p0.y, p0.z);
	glVertex3f(p1.x, p0.y, p0.z);
	glVertex3f(p1.x, p0.y, p1.z);
	glVertex3f(p0.x, p0.y, p1.z);
	glEnd();
}

static inline void glLine(Vector3f p0, Vector3f p1)
{
    glBegin(GL_LINES);
	glVertex3f(p0.x, p0.y, p0.z);
	glVertex3f(p1.x, p1.y, p1.z);
    glEnd();
}

static inline void glPoint(Vector3f p, float size)
{
    glPointSize(size);
    glBegin(GL_POINTS);
    glVertex3f(p.x, p.y, p.z);
    glEnd();
}



static void glAxisIndicators(float s)
{
    glColor3f(1,0,0);
    glLine(Vector3f(0,0,0), Vector3f(s,0,0));
    glLine(Vector3f(s,0,0), Vector3f(s-0.1,0.1,0));
    glLine(Vector3f(s,0,0), Vector3f(s-0.1,-0.1,0));
    glColor3f(0,1,0);
    glLine(Vector3f(0,0,0), Vector3f(0,s,0));
    glLine(Vector3f(0,s,0), Vector3f(0.1,s-0.1,0));
    glLine(Vector3f(0,s,0), Vector3f(-0.1,s-0.1,0));
    glColor3f(0,0,1);
    glLine(Vector3f(0,0,0), Vector3f(0,0,s));
    glLine(Vector3f(0,0,s), Vector3f(0.1,0,s-0.1));
    glLine(Vector3f(0,0,s), Vector3f(-0.1,0,s-0.1));
}


static void glMakePTransform(float *mat, float left, float right, float bottom, float top,  float near, float far)
{
    float n = near; float f = far;
    float l = left; float r = right;
    float t = top; float b = bottom;
    mat[0] = 2*n / (r - l);
    mat[1] = 0.0;
    mat[2] = 0.0;
    mat[3] = 0.0;
    
    mat[4] = 0.0;
    mat[5] = 2*n / (t - b);
    mat[6] = 0.0;
    mat[7] = 0.0;
    
    mat[8] = (r + l) / (r - l);
    mat[9] = (t + b) / (t - b);
    mat[10] = -(f + n) / (f - n);
    mat[11] = -1.0;
    
    mat[12] = 0.0;
    mat[13] = 0.0;
    mat[14] = - 2*n *f / (f-n);
    mat[15] = 0.0;
}

static void glMakePTransform2(float *mat, float left, float right, float bottom, float top,  float near, float far)
{
    float temp, temp2, temp3, temp4;
    temp = 2.0 * near;
    temp2 = right - left;
    temp3 = top - bottom;
    temp4 = far - near;
    
    mat[0] = temp / temp2;
    mat[1] = 0.0;
    mat[2] = 0.0;
    mat[3] = 0.0;
    
    mat[4] = 0.0;
    mat[5] = temp / temp3;
    mat[6] = 0.0;
    mat[7] = 0.0;
    
    mat[8] = (right + left) / temp2;
    mat[9] = (top + bottom) / temp3;
    mat[10] = (far + near) / temp4;
    mat[11] = -1.0;
    
    mat[12] = 0.0;
    mat[13] = 0.0;
    mat[14] = (temp * far) / temp4;
    mat[15] = 0.0;
}

static void glMakeIPTransform(float *mat, float left, float right, float bottom, float top,  float near, float far)
{
    float n = near; float f = far;
    float l = left; float r = right;
    float t = top; float b = bottom;
    
    mat[0] = -f*(r-l) / (f-n);
    mat[1] = 0.0;
    mat[2] = 0.0;
    mat[3] = 0.0;
    
    mat[4] = 0.0;
    mat[5] = -f*(t-b) / (f-n);
    mat[6] = 0.0;
    mat[7] = 0.0;
    
    mat[8] = 0;
    mat[9] = 0;
    mat[10] = 0;
    mat[11] = 1.0;
    
    mat[12] = - f*(r+l) / (f-n);
    mat[13] = - f*(t+b) / (f-n);
    mat[14] = 2*f*n / (f-n);
    mat[15] = - (f+n) / (f-n);
}



#endif
