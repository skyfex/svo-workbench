

#import "OctreeView.h"
#import "NSOpenGLView-imageFromView.h"
#import "OpenGL/glu.h"
#import "glutils.h"
#include <math.h>

#include <unistd.h>
#include <fcntl.h>
#include <termios.h>

#include "fpgalink.h"

#include "ply.h"

#define ROOT "/Users/skyfex/svo-workbench"

static Model_PLY *dragon;
static float dragon_scale = 4.93046;
static Vector3f dragon_translate = Vector3f(-0.107585,0.0528441,-0.049836);
static Model_PLY *bunny;
static float bunny_scale = 6.43061;
static Vector3f bunny_translate = Vector3f(-0.0945716, 0.0333891, -0.0618736);
static Model_PLY *happy;
static float happy_scale = 5.06275;

static float _near = 0.1;
static float _far = 8;
static float _left = -1.333*_near;
static float _right = 1.333*_near;
static float _bottom = -_near;
static float _top = _near;


@implementation OctreeView


- (void)awakeFromNib
{
    shouldSaveImage = NO;
    octree = new Octree();
    framebuffer = new Framebuffer(640,480);
    raytracer = new OctreeSWRaytracer(octree, framebuffer);
//    raytracer->cache = new Cache(128, 1, CTDirectMapped);
//    raytracer->cache = new Cache(64, CTDirectMapped);
    gl_renderer = new OctreeGL(octree);
    
    // These PLY models can be downloaded from
    // https://graphics.stanford.edu/data/3Dscanrep/
    dragon = new Model_PLY();
    dragon->Load(ROOT"/data/dragon.ply");
    bunny = new Model_PLY();
    bunny->Load(ROOT"/data/bunny2.ply");
    happy = new Model_PLY();
    happy->Load(ROOT"/data/happy2.ply");
    
    // This voxel model (generated from above ply files) should be included in the repository
    ifstream file(ROOT"/data/dragon1024.bin", ios::in|ios::binary);
    octree->readFromFile(&file);
    
    file.close();
    
    [self update:self];
}

- (void)prepareOpenGL
{
    glShadeModel(GL_SMOOTH);
    glEnable(GL_NORMALIZE);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_COLOR_MATERIAL);
    
    glClearColor(1,1,1, 0.0f);
//    glClearColor(0.2f, 0.1f, 0.2f, 0.0f);
    
    glDisable(GL_LIGHTING);
    
    glEnable(GL_LIGHT0);
    GLfloat light_diffuse[] = { 1, 1, 1, 1 };
    glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
//    GLfloat light_position[] = { 0, 0, 5, 1 };
//    glLightfv(GL_LIGHT0, GL_POSITION, light_position);

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);   
	

}

- (void)reshape
{

}

-(void)drawHappy
{
    glPushMatrix();
    glTranslatef(0.3, 0, 0.7);
    float s = happy_scale;
    glScalef(s, s, s);
    glColor3f(0.5,0.7,1);
    happy->Draw();
    glPopMatrix();
    
}

-(void)drawDragon
{
    glPushMatrix();
//    glTranslatef(0.3, 0, 0.7);
    float s = dragon_scale;
    glScalef(2,2,2);
    glTranslatef(-0.5,-0.5,-0.5);
    glScalef(s, s, s);
    glTranslatef(-dragon_translate.x, -dragon_translate.y, -dragon_translate.z);
    glColor3f(1,0.7,0.5);
    dragon->Draw();
    glPopMatrix();
}

-(void)drawOctree
{
    glPushMatrix();
//    glTranslatef(0.3, 0, 0.7);
    glScalef(0.5,0.5,0.5);
    glTranslatef(1,1,1);
    glColor3f(1,0.5,0.7);
    gl_renderer->render();
    glPopMatrix();    
}

-(void)drawBunny
{
    glPushMatrix();
    
    glTranslatef(-0.5, 0, -1);
    glScalef(2,2,2);
    glTranslatef(-0.8,-0.8,-0.8);
    float s = bunny_scale;
    glScalef(s, s, s);
    glTranslatef(-bunny_translate.x, -bunny_translate.y, -bunny_translate.z);

    glColor3f(0.5,0.7,1);
    bunny->Draw();
    
    glPopMatrix();
}

-(void)drawScene
{
    
//    glAxisIndicators(1);
    
    float lpos[4] = {0,0,5,1};
    glLightfv(GL_LIGHT0, GL_POSITION, lpos);
//    glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
    glEnable(GL_LIGHTING);
    
//    [self drawDragon];
    
    glPushMatrix();
    
    float rot = [camRotateSlider floatValue];
    glTranslatef(0, 0, 0.2);
    glRotatef(rot, 0, 1, 0);
    glTranslatef(0, 0, -0.8);
    glScalef(0.7,0.7,0.7);

//    [self drawBunny];
    glPopMatrix();
    
    glScalef(2,2,2);
    glTranslatef(0,0.5,0);
    glDisable(GL_LIGHTING);
    glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
    [self drawBunny];
    
    glTranslatef(0,-1,0);
//    [self drawHappy];
    
    glDisable(GL_LIGHTING);
}

- (void)viewTransform
{
    float rot = [rotateSlider floatValue];
    float rad = 2;
    glRotatef(rot, 0, 1, 0);
    glTranslatef(0,0,1);
    glTranslatef(rad*sin(rot/180*M_PI),0,-rad*cos(rot/180*M_PI));
//    glTranslatef(0,0,rot/180*8);
}

- (void)octreeTransform
{
    glScalef([scaleSlider floatValue], [scaleSlider floatValue], [scaleSlider floatValue]);
}

-(void)drawView1
{

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    
    float pmat[16];    
    glMakePTransform(pmat, _left, _right, _bottom, _top, _near, _far);
    glLoadMatrixf(pmat);
    
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    
    [self viewTransform];
    
    [self drawScene];
    
//    [self drawOctree];
}

-(void)drawView2
{

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    
    float pmat[16];    
    glMakePTransform(pmat, _left, _right, _bottom, _top, _near, _far*4);
    glLoadMatrixf(pmat);
    
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    
    
    float pmat1[16];    
    glMakePTransform2(pmat, _left, _right, _bottom, _top, _near, _far);

    float fade = [newSlider floatValue];
    for (int x=0;x<4;x++) {
        for (int y=0;y<4;y++) {
            int i = x*4 + y;
            if (x==y) {
                pmat1[i] = (1-fade) + pmat1[i]*fade;
            }
            else {
                pmat1[i] = pmat1[i]*fade;
            }
        }
    }
 
    float rot = [rotateSlider floatValue];

    
    glTranslatef(0,0,-2);
    glRotatef(0,1,0,0);
    glRotatef(80,0,1,0);
    
    glPushMatrix();

    glMultMatrixf(pmat1);

    glPushMatrix();

    float rad = 1.2;
    glRotatef(rot, 0, 1, 0);
    glTranslatef(rad*sin(rot/180*M_PI),-0.5,-rad*cos(rot/180*M_PI));
    
    [self drawScene];
    
    glPopMatrix();
    
    // Render camera
    glColor3f(1,0,0);

    glLine(Vector3f(0,0,0),Vector3f(-2*_far,-_far,-_far));
    glLine(Vector3f(0,0,0),Vector3f(2*_far,-_far,-_far));
    glLine(Vector3f(0,0,0),Vector3f(-2*_far,_far,-_far));
    glLine(Vector3f(0,0,0),Vector3f(2*_far,_far,-_far));
    
    // Draw far plane
    glColor4f(0.5, 0.5, 1, 0.3);
    
    glQuad(Vector3f(2*_far,_far,-_far), 
           Vector3f(-2*_far,_far,-_far), 
           Vector3f(-2*_far,-_far,-_far), 
           Vector3f(2*_far,-_far,-_far));
    
    glColor4f(0.5, 1, 0.5, 0.1);
    
    // Draw near plane
    glQuad(Vector3f(-2*_near,_near,-_near),
           Vector3f(-2*_near,-_near,-_near),
           Vector3f(2*_near,-_near,-_near),
           Vector3f(2*_near,_near,-_near));
    
    
    glPopMatrix();

//    glColor3f(1,0,0);
//    glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
//    glAxisCube(Vector3f(-1,-1,-1), Vector3f(1,1,1));
//    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  
}

-(void)drawView3
{
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    
    float pmat[16];    
    glMakePTransform(pmat, _left, _right, _bottom, _top, _near, _far);
//    glLoadMatrixf(pmat);
    
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    
    [self drawDragon];
    
//    float rot = [rotateSlider floatValue];
//    float rad = 2.5;
//    glRotatef(rot, 0, 1, 0);
//    glTranslatef(rad*sin(rot/180*M_PI),-0.5,-rad*cos(rot/180*M_PI));
//    
//    glColor3f(0,1,0);
//
//    gl_renderer->render();
//    
//    glColor3f(1,0,0);
//    glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
//    glAxisCube(Vector3f(-1,-1,-1), Vector3f(1,1,1));
//    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

-(void)drawView4
{

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    
    glMakePTransform(raytracer->glProjectionMat.data, _left, _right, _bottom, _top, _near, _far);
    glMakeIPTransform(raytracer->projectionMat.data, _left, _right, _bottom, _top, _near, _far);

    [self viewTransform];
    [self octreeTransform];
        
    Matrix4f mvMat;
    
    glGetFloatv(GL_MODELVIEW, mvMat.data);
    
    raytracer->lightPos = Vector4f(cosf([newSlider floatValue])*2,2,sinf([newSlider floatValue])*2,1);
    
    raytracer->modelMat = mvMat.inverse();
    
    framebuffer->clear();
    
    raytracer->render();
    
    framebuffer->flush();
}

-(void)drawView5
{
    
}

-(void)drawRect:(NSRect)dirtyRect
{
    double glTime;
    double rcTime;
    NSDate *start;
    
    const NSSize viewSize = self.bounds.size;
    float portWidth = viewSize.width;
    float portHeight = viewSize.height;
    
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    glViewport(0,0,portWidth,portHeight);
    glWindowPos2i(0,0);

    if (drawSVO.state) {
    start = [NSDate date];
        [self drawView4];
    rcTime = [start timeIntervalSinceNow]*-1;
    }

    
//    glPixelZoom(0.5,0.5);
    
    if (drawPolygon.state) {
        [self drawView1];
    }
    
    if (showZbuffer.state) {
        float *zbuffer = (float*)malloc(sizeof(float)*portWidth*portHeight);
        glReadPixels(0, 0, portWidth, portHeight, GL_DEPTH_COMPONENT, GL_FLOAT, zbuffer);
    //    glWindowPos2i(portWidth,0);
        glDrawPixels(portWidth, portHeight, GL_LUMINANCE, GL_FLOAT, zbuffer);
        free(zbuffer);
    }
    
//    glViewport(portWidth,0,portWidth,portHeight);
//    [self drawView2];    
    
//    glViewport(0,portHeight,portWidth,portHeight);
//    [self drawView3];    
//
//    glViewport(portWidth,portHeight,portWidth,portHeight);
//    glWindowPos2i(portWidth,portHeight);
//    [self drawView4]; 
    
    if (raytracer->cache) {
    cacheHitText.stringValue = [NSString stringWithFormat: @"%d", raytracer->cache->hits];
    cacheMissText.stringValue = [NSString stringWithFormat: @"%d", raytracer->cache->misses];
    cacheHitMissText.stringValue = [NSString stringWithFormat: @"%f", 
                                    (float)raytracer->cache->hits / (float) raytracer->cache->misses];
    }
    [rcTimeField setStringValue: [NSString stringWithFormat: @"%.0f ms / %.2f fps", rcTime*1000, 1/rcTime]];

    
    if (shouldSaveImage) {
        NSImage *image = [self imageFromView];
//        NSBitmapImageRep *rep = [[image representations] objectAtIndex: 0];
        NSData *data = [image TIFFRepresentation]; ///[rep TIFFRepresentation];
        [data writeToFile: @"/octree/octree.tiff" atomically: YES];
        shouldSaveImage = NO;
    }
    
    [[self openGLContext] flushBuffer];
}


- (IBAction)update:(id)sender
{
    gl_renderer->terminLevel = [slider intValue];
    raytracer->terminLevel = [slider intValue];

    [renderLevelText setIntValue: [slider intValue]];
    [self setNeedsDisplay:YES];
}
- (IBAction)rebuild:(id)sender
{

    [self setNeedsDisplay:YES];

}
- (IBAction)save:(id)sender
{
    ofstream file ("/octree/octree.bin", ios::out|ios::binary);
    octree->writeToFile(&file);
    file.close();    
}
- (IBAction)load:(id)sender
{
    NSOpenPanel *dlg = [NSOpenPanel openPanel];
    [dlg setCanChooseFiles: YES];
    [dlg setDirectory: @"/octree"];
    [dlg setAllowedFileTypes: [NSArray arrayWithObjects: @"bin", nil]];
    if ([dlg runModal] == NSOKButton) {
        NSString *filename = [dlg filename];
        
        ifstream file([filename cString], ios::in|ios::binary);
        octree->readFromFile(&file);
        
        file.close();
        [self setNeedsDisplay:YES];
    }
    
//    ifstream file("/octree/octree.bin", ios::in|ios::binary);
//    octree->readFromFile(&file);
//    
//    file.close();
//    [self setNeedsDisplay:YES];

}

- (IBAction)loadBinVox:(id)sender
{
    NSOpenPanel *dlg = [NSOpenPanel openPanel];
    [dlg setCanChooseFiles: YES];
    [dlg setDirectory: @"/octree"];
    [dlg setAllowedFileTypes: [NSArray arrayWithObjects: @"binvox", nil]];
    if ([dlg runModal] == NSOKButton) {
        NSString *filename = [dlg filename];
        
        ifstream file([filename cString], ios::in|ios::binary);
        octree->readFromBinvox(&file);
        
        file.close();
        [self setNeedsDisplay:YES];
    }
}

- (IBAction)saveRayData:(id)sender
{

//    ofstream file ("/octree/raydata.bin", ios::out|ios::binary);
//    float r;    
//    for (r=-170; r<180; r+= 360/10) {
//        [rotateSlider setFloatValue: r];
    
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        
        glMakePTransform(raytracer->glProjectionMat.data, _left, _right, _bottom, _top, _near, _far);
        glMakeIPTransform(raytracer->projectionMat.data, _left, _right, _bottom, _top, _near, _far);
        
        [self viewTransform];
        [self octreeTransform];
        
        Matrix4f mvMat;
        
        glGetFloatv(GL_MODELVIEW, mvMat.data);
        
        raytracer->modelMat = mvMat.inverse();



//        raytracer->exportRayData(&file, 640, 480);
//    }    
//    int stop = 0;
//    file.write((char*)&stop, 4);
    
//    ofstream file ("/octree/raydata.bin", ios::out|ios::binary);
//    raytracer->exportRayData(&file, 0x04800000, 640, 480);
    
    ofstream file ("/octree/raydata_sim.bin", ios::out|ios::binary);
    raytracer->exportRayData(&file, 32, 24);
    
//    ofstream file ("/octree/raydata_sim.bin", ios::out|ios::binary);
//    raytracer->exportRayData(&file, 0x00600000, 32, 24);
    
    file.close();        
}

- (IBAction)saveImage:(id)sender
{
//    NSRect windowFrame = [window frame];
//    NSRect frame = [self frame];
//    NSRect newframe = NSMakeRect(0, 0, 2048, 1024);
//    [[self window] setFrame: newframe display:YES];
//    [self setFrame: newframe];
    shouldSaveImage = YES;
    [self setNeedsDisplay: YES];
}

- (IBAction)cacheProgram:(id)sender
{
    int config[][4] = {
        
        
        {8 , 1, 2, CTTwoWaySetAssoc},
        {32 , 1, 2, CTTwoWaySetAssoc},
        {128, 1, 2, CTTwoWaySetAssoc},
        {512, 1, 2, CTTwoWaySetAssoc},
        {8 , 1, 4, CTTwoWaySetAssoc},
        {32 , 1, 4, CTTwoWaySetAssoc},
        {128, 1, 4, CTTwoWaySetAssoc},
        {512, 1, 4, CTTwoWaySetAssoc},
        {8 , 1, 6, CTTwoWaySetAssoc},
        {32 , 1, 6, CTTwoWaySetAssoc},
        {128, 1, 6, CTTwoWaySetAssoc},
        {512, 1, 6, CTTwoWaySetAssoc},
        {8 , 1, 8, CTTwoWaySetAssoc},
        {32 , 1, 8, CTTwoWaySetAssoc},
        {128, 1, 8, CTTwoWaySetAssoc},
        {512, 1, 8, CTTwoWaySetAssoc}

        
//        {32,  1, 20, CTDirectMapped},
//        {128, 1, 20, CTDirectMapped},
//        {512, 1, 20, CTDirectMapped},
//        {32,  2, 20, CTDirectMapped},
//        {128, 2, 20, CTDirectMapped},
//        {512, 2, 20, CTDirectMapped},
//        {32,  4, 20, CTDirectMapped},
//        {128, 4, 20, CTDirectMapped},
//        {512, 4, 20, CTDirectMapped},
//        {32,  8, 20, CTDirectMapped},
//        {128, 8, 20, CTDirectMapped},
//        {512, 8, 20, CTDirectMapped},
//        
//        {32,  1, 20, CTTwoWaySetAssoc},
//        {128, 1, 20, CTTwoWaySetAssoc},
//        {512, 1, 20, CTTwoWaySetAssoc},
//        {32,  2, 20, CTTwoWaySetAssoc},
//        {128, 2, 20, CTTwoWaySetAssoc},
//        {512, 2, 20, CTTwoWaySetAssoc},
//        {32,  4, 20, CTTwoWaySetAssoc},
//        {128, 4, 20, CTTwoWaySetAssoc},
//        {512, 4, 20, CTTwoWaySetAssoc},
//        {32,  8, 20, CTTwoWaySetAssoc},
//        {128, 8, 20, CTTwoWaySetAssoc},
//        {512, 8, 20, CTTwoWaySetAssoc},
//        
//        {32,  1, 20, CTFourWaySetAssoc},
//        {128, 1, 20, CTFourWaySetAssoc},
//        {512, 1, 20, CTFourWaySetAssoc},
//        {32,  2, 20, CTFourWaySetAssoc},
//        {128, 2, 20, CTFourWaySetAssoc},
//        {512, 2, 20, CTFourWaySetAssoc},
//        {32,  4, 20, CTFourWaySetAssoc},
//        {128, 4, 20, CTFourWaySetAssoc},
//        {512, 4, 20, CTFourWaySetAssoc},
//        {32,  8, 20, CTFourWaySetAssoc},
//        {128, 8, 20, CTFourWaySetAssoc},
//        {512, 8, 20, CTFourWaySetAssoc}
    };
    for (int i=0;i<sizeof(config)/sizeof(config[0]);i++) {
        int size = config[i][0];
        int block = config[i][1];
        int maxlevels = config[i][2];
        int type = CTDirectMapped;//config[i][3];
        raytracer->cache = new Cache(size, block, (CacheType)type);
        raytracer->cache->maxlevel = maxlevels;
//        printf("Cache size: %d\nBlock size: %d\nCache type: %s\n", size, block,
//               type==CTDirectMapped?"Direct mapped":type==CTTwoWaySetAssoc?"Two way set assoc.":"Four way set assoc.");
        int totalHits = 0;
        int totalMiss = 0;
        for (float a=0;a<=360;a+=90) {
            [rotateSlider setFloatValue: a];
            [self drawView4];
            int hits = raytracer->cache->hits; totalHits+=hits;
            int miss = raytracer->cache->misses; totalMiss+=miss;
            float missrate = (float)miss/(float)(hits+miss);
//            printf("Angle: %f; Hits: %d; Misses: %d; Missrate: %f\n", a, hits, miss, missrate);
        }
//        printf("Avg missrate: %f\n\n", (float)totalMiss/(float)(totalMiss+totalHits));
        printf("\\cd{%.0f}", (float)totalMiss/(float)(totalMiss+totalHits)*100.0);
        if (i%4==3) printf(" \\\\ \n"); else printf(" & ");
    }
}

int
set_interface_attribs (int fd, int speed, int parity)
{
    struct termios tty;
    memset (&tty, 0, sizeof tty);
    if (tcgetattr (fd, &tty) != 0)
    {
        throw;
    }
    
    cfsetospeed (&tty, speed);
    cfsetispeed (&tty, speed);
    
    tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;     // 8-bit chars
    // disable IGNBRK for mismatched speed tests; otherwise receive break
    // as \000 chars
    tty.c_iflag &= ~IGNBRK;         // ignore break signal
    tty.c_lflag = 0;                // no signaling chars, no echo,
    // no canonical processing
    tty.c_oflag = 0;                // no remapping, no delays
    tty.c_cc[VMIN]  = 0;            // read doesn't block
    tty.c_cc[VTIME] = 5;            // 0.5 seconds read timeout
    
    tty.c_iflag &= ~(IXON | IXOFF | IXANY); // shut off xon/xoff ctrl
    
    tty.c_cflag |= (CLOCAL | CREAD);// ignore modem controls,
    // enable reading
    tty.c_cflag &= ~(PARENB | PARODD);      // shut off parity
    tty.c_cflag |= parity;
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CRTSCTS;
    
    if (tcsetattr (fd, TCSANOW, &tty) != 0)
    {
        throw;
    }
    return 0;
}

void
set_blocking (int fd, int should_block)
{
    struct termios tty;
    memset (&tty, 0, sizeof tty);
    if (tcgetattr (fd, &tty) != 0)
    {
        throw;
    }
    
    tty.c_cc[VMIN]  = should_block ? 1 : 0;
    tty.c_cc[VTIME] = 5;            // 0.5 seconds read timeout
    
    if (tcsetattr (fd, TCSANOW, &tty) != 0)
        throw;
}


- (IBAction)push:(id)sender
{
//    printf("Saving Ray Data");
    [self saveRayData: self];
     
    int fdx = open ("/octree/signal", O_CREAT);
    close(fdx);
    
    while ((fdx = open ("/octree/signal", O_RDONLY)) >= 0)
    {
        close(fdx);
        usleep(10000);
    }
    
    printf("\n---\n");

    int fd;

    fd = open( "/dev/tty.usbmodemfa1311", O_RDWR | O_NOCTTY );
    if (fd < 0)
    {
        throw;
    }
    set_interface_attribs (fd, B115200, 0);  // set speed to 115,200 bps, 8n1 (no parity)
    set_blocking (fd, 1);    
    
    printf("----\n");
    
    usleep(1000);

    write (fd, "vx", 2); 

    sleep(1);
    
    printf("-----\n");
    
    char buf [1024];
    int n = read (fd, buf, (sizeof buf)); 
    if (n>0)
        write(1, buf, n);
    
    printf("\n--\n");
    
    close(fd);
}

@end
