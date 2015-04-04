
#ifndef Octree_OctreeGL_h
#define Octree_OctreeGL_h

#include "Shared.h"
#include "Octree.h"

class OctreeGL
{
public:
    Octree *ot;
    
    int terminLevel;

    OctreeGL(Octree *targetOctree) : ot(targetOctree) 
    {
        terminLevel = 10;
    }
    
    
    void renderLevel(OctreeCursor cur, Vector3f p0, float size, int level)
    {
        if (level==terminLevel) {
            glAxisCube(p0, p0+Vector3f(size,size,size));
            return;
        }
        float c_size = size/2;
        Vector3f c_p0[8];
        ot->generateChildCoords(c_p0, p0, c_size);
        int i;
        for (i=0;i<8;i++) {
            if (cur.childIsFilled(i)) {
                glAxisCube(c_p0[i], c_p0[i]+Vector3f(c_size, c_size, c_size));
            }
            else
            if (cur.childIsPartial(i)) {
                renderLevel(cur.getChild(i), c_p0[i], c_size, level+1);                
            }
  
        }
    }
    
    void render()
    {
        if (!ot->isBuilt()) return;
        OctreeCursor cur = ot->newRootCursor();
        renderLevel(cur, Vector3f(-1,-1,-1), 2, 0);
    }
    
    
};

#endif
