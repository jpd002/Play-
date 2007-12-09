#import "ApplicationDelegate.h"
#import "GSH_OpenGLMacOSX.h"
#import "Globals.h"

using namespace std;

@implementation CApplicationDelegate

-(void)applicationDidFinishLaunching : (NSNotification*)notification
{
	g_virtualMachine->Initialize();
	NSOpenGLContext* context = [m_openGlView openGLContext];
	void* lowLevelContext = [context CGLContextObj];
	CGSH_OpenGLMacOSX::CreateGSHandler(*g_virtualMachine, reinterpret_cast<CGLContextObj>(lowLevelContext));
}

-(void)BootElf : (id)sender
{
	NSOpenPanel* openPanel = [NSOpenPanel openPanel];
	NSArray* fileTypes = [NSArray arrayWithObject:@"elf"];
	if([openPanel runModalForTypes:fileTypes] != NSOKButton)
	{
		return;
	}
	NSString* fileName = [openPanel filename];
	g_virtualMachine->Reset();
	try
	{
		CPS2OS* os = g_virtualMachine->m_os;
		os->BootFromFile([fileName fileSystemRepresentation]);
		g_virtualMachine->Resume();
	}
	catch(const exception& excep)
	{
		NSString* errorMessage = [[NSString alloc] initWithCString:excep.what()];
		NSRunCriticalAlertPanel(@"Load ELF error:", errorMessage, NULL, NULL, NULL);
	}
}

@end
