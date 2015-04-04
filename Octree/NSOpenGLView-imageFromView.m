//
//  NSOpenGLView-imageFromView.m
//
//	Category for NSOpenGLView to return an image of the view.
//
//  Based on http://lists.apple.com/archives/Mac-opengl/2002/Jun/msg00128.html
//  with the addition of bitmap vertical flip. It is not enough to set image isFlipped
//  when we paste the image into other applications.
//
//  In-place XOR is used to swap rows of the bitmap.  This is cute and perhaps slower
//  that it needs to be.  Interesting to test one day.

#import "NSOpenGLView-imageFromView.h"

#import <OpenGL/gl.h>

@implementation NSOpenGLView ( image )

static void memxor(unsigned char *dst, unsigned char *src, unsigned int bytes)
{
    while (bytes--) *dst++ ^= *src++;
}

static void memswap(unsigned char *a, unsigned char *b, unsigned int bytes)
{
    memxor(a, b, bytes);
    memxor(b, a, bytes);
    memxor(a, b, bytes);
}


- (NSImage *) imageFromView
{
	NSRect bounds;
	int height, width, row, bytesPerRow;
	NSBitmapImageRep *imageRep;
	unsigned char *bitmapData;
	NSImage *image;
	
	bounds = [self bounds];
	
	height = bounds.size.height;
	width = bounds.size.width;
	
	imageRep = [[NSBitmapImageRep alloc] initWithBitmapDataPlanes: NULL
                                                       pixelsWide: width
                                                       pixelsHigh: height
                                                    bitsPerSample: 8
                                                  samplesPerPixel: 4
                                                         hasAlpha: YES
                                                         isPlanar: NO
                                                   colorSpaceName: NSCalibratedRGBColorSpace
                                                      bytesPerRow: 0				// indicates no empty bytes at row end
                                                     bitsPerPixel: 0];
    
	[[self openGLContext] makeCurrentContext];
    
	bitmapData = [imageRep bitmapData];
    
	bytesPerRow = [imageRep bytesPerRow];
	
	glPixelStorei(GL_PACK_ROW_LENGTH, 8*bytesPerRow/[imageRep bitsPerPixel]);
    
	glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, bitmapData);
    
	// Flip the bitmap vertically to account for OpenGL coordinate system difference
	// from NSImage coordinate system.
	
	for (row = 0; row < height/2; row++)
	{
		unsigned char *a, *b;
		
		a = bitmapData + row * bytesPerRow;
		b = bitmapData + (height - 1 - row) * bytesPerRow;
		
		memswap(a, b, bytesPerRow);
	}
    
	// Create the NSImage from the bitmap
    
	image = [[[NSImage alloc] initWithSize: NSMakeSize(width, height)] autorelease];
	[image addRepresentation: imageRep];
	
	// Release memory
	
	[imageRep release];
    
	// Previously we did not flip the bitmap, and instead did [image setFlipped:YES];
	// This does not work properly (i.e., the image remained inverted) when pasting 
	// the image to AppleWorks or GraphicConvertor.
    
	return image;
}

@end