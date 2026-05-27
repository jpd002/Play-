#include "LayerFromView.h"
#include <AppKit/AppKit.h>
#include <QuartzCore/QuartzCore.h>

void* GetLayerFromView(void* viewPtr)
{
	NSView* view = (NSView*)viewPtr;
	id layer = [view layer];
	// More recent versions of Qt uses a different type of layer (QContentLayer)
	// and does not provide a CAMetalLayer directly. Handle both possibilities.
	if([layer isKindOfClass:[CAMetalLayer class]])
	{
		return layer;
	}
	return [layer contentLayer];
}
