#import <UIKit/UIKit.h>
#import "AppDelegate.h"
#include "DebuggerSimulator.h"
#include "DefaultAppConfig.h"

int main(int argc, char* argv[])
{
	StartSimulateDebugger();
	@autoreleasepool
	{
		return UIApplicationMain(argc, argv, nil, NSStringFromClass([AppDelegate class]));
	}
}
