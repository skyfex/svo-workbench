

#ifndef Octree_Framebuffer_h
#define Octree_Framebuffer_h


#include <OpenGL/gl.h>
#include <stdlib.h>
#include <stdint.h>
#include "vmath.h"

#define PCtype GLfloat

struct Pixel {
    PCtype red;
    PCtype green;
    PCtype blue;
    PCtype alpha;
    
    Pixel(PCtype red, PCtype green, PCtype blue, PCtype alpha=1) :
    red(red),
    green(green),
    blue(blue),
    alpha(alpha)
    {
    }
};



class Framebuffer {
    
public:
    const GLenum format;
    const GLenum format_type;
    const size_t bpp;
    
    const GLuint width;
    const GLuint height;
    PCtype *framebuffer;
    PCtype *depthbuffer;
    
public:
    Framebuffer (uint32_t width, uint32_t height) :
    width(width),
    height(height),
    format(GL_RGBA),
    format_type(GL_FLOAT),
    bpp(4)
    {
        framebuffer = (PCtype*)malloc(sizeof(PCtype)*bpp*width*height);
        depthbuffer = (PCtype*)malloc(sizeof(PCtype)*width*height);
        clear();
    }
    
    void drawPixel(int x, int y, Pixel p, float depth)
    {
        y = height-1-y;
        PCtype *ptr = framebuffer + ((x%width)+(y*width))*bpp;
        *ptr++ = p.red;
        *ptr++ = p.green;
        *ptr++ = p.blue;
        *ptr++ = p.alpha;
        PCtype *dptr = depthbuffer + ((x%width)+(y*width));
        *dptr = depth;
    }
    
    void clear() 
    {
        PCtype *ptr = framebuffer;
        PCtype *dptr = depthbuffer;
        int i;
        for (i=0;i<width*height*bpp;i++) {
            ptr[i] = 0;
        }
        for (i=0;i<width*height;i++) {
            dptr[i] = 1;
        }
    }
    
    void flush()
    {
//        glGenTextures (1, &texID);
//        glBindTextures (GL_TEXTURE_RECTANGLE_EXT, texID);
//        glTexImage2D (GL_TEXTURE_RECTANGLE_EXT, 0, GL_RGBA, 640, 480, 0, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, frame);
//        glBegin (GL_QUADS);
//        glTexCoord2f (0, 0);
//        glVertex2f (0, 0);
//        glTexCoord2f (640, 0);
//        glVertex2f (windowWidth, 0);
//        glTexCoord2f (640, 480);
//        glVertex2f (windowWidth, windowHeight);
//        glTexCoord2f (0, 480);
//        glVertex2f (0, windowHeight);
//        glEnd();
        


        if (width<32)
            glPixelZoom(8,8);
        glDepthMask(GL_FALSE);
        glDrawPixels(width, height, format, format_type, (GLvoid*)framebuffer);	
        glDepthMask(GL_TRUE);

        glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
        glDrawPixels(width, height, GL_DEPTH_COMPONENT, format_type, (GLvoid*)depthbuffer);
        glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);


    }
};


#endif
