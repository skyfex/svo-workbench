
#import <Cocoa/Cocoa.h>

#ifdef __cplusplus
#include "Framebuffer.h"
#include "Octree.h"
#include "OctreeSWRaytracer.h"
#include "OctreeGL.h"
#endif

@interface OctreeView : NSOpenGLView {
    
#ifdef __cplusplus
    Framebuffer *framebuffer;
    Octree *octree;
    OctreeSWRaytracer *raytracer;
    OctreeGL *gl_renderer;
#else
    void *framebuffer;
    void *octree;
    void *raytracer;
    void *gl_renderer;
#endif
    bool shouldSaveImage;
    
    IBOutlet NSSlider *slider;
    IBOutlet NSTextField *renderLevelText;
    IBOutlet NSSlider *rotateSlider;
    IBOutlet NSSlider *camRotateSlider;
    IBOutlet NSSlider *newSlider;
    IBOutlet NSSlider *scaleSlider;
    IBOutlet NSTextField *sizeField;
    IBOutlet NSTextField *glTimeField;
    IBOutlet NSTextField *rcTimeField;
    
    IBOutlet NSTextField *buildLevelText;
    
    IBOutlet NSTextField *cacheMissText;
    IBOutlet NSTextField *cacheHitText;
    IBOutlet NSTextField *cacheHitMissText;
    
    IBOutlet NSButton *drawSVO;
    IBOutlet NSButton *drawPolygon;
    IBOutlet NSButton *showZbuffer;
    

}

- (IBAction)update:(id)sender;
- (IBAction)rebuild:(id)sender;
- (IBAction)save:(id)sender;
- (IBAction)load:(id)sender;
- (IBAction)loadBinVox:(id)sender;
- (IBAction)saveImage:(id)sender;
- (IBAction)saveRayData:(id)sender;
- (IBAction)cacheProgram:(id)sender;
- (IBAction)push:(id)sender;

@end
