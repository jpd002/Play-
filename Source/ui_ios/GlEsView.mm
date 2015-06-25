#import <QuartzCore/QuartzCore.h>
#import "GlEsView.h"

@implementation GlEsView

+(Class)layerClass
{
	return [CAEAGLLayer class];
}

-(id)initWithFrame: (CGRect)frame
{
	self = [super initWithFrame: frame];
	if(self)
	{
		hasRetinaDisplay = NO;
		if([[UIScreen mainScreen] respondsToSelector: NSSelectorFromString(@"scale")])
		{
			if([self respondsToSelector: NSSelectorFromString(@"contentScaleFactor")])
			{
				float scale = [[UIScreen mainScreen] scale];
				self.contentScaleFactor = scale;
				if(scale == 2.0f)
				{
					hasRetinaDisplay = YES;
				}
			}
		}
		
		CAEAGLLayer* eaglLayer = (CAEAGLLayer*)self.layer;
		
		eaglLayer.opaque = TRUE;
		eaglLayer.drawableProperties = [NSDictionary dictionaryWithObjectsAndKeys:
										[NSNumber numberWithBool: FALSE], kEAGLDrawablePropertyRetainedBacking,
										kEAGLColorFormatRGBA8, kEAGLDrawablePropertyColorFormat,
										nil];
	}
	
	return self;
}

-(BOOL)hasRetinaDisplay
{
	return hasRetinaDisplay;
}

@end
