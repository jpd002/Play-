#import "ApplicationDelegate.h"
#import "GSH_OpenGLMacOSX.h"
#import "Globals.h"
#import "../PS2OS.h"

using namespace std;

@implementation CApplicationDelegate

-(void)applicationDidFinishLaunching : (NSNotification*)notification
{
	g_virtualMachine->Initialize();
	[m_outputWindow setContentSize:NSMakeSize(640.0, 480.0)];
	NSOpenGLContext* context = [m_openGlView openGLContext];
	void* lowLevelContext = [context CGLContextObj];
	CGSH_OpenGLMacOSX::CreateGSHandler(*g_virtualMachine, reinterpret_cast<CGLContextObj>(lowLevelContext));
	
	//Check arguments
	NSArray* args = [[NSProcessInfo processInfo] arguments];
	
	NSString* bootFromElfPath = nil;
	if([args count] > 1)
	{
		bootFromElfPath = [args objectAtIndex:1];
	}
		
	if(bootFromElfPath != nil)
	{
		[self BootFromElf:bootFromElfPath];
	}
	
	//Initialize debugger
	[m_debuggerWindow Initialize];
}

-(void)OnBootElf : (id)sender
{
	NSOpenPanel* openPanel = [NSOpenPanel openPanel];
	NSArray* fileTypes = [NSArray arrayWithObject:@"elf"];
	if([openPanel runModalForTypes:fileTypes] != NSOKButton)
	{
		return;
	}
	NSString* fileName = [openPanel filename];
	[self BootFromElf:fileName];
}

-(void)OnPauseResume: (id)sender
{
	if(g_virtualMachine->GetStatus() == CVirtualMachine::RUNNING)
	{
		g_virtualMachine->Pause();	
	}
	else
	{
		g_virtualMachine->Resume();
	}
}

-(void)OnSaveState: (id)sender
{
	g_virtualMachine->SaveState("state.st0.zip");
}

-(void)OnLoadState: (id)sender
{
	g_virtualMachine->LoadState("state.st0.zip");
}

-(void)BootFromElf : (NSString*)fileName
{
	g_virtualMachine->Reset();
	try
	{
		CPS2OS* os = g_virtualMachine->m_os;
		os->BootFromFile([fileName fileSystemRepresentation]);
//		g_virtualMachine->Resume();
	}
	catch(const exception& excep)
	{
		NSString* errorMessage = [[NSString alloc] initWithCString:excep.what()];
		NSRunCriticalAlertPanel(@"Load ELF error:", errorMessage, NULL, NULL, NULL);
	}
}

@end
