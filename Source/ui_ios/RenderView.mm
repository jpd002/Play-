#import "RenderView.h"
#import <QuartzCore/QuartzCore.h>
#include "../AppConfig.h"
#include "PreferenceDefs.h"

@implementation RenderView

+ (Class)layerClass
{
	auto gsHandlerId = CAppConfig::GetInstance().GetPreferenceInteger(PREFERENCE_VIDEO_GS_HANDLER);
	switch(gsHandlerId)
	{
	default:
		[[fallthrough]];
	case PREFERENCE_VALUE_VIDEO_GS_HANDLER_OPENGL:
		return [CAEAGLLayer class];
	case PREFERENCE_VALUE_VIDEO_GS_HANDLER_VULKAN:
		return [CAMetalLayer class];
	}
}

- (void)initView
{
	hasRetinaDisplay = NO;
	if([[UIScreen mainScreen] respondsToSelector:NSSelectorFromString(@"scale")])
	{
		if([self respondsToSelector:NSSelectorFromString(@"contentScaleFactor")])
		{
			float scale = [[UIScreen mainScreen] scale];
			self.contentScaleFactor = scale;
			if(scale == 2.0f)
			{
				hasRetinaDisplay = YES;
			}
		}
	}

	self.layer.opaque = TRUE;

	auto gsHandlerId = CAppConfig::GetInstance().GetPreferenceInteger(PREFERENCE_VIDEO_GS_HANDLER);
	if(gsHandlerId == PREFERENCE_VALUE_VIDEO_GS_HANDLER_OPENGL)
	{
		CAEAGLLayer* layer = (CAEAGLLayer*)self.layer;
		layer.drawableProperties = [NSDictionary dictionaryWithObjectsAndKeys:
		                                             [NSNumber numberWithBool:FALSE], kEAGLDrawablePropertyRetainedBacking,
		                                             kEAGLColorFormatRGBA8, kEAGLDrawablePropertyColorFormat,
		                                             nil];
	}
}

- (id)initWithFrame:(CGRect)frame
{
	self = [super initWithFrame:frame];
	if(self)
	{
		[self initView];
	}

	return self;
}

- (id)initWithCoder:(NSCoder*)decoder
{
	self = [super initWithCoder:decoder];
	if(self)
	{
		[self initView];
	}
	return self;
}

- (BOOL)hasRetinaDisplay
{
	return hasRetinaDisplay;
}

@end
