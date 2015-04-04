
#ifndef Octree_Octree_h
#define Octree_Octree_h

#include <OpenGL/gl.h>
#include <math.h>
#include <stdlib.h>
#include <stdint.h>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <assert.h>

#include "vmath.h"
#include "glutils.h"
#include "binvox.h"

#import "Framebuffer.h"
#include "Shared.h"

using namespace std;



enum NodeType { NT_EMPTY, NT_PARTIAL, NT_FILLED };

struct Node {
    NodeType type;
    Node *children[8];
    int childAdr;
    int farAdr;
    int nid;
};

struct CNode { // Compact node
//    int nid; // For debugging purposes
    union {
        struct {
            UInt8 valid_mask;
            UInt8 leaf_mask;
            UInt16 child_ptr;
        };
        UInt32 far_ptr;
    };
} __attribute__((packed));


enum CacheType {
    CTDirectMapped,
    CTTwoWaySetAssoc,
    CTFourWaySetAssoc
};

class Cache
{
public:
    int *tags;
    int *flags;
    int size;
    int blockSize;
    CacheType type;
    int hits;
    int misses;
    int maxlevel;
    
    Cache(int size, int blockSize, CacheType type) : blockSize(blockSize), type(type)
    {
        this->size = size/blockSize;
        tags = (int*)malloc(sizeof(int*)*size);
        flags = (int*)malloc(sizeof(int*)*size);
        maxlevel = 30;
        reset();
    }

    void reset()
    {
        hits = 0;
        misses = 0;
        for (int i=0;i<size;i++) {
            tags[i] = -1;
            flags[i] = 0;
        }
    }
    
    void request(int adr, int level)
    {
        if (level>maxlevel) {
            misses++;
            return;
        }
        adr = adr/blockSize;
        int slot;
        int setSize;
        if (type==CTDirectMapped) {
            slot = adr % size;
            if (tags[slot] == adr)
                hits++;
            else
                misses++;
        }
        else if (type==CTTwoWaySetAssoc) {
            setSize = size/2;
            for (int i=0;i<2;i++) {
                slot = (adr % setSize)+setSize*i;
                if (tags[slot] == adr) {
                    flags[adr%setSize] = (i+1)%2;
                    break;
                }                            
            }
            if (tags[slot] == adr) {
                hits++;
            }
            else {
                misses++;
                slot = (adr % setSize)+setSize*flags[adr%setSize];
                flags[adr%setSize] = (flags[adr%setSize]+1)%2;
            }
        }
        else if (type==CTFourWaySetAssoc) {
            setSize = size/4;
            for (int i=0;i<4;i++) {
                slot = (adr % setSize)+setSize*i;
                if (tags[slot] == adr) {
                    break;
                }                            
            }
            if (tags[slot] == adr) {
                hits++;
            }
            else {
                misses++;
                int max = -1;
                for (int i=0;i<4;i++) {
                    int checkSlot = (adr % setSize)+setSize*i;
                    if (flags[checkSlot] > max) {
                        max = flags[checkSlot];
                        slot = checkSlot;
                    }
                }
            }
            for (int j=0;j<4;j++) {
                int otherSlot = (adr % setSize)+setSize*j;
                flags[otherSlot]++;
            }
            flags[slot] = 1;
        }
        
        tags[slot] = adr;

    }
};


struct OctreeCursor {

    CNode *data;
    int dataSize;
    CNode *node;
    Node *refNode;
    int adr;
    int level;
    Cache *cache;
    
    
    OctreeCursor getChild(int child)
    {
        OctreeCursor cur;
        cur.data = data;
        cur.dataSize = dataSize;
        cur.cache = cache;
        cur.level = level+1;
        
        int childIdx=0;
        for (int i=0; i<child; i++) {
            if (childIsPartial(i)) {
                childIdx++;
            }
        }
        
        int childPtr = node->child_ptr & ~(1<<15);
        if (node->child_ptr & (1<<15)) {
            int farAdr = adr+childPtr;
            if (cache) cache->request(farAdr, level);
            assert(farAdr < dataSize);
            CNode *farEntry = &data[farAdr];
            childPtr = farEntry->far_ptr;
        }
        int childAdr = adr + childPtr + childIdx;
        if (cache) cache->request(childAdr, level);
        assert(childAdr < dataSize);

        cur.node = &data[childAdr];
        if (refNode)
            cur.refNode = refNode->children[child];
        else
            cur.refNode = NULL;
//        assert(cur.refNode->nid == childNode->nid);
        cur.adr = childAdr;
        return cur;
    }
    
    bool childPtrIsFar()
    {
        return node->child_ptr & (1<<15);
    }
    int childPtr()
    {
        return node->child_ptr & ~(1<<15); 
    }
    int farPtr()
    {
        if (!childPtrIsFar()) return 0;
        int childPtr = node->child_ptr & ~(1<<15);
        int farAdr = adr+childPtr;
        CNode *farEntry = &data[farAdr];
        childPtr = farEntry->far_ptr;
        return childPtr;
    }
    
    bool childIsFilled(int child)
    {
        return (node->leaf_mask & (1<<child));
    }
    
    bool childIsPartial(int child)
    {
        return (node->valid_mask & (1<<child)) && !(node->leaf_mask & (1<<child));
    }
    
    bool childIsValid(int child)
    {
        return (node->valid_mask & (1<<child));   
    }
    
    bool childIsEmpty(int child)
    {
        return !(node->valid_mask & (1<<child));
    }
} ;

class Octree {
    
private:
    UInt64 _nodeSize;
    UInt64 _shortPtrSize;
    UInt64 _shortPtrTop;
    UInt64 _longPtrSize;
    UInt64 _longPtrTop;
        
public:
    
    Node* rootNode;
    CNode* data;
    int dataSize;
    int nextNid;
    
    Octree()
    {   
        _nodeSize = 4;
        _shortPtrSize = 2;
        _longPtrSize = 4;
        _shortPtrTop = 1 << 15;
        _longPtrTop  = 1l << 32l;
        rootNode = NULL;
        data = NULL;
        dataSize = NULL;
    }
    
    void generateChildCoords(Vector3f children[8], Vector3f p, float size)
    {
        children[0] = Vector3f(p.x, p.y, p.z);
        children[1] = Vector3f(p.x, p.y, p.z+size);
        children[2] = Vector3f(p.x, p.y+size, p.z);
        children[3] = Vector3f(p.x, p.y+size, p.z+size);
        children[4] = Vector3f(p.x+size, p.y, p.z);
        children[5] = Vector3f(p.x+size, p.y, p.z+size);
        children[6] = Vector3f(p.x+size, p.y+size, p.z);
        children[7] = Vector3f(p.x+size, p.y+size, p.z+size);
    }
    
    void writeToFile(ofstream *file)
    {
        file->write((char*)data, dataSize*sizeof(CNode));
    }
    void readFromFile(ifstream *file)
    {
        file->seekg(0, std::ios::end);
        int filesize = file->tellg();
        file->seekg(0);
        dataSize = filesize / sizeof(CNode);
        data = (CNode*)malloc(dataSize * sizeof(CNode));
        file->read((char*)data, dataSize*sizeof(CNode));
        assert(file->gcount() == dataSize*sizeof(CNode));
    }
    
    
    // ----
    
    OctreeCursor newRootCursor()
    {
        OctreeCursor cur;
        cur.data = data;
        cur.dataSize = dataSize;
        cur.node = data;
        cur.refNode = rootNode;
        cur.adr = 0;
        cur.level = 0;
        return cur;
    }
    
    
    // ----
    
    void compactTree()
    {
        rootNode->farAdr = 0;
        rootNode->childAdr = 0;
        dataSize = compactAllocate(rootNode, 1);
        data = (CNode*)malloc(dataSize*sizeof(CNode));
        compactNode(rootNode, dataSize);
    }
    

    
    void compactNode(Node *node, int pos)
    {
        int childAdr = node->childAdr;
        for (int i=0;i<8;i++) {
            Node *child = node->children[i];
            if (child && child->type==NT_PARTIAL) {
                compactNode(child, childAdr);
                childAdr--;
            }
        }
        assert(pos<=dataSize);
        
        CNode *cnode = &data[dataSize-pos];
//        cnode->nid = node->nid;
        cnode->valid_mask = 0;
        cnode->leaf_mask = 0;
        
        for (int i=0;i<8;i++) {
            if (node->children[i]) {
                cnode->valid_mask |= (1<<i);
                if (node->children[i]->type == NT_FILLED)
                    cnode->leaf_mask |= (1<<i);
            }
        }
        
        int childPtr = pos - node->childAdr;
        
        if (node->farAdr) {
            int farPtr = pos-node->farAdr;
            assert(farPtr < _shortPtrTop);
            CNode *farPtrEntry = &data[dataSize-node->farAdr]; 
            farPtrEntry->far_ptr = childPtr;
            cnode->child_ptr = (1<<15) | farPtr;
        }
        else {
            cnode->child_ptr = childPtr;
        }
        
    }
    
    int compactAllocate(Node *node, int pos)
    {  
        // Example 
        // 1: Node 1
        // 2: Node 2
        // -----------
        // 3: FarPtr (-> #0)
        // 4: Node 3 (Far -> #2) 
        // 5: Node 4
        // -----------
        // 6: This Node (-> #4)
        
        for (int i=0;i<8;i++) {
            Node *child = node->children[i];
            if (child) {
                child->farAdr = 0;
                child->childAdr = 0;
            }
            if (child && child->type==NT_PARTIAL) {
                pos = compactAllocate(child, pos);
            }
        }
        
        
        for (int i=0;i<8;i++) {
            Node *child = node->children[i];
            if (child && child->childAdr) {
                int gchildAdrMax = pos - child->childAdr + 16;
                if (gchildAdrMax > _shortPtrTop) {
                    child->farAdr = pos;
                    pos++;
                }
            }
        }
        
        for (int i=0;i<8;i++) {
            Node *child = node->children[i];
            if (child && child->type==NT_PARTIAL) {
                node->childAdr = pos;
                pos++;
            }
        }
        
        
        return pos;
    }
    
    // ---
    
    Node* readFromBinvox_rec(unsigned char *voxels, int voxels_width, int x, int y, int z, int width)
    {
        Node *node = new Node;
        if (width==1) {
            if (voxels[y + z*voxels_width + x*voxels_width*voxels_width]) {
                // Solid Leaf node
                node->type = NT_FILLED;
                node->nid = nextNid++;
                memset(node->children, 0, sizeof(node->children));
                return node;
            }
            else {
                delete node;
                return NULL;
            }
        }
        int child_width = width/2;
        Node** children = node->children;
        children[0] = readFromBinvox_rec(voxels,voxels_width, x, y, z, child_width);
        children[1] = readFromBinvox_rec(voxels,voxels_width, x, y, z+child_width, child_width);
        children[2] = readFromBinvox_rec(voxels,voxels_width, x, y+child_width, z, child_width);
        children[3] = readFromBinvox_rec(voxels,voxels_width, x, y+child_width, z+child_width, child_width);
        children[4] = readFromBinvox_rec(voxels,voxels_width, x+child_width, y, z, child_width);
        children[5] = readFromBinvox_rec(voxels,voxels_width, x+child_width, y, z+child_width, child_width);
        children[6] = readFromBinvox_rec(voxels,voxels_width, x+child_width, y+child_width, z, child_width);
        children[7] = readFromBinvox_rec(voxels,voxels_width, x+child_width, y+child_width, z+child_width, child_width);
        bool all_empty = true;
        bool all_filled = true;
        for (int i=0;i<8;i++) {
            if (children[i]) {
                if (children[i]->type != NT_FILLED)
                    all_filled = false;
                all_empty = false;
            }
            else {
                all_filled = false;
            }
        }
        if (all_empty) {
            delete node;
            return NULL;
        }
        else {
            if (all_filled) {
                for (int i=0;i<8;i++) {
                    delete children[i];
                    children[i] = NULL;
                }
                node->type = NT_FILLED;
            }
            else
                node->type = NT_PARTIAL;
            node->nid = nextNid++;
            return node;
        }
    }
    
    void readFromBinvox(ifstream *file)
    {
        nextNid = 0;
        unsigned char *voxels = NULL;
        int width = read_binvox(file, &voxels);
        
        if (!width) {
            return;
        }
        printf("Voxel model width: %d\n", width);
        
        rootNode = readFromBinvox_rec(voxels, width, 0, 0, 0, width);
                
        compactTree();
        printf("Data size: %d\n", dataSize);

        
        free(voxels);
    }
    
    // ----
    
    bool isBuilt()
    {
        return (data!=NULL);
    }
    
    void clear()
    {
        
    }
    
};

#endif