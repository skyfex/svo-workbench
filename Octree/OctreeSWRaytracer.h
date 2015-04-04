
#ifndef Octree_OctreeSWRaytracer_h
#define Octree_OctreeSWRaytracer_h

#include "Shared.h"
#include "Octree.h"

typedef SInt32 tnum; 
const tnum tnum_max = INT32_MAX;
const tnum tnum_min = INT32_MIN;
const float o_max = (1<<5);
const float d_min = 1/(1<<10);
float tnum_scale = (1<<16);

#define EXIT_X 1
#define EXIT_Y 2
#define EXIT_Z 3

void tnum_clip(float *v)
{
    if (*v >= tnum_max) *v = tnum_max-1000;
    if (*v <= tnum_min) *v = tnum_min+1000;
}

struct RayCastOutput 
{
    float t;
    bool is_leaf;
    
    RayCastOutput()
    {
        
    }
    
    RayCastOutput(float t, bool is_leaf) : t(t), is_leaf(is_leaf)
    {
        
    }
};

struct StackItem
{
    OctreeCursor cur;
    tnum tx0, ty0, tz0;
    tnum tx1, ty1, tz1;
    int idx;
};

struct Stack
{
    StackItem *store;
    int top;
    int length;
    int ss;
    
    Stack(int size) : ss(size) {
        store = (StackItem*)malloc(sizeof(StackItem)*size);
        top = 0;
        length = 0;
    }  
    
    bool empty()
    {
        if (ss==0) return true;
        return length==0;
    }
    
    void reset()
    {
        top = 0;
        length = 0;
    }
    
    void push(OctreeCursor cur, 
              tnum tx0, tnum ty0, tnum tz0,
              tnum tx1, tnum ty1, tnum tz1,
              int idx)
    {
        if (ss==0) return;
        store[top] = (StackItem){cur, tx0, ty0, tz0, tx1, ty1, tz1, idx};
        top = (top+1)%ss;
        if (length<ss)
            length++;
    }
    
    void pop(OctreeCursor *cur, 
             tnum *tx0, tnum *ty0, tnum *tz0,
             tnum *tx1, tnum *ty1, tnum *tz1,
             int *idx)
    {
        if (empty()) throw;
        top--;
        if (top<0) top+=ss;
        length--;
        *cur = store[top].cur;
        *tx0 = store[top].tx0;
        *ty0 = store[top].ty0;
        *tz0 = store[top].tz0;
        *tx1 = store[top].tx1;
        *ty1 = store[top].ty1;
        *tz1 = store[top].tz1;
        *idx = store[top].idx;
    }
    
};

class OctreeSWRaytracer
{
public:
    Octree *ot;
    int terminLevel;
    Framebuffer *fb;
    Matrix4f projectionMat;
    Matrix4f glProjectionMat;
    Matrix4f modelMat;
    Vector4f lightPos;
    
    Cache *cache;
    
    bool debug_ray;
    bool gl_debug;
    
    Stack *stack;
    
    bool exportMode;
    ofstream *exportFile;
    int exportRayCount;
    
    int far_count;
    int push_count;
    int restart_count;
    
    OctreeSWRaytracer(Octree *targetOctree, Framebuffer *fb) : ot(targetOctree), fb(fb) 
    {
        debug_ray = false;
        terminLevel = 10;
        cache = NULL;
        stack = new Stack(8);
        exportMode = false;
    }
    
    
    RayCastOutput rayCast(Ray r)
    {
        Ray r2 = r;
        int dir_mask=0;
        
//        if (fabs(r.o.x)>o_max ||
//            fabs(r.o.y)>o_max ||
//            fabs(r.o.z)>o_max)
//            throw;
        
        if (r.d.x==0) 
            r.d.x+=0.001; 
        if (r.d.x<0.0) {
            r.o.x = -r.o.x;
            r.d.x = -r.d.x;
            dir_mask |= 4;
        }
        if (r.d.y==0) 
            r.d.y+=0.001;
        if (r.d.y<0.0) {
            r.o.y = -r.o.y;
            r.d.y = -r.d.y;
            dir_mask |= 2;
        }
        if (r.d.z==0)  
            r.d.z+=0.001; 
        if (r.d.z<0.0) {
            r.o.z = -r.o.z;
            r.d.z = -r.d.z;
            dir_mask |= 1;
        }

        float tx0, tx1, ty0, ty1, tz0, tz1;

        tx0 = (-1-r.o.x)/r.d.x;
        tx1 = (1-r.o.x)/r.d.x;
        ty0 = (-1-r.o.y)/r.d.y;
        ty1 = (1-r.o.y)/r.d.y;
        tz0 = (-1-r.o.z)/r.d.z;
        tz1 = (1-r.o.z)/r.d.z;
        
        if (debug_ray) {
//            printf("Init:\n\t%f %f %f\n\t%f %f %f\n", 
//                   tx0, ty0, tz0,
//                   tx1, ty1, tz1);
            printf("\tdir_mask: %d\n", dir_mask);
        }
        
//        float t_min = min(tx0,min(ty0,tz0));
//        float t_max = max(tx1,max(ty1,tz1));
//        float t_absmax = max(abs(t_min),abs(t_max));
        
        float t_enter = max(max(tx0,ty0),tz0);
        float t_exit = min(min(tx1, ty1), tz1);
        

        tx0 *= tnum_scale; ty0 *= tnum_scale; tz0 *= tnum_scale;
        tx1 *= tnum_scale; ty1 *= tnum_scale; tz1 *= tnum_scale;

        
        tnum_clip(&tx0); 
        tnum_clip(&ty0); 
        tnum_clip(&tz0);
        tnum_clip(&tx1); tnum_clip(&ty1); tnum_clip(&tz1);
        
        assert(tx0<tnum_max && tx0>tnum_min);
        assert(ty0<tnum_max && ty0>tnum_min);
        assert(tz0<tnum_max && tz0>tnum_min);
        
        assert(tx1<tnum_max && tx1>tnum_min);
        assert(ty1<tnum_max && ty1>tnum_min);
        assert(tz1<tnum_max && tz1>tnum_min);
        
        if (debug_ray)
            printf("%.8x %.8x %.8x\n%.8x %.8x %.8x\n\n", 
                   (tnum)tx0, (tnum)ty0, (tnum)tz0, 
                   (tnum)tx1, (tnum)ty1, (tnum)tz1);
        
        if (t_enter < t_exit) {
            OctreeCursor cursor = ot->newRootCursor();
            cursor.cache = cache;
            
            if (exportMode){
                exportRay(tx0,ty0,tz0,
                          tx1,ty1,tz1,
                          dir_mask);
                return RayCastOutput(0,0);
            }
            else
                return rayCastCore(r2, 
                            tx0,ty0,tz0,
                            tx1,ty1,tz1,
                            dir_mask,
                            cursor);
        }
        else {
            if (exportMode)
                exportRay(0,0,0,0,0,0, (1<<31));
        }
        if (debug_ray) printf("End.\n");
        return RayCastOutput(0,false);
    }



    
    RayCastOutput rayCastCore(Ray ray, 
                     tnum r_tx0, tnum r_ty0, tnum r_tz0,
                     tnum r_tx1, tnum r_ty1, tnum r_tz1,
                     int dir_mask,
                     OctreeCursor r_cur)
    {
        
        int level;
        int idx, idx_flip;
        bool is_first_node;
        bool exits_node;
        bool pass_node;
        tnum t_start = 0;
        tnum t_enter;
        tnum t_exit_child;
        
        tnum tx0, ty0, tz0;
        tnum tx1, ty1, tz1;
        tnum txm, tym, tzm;
        tnum tx0_child, ty0_child, tz0_child;
        tnum tx1_child, ty1_child, tz1_child;
        
        bool is_leaf;
        tnum t_out;
        
        OctreeCursor cur;
        
    S_INIT:
        cur = r_cur;

        tx0 = r_tx0; ty0 = r_ty0; tz0 = r_tz0;
        tx1 = r_tx1; ty1 = r_ty1; tz1 = r_tz1;
        
        is_first_node = true;
        level = 0;
        exits_node = false;
        
        stack->reset();
        
        if (debug_ray)
            printf("Init - t_start: %.8x\n",  t_start);
        goto S_CALC_T;
        
  
    S_CALC_T:
        txm = (tx0+tx1)/2;
        tym = (ty0+ty1)/2;
        tzm = (tz0+tz1)/2;

        t_enter = max(tx0,max(ty0,tz0));
        
        if (debug_ray)
            printf("t_enter: %.8x\n",  t_enter);
        
        if (is_first_node)
            goto S_FIRST_IDX;
        else
            goto S_NEXT_IDX;
        
    S_FIRST_IDX:
        idx = (txm < t_enter)<<2 | (tym < t_enter)<<1 | (tzm < t_enter);
        exits_node = false;
        is_first_node = false;
        if (debug_ray)
            printf("FrstIdx: %d / %d\n", idx, idx^dir_mask);
        goto S_DATA_WAIT;
        
    S_DATA_WAIT:
        // if (got_data)
        if (debug_ray)
            printf("Data: %.8x : %.8x\n", cur.adr*4, *((int*)cur.node));
        goto S_CALC_CHILD_T;

        
        
    S_NEXT_IDX:
        exits_node = false;
        if (t_exit_child == tx1_child) {
            if (idx & 4)
                exits_node = true;
            else
                idx = idx ^ 4;
        }
        else if (t_exit_child == ty1_child) {
            if (idx & 2)
                exits_node = true;
            else
                idx = idx ^ 2;
        }
        else {
            if (idx & 1)
                exits_node = true;
            else
                idx = idx ^ 1;        
        }
        if (debug_ray)
            printf("NextIdx: %d / %d   exit: %d\n", idx, idx^dir_mask, exits_node);
        goto S_NEXT_EVAL;

    S_NEXT_EVAL:
        if (exits_node) {
            if (level==0) {
                is_leaf = false;
                goto S_FINISHED;
            }
            else if (stack->empty()) {
                if (debug_ray) 
                    printf("Underflow\n\n");
                restart_count++;
                t_start = t_exit_child;
                goto S_INIT;
            }
            else {
                if (debug_ray) printf("Pop\n");
                stack->pop(&cur,
                           &tx0, &ty0, &tz0,
                           &tx1, &ty1, &tz1,
                           &idx);
                level--;
                goto S_CALC_T;
            }
        }
        else {
            goto S_CALC_CHILD_T;
        }
        
    S_CALC_CHILD_T:
//        if (debug_ray) printf("Idx %d / %d\n", idx, idx ^ dir_mask);
        tx0_child = (idx&4) ? txm : tx0;
        tx1_child = (idx&4) ? tx1 : txm;
        ty0_child = (idx&2) ? tym : ty0;
        ty1_child = (idx&2) ? ty1 : tym;
        tz0_child = (idx&1) ? tzm : tz0;
        tz1_child = (idx&1) ? tz1 : tzm;
        
        t_exit_child = min(tx1_child,min(ty1_child,tz1_child));

        goto S_EVAL;
        
    S_EVAL:
        idx_flip = idx ^ dir_mask;
        pass_node = t_exit_child <= t_start;
        if (pass_node) {
            goto S_NEXT_IDX;
        }
        else if (cur.childIsFilled(idx_flip) || level==terminLevel) {
            tx0 = tx0_child; ty0 = ty0_child; tz0 = tz0_child;
            tx1 = tx1_child; ty1 = ty1_child; tz1 = tz1_child;
            is_leaf = true;
            goto S_FINISHED;
        }
        else if (cur.childIsValid(idx_flip)) {
            if (debug_ray) {
//                printf("%.8x %.8x %.8x\n%.8x %.8x %.8x\n", 
//                       tx0, ty0, tz0, tx1, ty1, tz1);
                if (cur.childPtrIsFar())
                    printf("Far %x\n", cur.farPtr());
                printf("Push\n\n");
            }
            if (cur.childPtrIsFar()) {
                far_count++;
            }
            push_count++;
            stack->push(cur,
                        tx0, ty0, tz0,
                        tx1, ty1, tz1,
                        idx);
            
            tx0 = tx0_child; ty0 = ty0_child; tz0 = tz0_child;
            tx1 = tx1_child; ty1 = ty1_child; tz1 = tz1_child;
            
            level++;
            is_first_node = true;
            
            cur = cur.getChild(idx_flip);
            
            goto S_CALC_T;
        }
        else {
            goto S_NEXT_IDX;
        }
        
    S_FINISHED:
        t_enter = max(tx0,max(ty0,tz0));
        t_out = is_leaf ? t_enter : t_exit_child;
        if (debug_ray) {
         printf("Leaf: %d %.8x\n\n\n", is_leaf, t_out);   
        }
        return RayCastOutput((float)t_out/(float)tnum_scale, is_leaf);
    }
    
    void drawRay(int x, int y, Ray ray, RayCastOutput result)
    {
        if (result.is_leaf) {
            float qz = result.t;
            float n = 0.1;
            float f = 8;
            float vz = f/(f-n) - f*n/(f-n)/((f-n)*qz+n);
            fb->drawPixel(x, y, Pixel(vz,vz,vz), 0);
        }
        else {
            fb->drawPixel(x, y, Pixel(1,1,1), 0);
        }
    }
 
    void render()
    {
        if (!ot->isBuilt()) return;
        if (cache) cache->reset();
        
        float w = fb->width;
        float h = fb->height;
        
        float dx = 2/w;
        float dy = 2/h;
        
        float rx, ry;
        int x,y;
        for (x = 0, rx = -1; x < fb->width; x++, rx+=dx) {
            for (y = 0, ry = -1; y < fb->height; y++, ry+=dy) {
                
//                if (x==10 && y==7) 
//                    debug_ray = true;
                
                Vector4f origin = Vector4f(rx, -ry, -1, 1);
                Vector4f dest = Vector4f(rx, -ry, 1, 1);
                
                origin = projectionMat * origin;
                origin = origin / origin.w;
                dest = projectionMat * dest;
                dest = dest / dest.w;
                Vector4f vec = dest-origin;
                
                Vector4f origin2 = modelMat * origin;
                Vector4f dest2 = modelMat * dest;
                Vector4f vec2 = dest2-origin2;
                
                Ray ray = Ray(origin2, vec2);
                
                push_count = 0;
                restart_count = 0;
                far_count = 0;
                RayCastOutput result;
                result = rayCast(ray);
                
                if (result.is_leaf) {
                    debug_ray = false;
                    float t = result.t - 10/(float)tnum_scale;
                    Vector4f ipoint = origin + vec*t;
                    Vector4f glipoint = glProjectionMat * ipoint;
                    glipoint = glipoint / glipoint.w;
                    float vz = (glipoint.z+1)/2;
                    
                    
                    if (1) {
                        // Normal drawing routine
                        Vector4f ipoint2 = origin2 + vec2*t;
                        Vector4f lightVec = lightPos - ipoint2;
                        
                        Ray lray = Ray(ipoint2, lightVec);
                        
                        RayCastOutput lres;
                        lres = rayCast(lray); 
                        if (!lres.is_leaf) {
                            fb->drawPixel(x, y, Pixel(1,0.7,0.5), vz); 
                        }
                        else {
                            fb->drawPixel(x, y, Pixel(0.5,0.35,0.25), vz); 
                        }
                    }
                    else {
                        // Debug drawing routine to show cost of rays
                        fb->drawPixel(x, y, Pixel((float)push_count/20.0,0,0), 0); 
                    }
                }
                else {
                    if (0) 
                        fb->drawPixel(x, y, Pixel(0,(float)push_count/20.0,(float)push_count/20.0), 0); 
                    if (debug_ray) {
                        fb->drawPixel(x, y, Pixel(1,0,0), 1);
                    }
                }
                
                debug_ray = false;
            }
        }
    }
    
    void exportRay(tnum r_tx0, tnum r_ty0, tnum r_tz0,
                   tnum r_tx1, tnum r_ty1, tnum r_tz1,
                   int dir_mask)
    {
        exportFile->write((char*)&dir_mask, sizeof(int));
        exportFile->write((char*)&r_tx0, sizeof(tnum));
        exportFile->write((char*)&r_ty0, sizeof(tnum));
        exportFile->write((char*)&r_tz0, sizeof(tnum));
        exportFile->write((char*)&r_tx1, sizeof(tnum));
        exportFile->write((char*)&r_ty1, sizeof(tnum));
        exportFile->write((char*)&r_tz1, sizeof(tnum));
        exportRayCount++;
    }
    
    void exportRayData(ofstream *file, int width, int height)
    {

        exportMode = true;
        exportFile = file;
        exportRayCount = width*height;
        
        float w = width;
        float h = height;
        
        float dx = 2/w;
        float dy = 2/h;
        
        
        exportFile->write((char*)&exportRayCount, sizeof(int));
        
        float rx, ry;
        int x,y;
        for (y = 0, ry = -1; y < height; y++, ry+=dy) {
            for (x = 0, rx = -1; x < width; x++, rx+=dx) {

                Vector4f origin = Vector4f(rx, -ry, -1, 1);
                Vector4f dest = Vector4f(rx, -ry, 1, 1);
                
                origin = projectionMat * origin;
                origin = origin / origin.w;
                dest = projectionMat * dest;
                dest = dest / dest.w;
                Vector4f vec = dest-origin;
                
                Vector4f origin2 = modelMat * origin;
                Vector4f dest2 = modelMat * dest;
                Vector4f vec2 = dest2-origin2;
                
                Ray ray = Ray(origin2, vec2);
                rayCast(ray);
                
            }
        }

        exportFile = NULL;
        exportMode = false;
    }
};

#endif
