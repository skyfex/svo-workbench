
#ifndef Octree_Shared_h
#define Octree_Shared_h

struct Ray {
    Vector4f o;
    Vector4f d;
    
    Ray(Vector4f o, Vector4f d) : o(o), d(d)
    {
        
    }
};


#endif
