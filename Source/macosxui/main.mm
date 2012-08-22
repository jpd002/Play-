#import <Cocoa/Cocoa.h>
#import "Globals.h"

int main(int argc, char *argv[])
{
	g_virtualMachine = new CPS2VM();
	return NSApplicationMain(argc, (const char **)argv);
}
