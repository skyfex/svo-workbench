//
//  fpgalink.h
//  Octree
//
//  Created by Audun Wilhelmsen on 09.06.12.
//  Copyright (c) 2012 NTNU. All rights reserved.
//

#ifndef Octree_fpgalink_h
#define Octree_fpgalink_h
extern "C" {
    void fl_upload(const char *file_path, unsigned int address, bool verify, bool swap);
    void fl_init();
}
#endif
