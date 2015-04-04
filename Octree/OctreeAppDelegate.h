
#import <Cocoa/Cocoa.h>
#import "OctreeView.h"

@interface OctreeAppDelegate : NSObject <NSApplicationDelegate> {
@private
    NSWindow *window;
    OctreeView *oglview;

}

- (IBAction)foobar:(id)sender;

@property (assign) IBOutlet OctreeView *oglview;
@property (assign) IBOutlet NSWindow *window;

@end
