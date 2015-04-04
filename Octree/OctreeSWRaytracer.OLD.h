//
//  OctreeSWRaytracer.h
//  Octree
//
//  Created by Audun Wilhelmsen on 15.11.11.
//  Copyright 2011 NTNU. All rights reserved.
//

#ifndef Octree_OctreeSWRaytracer_h
#define Octree_OctreeSWRaytracer_h

#include "Shared.h"
#include "Octree.h"

typedef int32_t tnum; 
const tnum tnum_max = INT32_MAX;
const tnum tnum_min = INT32_MIN;
float tnum_scale = 1;

class OctreeSWRaytracer
{
public:
    Octree *ot;
    int terminLevel;
    
    Framebuffer *cur_fb;
    
    OctreeSWRaytracer(Octree *targetOctree) : ot(targetOctree) 
    {
        terminLevel = 10;
    }
    
    int cur_x, cur_y;
    int dir_mask;
    int zero_mask;
    bool debug_ray;
    
    void rayCast(Ray r)
    {
        Ray r2 = r;
        dir_mask=0;
        zero_mask=0;
        
        if (debug_ray)    {
            printf("%f %f %f\n", r.d.x, r.d.y, r.d.z);
            printf("%f %f %f\n", r.o.x, r.o.y, r.o.z);
        }

        
        if (r.d.x==0) 
            r.d.x+=0.001; 
        //            zero_mask |= 4;
        if (r.d.x<0.0) {
            r.o.x = -r.o.x; // todo
            r.d.x = -r.d.x;
            dir_mask |= 4;
        }
        if (r.d.y==0) 
            r.d.y+=0.001;
        //            zero_mask |= 2;
        if (r.d.y<0.0) {
            r.o.y = -r.o.y; // --
            r.d.y = -r.d.y;
            dir_mask |= 2;
        }
        if (r.d.z==0)  
            r.d.z+=0.001; 
        //            zero_mask |= 1;
        if (r.d.z<0.0) {
            r.o.z = -r.o.z; // --
            r.d.z = -r.d.z;
            dir_mask |= 1;
        }
        
        
        float tx0, tx1, ty0, ty1, tz0, tz1;
            
        
        
        tx0 = (min(ot->p0.x, ot->p1.x)-r.o.x)/r.d.x;
        tx1 = (max(ot->p0.x, ot->p1.x)-r.o.x)/r.d.x;
        ty0 = (min(ot->p0.y, ot->p1.y)-r.o.y)/r.d.y;
        ty1 = (max(ot->p0.y, ot->p1.y)-r.o.y)/r.d.y;
        tz0 = (min(ot->p0.z, ot->p1.z)-r.o.z)/r.d.z;
        tz1 = (max(ot->p0.z, ot->p1.z)-r.o.z)/r.d.z;
        
        if (debug_ray) {
            printf("Init:\n\t%f %f %f\n\t%f %f %f\n", 
                   tx0, ty0, tz0,
                   tx1, ty1, tz1);
            printf("\tdir_mask: %d\n", dir_mask);
        }
        
        float t_min = min(tx0,min(ty0,tz0));
        float t_max = max(tx1,max(ty1,tz1));
        float t_absmax = max(abs(t_min),abs(t_max));
        tnum_scale = 10000;//(float)tnum_max / (t_absmax*2);
        
        float t_enter = max(max(tx0,ty0),tz0);
        float t_exit = min(min(tx1, ty1), tz1);
        
        tx0 *= tnum_scale; tx1 *= tnum_scale;
        ty0 *= tnum_scale; ty1 *= tnum_scale;
        tz0 *= tnum_scale; tz1 *= tnum_scale;
        
        if (debug_ray)
            printf("%.8x %.8x %.8x\n%.8x %.8x %.8x\n\n", 
                   (tnum)tx0, (tnum)ty0, (tnum)tz0, 
                   (tnum)tx1, (tnum)ty1, (tnum)tz1);
        
        if (t_enter < t_exit)
            procSubtree2(r2, 
                         tx0,ty0,tz0,
                         tx1,ty1,tz1, 
                         ot->size, ot->root_adr);
        if (debug_ray) printf("End.\n");
    }
    
    
    //#define NO_STACK
#define STACK_SIZE 100
    
    struct stack_item_t;
    struct stack_item_t {
        tnum tx0; tnum ty0; tnum tz0;
        tnum tx1; tnum ty1; tnum tz1;
        int adr;
        char idx;
        
    };
        
        
    bool procSubtree2(Ray r,
                      tnum r_tx0, tnum r_ty0, tnum r_tz0,
                      tnum r_tx1, tnum r_ty1, tnum r_tz1,
                      tnum size,
                      int r_adr)
    {
        
        stack_item_t stack[STACK_SIZE];
        
        int si = 0; // stack index
        int sl = 0; // stack level
        int stack_level_max = 3;
        
        Node *n;
        int node_adr;
        
        int level;
        int idx;
        bool is_first_node;
        
        tnum t_exit;
        tnum t_start = 0;
        
        tnum tx0, ty0, tz0;
        tnum tx1, ty1, tz1;
        
        int  c_offset[8];
        bool c_valid[8];
        bool c_leaf[8];       
    
        if (debug_ray) printf("Start\n");

    S_INIT:
        tx0 = r_tx0; ty0 = r_ty0; tz0 = r_tz0;
        tx1 = r_tx1; ty1 = r_ty1; tz1 = r_tz1;
        node_adr = r_adr;
        is_first_node = true;
        level = 0;
        si = 0;
        sl = 0;
        
        BlockEntry *be = ot->getEntry(node_adr);
        n = &be->node;
        child_decode(n, c_leaf, c_valid, c_offset);
        
        while(true) {  
            assert(si>=0);
            
            tnum txm, tym, tzm;
            
            txm = (tx0+tx1)/2;
            tym = (ty0+ty1)/2;
            tzm = (tz0+tz1)/2;
            
            bool pass_node = false;
            bool exit_node = false;
            bool stack_underflow = (sl==0);
            
            
            // -- Find first/next node --
            if (is_first_node) {
                idx = first_node(tx0, ty0, tz0, txm, tym, tzm);
                is_first_node = false;
            }
            else {
                idx = new_node((idx & 0b100)?tx1:txm, (idx & 0b010)?ty1:tym, (idx & 0b001)?tz1:tzm, 
                               idx);
                exit_node = (idx==8);
            }
            
            int idx_flip = idx^dir_mask;
            
            tnum tx0_child = (idx & 0b100)?txm:tx0; 
            tnum ty0_child = (idx & 0b010)?tym:ty0; 
            tnum tz0_child = (idx & 0b001)?tzm:tz0;
            tnum tx1_child = (idx & 0b100)?tx1:txm; 
            tnum ty1_child = (idx & 0b010)?ty1:tym; 
            tnum tz1_child = (idx & 0b001)?tz1:tzm; 
            
            tnum t_enter_child = max(tx0_child, max(ty0_child, tz0_child));
            tnum t_exit_child = min(tx1_child, min(ty1_child, tz1_child));
            
            pass_node = t_exit_child<=t_start;
            
            if (debug_ray) {
                printf("State:\n\t%.8x %.8x %.8x\n\t%.8x %.8x %.8x\n\t%.8x %.8x %.8x\n", 
                       tx0, ty0, tz0, txm, tym, tzm, tx1, ty1, tz1);
                printf("Node %d data: %.8x\n", node_adr,  *(int*)(n));
                printf("Idx: %x Flipped: %x\n", idx, idx_flip); 
            }
            
            // = S_EVAL =
            
            // -- Exited node --
            if (exit_node)
            {
                if (debug_ray) printf("Pop %d\n", level);
                
                if (level==0) {
                    // -- Exited octree --
                    if (debug_ray) cur_fb->drawPixel(cur_x, cur_y, Pixel(1,0,0));
                    return false;
                }
                else if (stack_underflow) { // stack underflow
                    tnum t_exit = min(min(tx1,ty1),tz1); 
                    t_start = t_exit;
                    
//                    printf("Underflow %d\n", t_start);
                    if (debug_ray) printf("Underflow %d\n", t_start);
                    
                    goto S_INIT;
                    
                }
                else {
                    // Pop
                    level--;
                    si--;
                    sl--;
                    tx0 = stack[si].tx0; ty0 = stack[si].ty0; tz0 = stack[si].tz0;
                    tx1 = stack[si].tx1; ty1 = stack[si].ty1; tz1 = stack[si].tz1;
                    idx = stack[si].idx;
                    node_adr = stack[si].adr;
                    
                    BlockEntry *be = ot->getEntry(node_adr);
                    n = &be->node;
                    child_decode(n, c_leaf, c_valid, c_offset);
                }
                continue;
            }
            else if (pass_node) {
                continue;
            }
            // -- Hit leaf --
            else if (c_leaf[idx_flip] || level==terminLevel) {
                procLeaf(r, t_enter_child, size);
                if (debug_ray) cur_fb->drawPixel(cur_x, cur_y, Pixel(0,1,0));
                return true;
            }
            else if (c_valid[idx_flip]) { 
                // -- Hit an octant containing something --
                if (debug_ray) printf("Push %d\n", level);

                assert(si<STACK_SIZE);
                stack_item_t s_new = {
                    tx0, ty0, tz0,
                    tx1, ty1, tz1,
                    node_adr,
                    idx
                };
                stack[si] = s_new;
                si++;
                if (sl<stack_level_max) sl++;
                
                level++;
                is_first_node = true;

                tx0 = tx0_child; ty0 = ty0_child; tz0 = tz0_child;
                tx1 = tx1_child; ty1 = ty1_child; tz1 = tz1_child;
                
                if (debug_ray) printf("Request adr: %x child_ptr: %x offset: %x\n", node_adr, n->child_ptr, c_offset[idx_flip]);   
                BlockEntryPtr child_bep = ot->getRelativeEntry(node_adr, n->child_ptr);
                node_adr = child_bep.adr+c_offset[idx_flip];
                BlockEntry *be = ot->getEntry(node_adr);
                n = &be->node;
                child_decode(n, c_leaf, c_valid, c_offset);
                
            }
            
        }
    }
    
    void child_decode(Node *n, bool *c_leaf, bool *c_valid, int *c_offset)
    {
        int i, c;
        for (i=0,c=0;i<8;i++) {
            bool valid = ((n->valid_mask >>i)&1);
            bool leaf =  ((n->leaf_mask  >>i)&1);
            c_valid[i] = valid;
            c_leaf[i] = leaf;
            //            if (debug_ray) printf("\t%d: valid: %d; leaf: %d; offset: %d\n", i, valid, leaf, c);
            
            if (valid && !leaf) {
                c_offset[i] = c;
                c++;
            }
        }
    }
    
    int new_node(tnum tx1, tnum ty1, tnum tz1, int node_index)
    {
        if (debug_ray) printf("next_node: %.8x %.8x %.8x - old idx: %x\n", tx1, ty1, tz1, node_index);
        if (tx1<ty1 && tx1<tz1) {
            if (node_index & 0b100) return 8;
            return node_index ^ 0b100;
        }
        else if (ty1 < tz1) {
            if (node_index & 0b010) return 8;
            return node_index ^ 0b010;
        }
        else {
            if (node_index & 0b001) return 8;
            return node_index ^ 0b001;
        }
    }
    
    int first_node(tnum tx0, tnum ty0, tnum tz0, tnum txm, tnum tym, tnum tzm)
    {
        int i = 0;
        tnum t_enter = max(tx0,max(ty0,tz0));
        if (tzm < t_enter) i ^= 1;
        if (tym < t_enter) i ^= 2;
        if (txm < t_enter) i ^= 4;
        
        return i;
    }
    
    void procLeaf(Ray r, tnum tmin, tnum size)
    {
        //        cur_fb->drawPixel(cur_x, cur_y, Pixel(1,1,1));
        
        //        float tmin_f = tmin/tnum_scale;
        //        Vector3f p = r.o + r.d*tmin_f;
        //        
        //        Vector3f c = Vector3f((p.x/40)+0.5, 0.2, (p.y/40)+0.5);
        //        
        //        c *= (25-tmin_f)/10;
        //        cur_fb->drawPixel(cur_x, cur_y, Pixel(c.x, c.y, c.z));
        
        //        Vector3f c = Vector3f(1,1,1);
        float blu = 0;
        float red = (tmin>>8)%256;
        
        //        printf("%f\n", x);
        cur_fb->drawPixel(cur_x, cur_y, Pixel(red/256, 0, blu/256));
    }
    
    void render(Framebuffer *fb, Camera cam, int terminLevel)
    {
        if (!ot->rootNode) return;
        this->terminLevel = terminLevel;
        cur_fb = fb;
        float w = fb->width;
        float h = fb->height;
        int i, c;
        c = fb->width*fb->height;
        int x = 0;
        int y = 0;
        for (i=0;i<c;i++) {
            Vector3f ray_d = cam.calcCameraRay(w, h, x, y);
            cur_x = x;
            cur_y = fb->height-1-y;
            if (x==15 && y==5) debug_ray = true;
            rayCast(Ray(cam.o, ray_d));
            if (x==w) {x=0;y++;}
            x++;
            debug_ray = false;
        }
    }
};

#endif
