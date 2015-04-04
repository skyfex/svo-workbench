

#import "OctreeAppDelegate.h"

@implementation OctreeAppDelegate

@synthesize window;
@synthesize oglview;

- (IBAction)foobar:(id)sender {
	[oglview setNeedsDisplay: YES];
}

- (void)applicationDidFinishLaunching:(NSNotification *)aNotification
{

}

@end
