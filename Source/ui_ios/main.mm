#import <UIKit/UIKit.h>
#import "AppDelegate.h"
#include "DebuggerSimulator.h"
#include "DefaultAppConfig.h"
#include <stdlib.h>

int main(int argc, char* argv[])
{
	//DIAGNOSTIC: the Vulkan renderer used on iOS is CDrawDesktop, which does its
	//framebuffer read-modify-write in the fragment shader and relies on
	//VK_EXT_fragment_shader_interlock to keep overlapping fragments from racing.
	//MoltenVK implements that with Metal raster order groups, and SPIRV-Cross
	//attaches the raster_order_group(0) attribute through a different code path
	//when Metal argument buffers are in use. Argument buffers became the default
	//in MoltenVK 1.4.2, so turn them off to test whether the interlock is holding.
	//MoltenVK reads its configuration lazily on first use, so setting this before
	//UIApplicationMain is early enough.
	setenv("MVK_CONFIG_USE_METAL_ARGUMENT_BUFFERS", "0", 1);

	StartSimulateDebugger();
	@autoreleasepool
	{
		return UIApplicationMain(argc, argv, nil, NSStringFromClass([AppDelegate class]));
	}
}
