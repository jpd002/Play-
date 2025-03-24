#include "LayerFromView.h"
#include <AppKit/AppKit.h>

void* GetLayerFromView(void* viewPtr)
{
	NSView* view = (NSView*)viewPtr;
	return [view layer];
}
