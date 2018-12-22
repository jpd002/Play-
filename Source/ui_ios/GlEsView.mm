#import <QuartzCore/QuartzCore.h>
#import "GlEsView.h"

@implementation GlEsView

+(Class)layerClass
{
	return [CAEAGLLayer class];
}

-(void)initView
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

-(id)initWithFrame: (CGRect)frame
{
	self = [super initWithFrame: frame];
	if(self)
	{
		[self initView];
	}
	
	return self;
}

-(id)initWithCoder: (NSCoder*)decoder
{
	self = [super initWithCoder: decoder];
	if(self)
	{
		[self initView];
	}
	return self;
}

-(BOOL)hasRetinaDisplay
{
	return hasRetinaDisplay;
}

@end
