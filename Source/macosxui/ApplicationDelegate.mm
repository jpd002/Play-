#import "ApplicationDelegate.h"
#import "VfsManagerController.h"
#import "GSH_OpenGLMacOSX.h"
#import "PH_HidMacOSX.h"
#import "Globals.h"
#import "../PS2OS.h"

using namespace std;

@implementation CApplicationDelegate

-(void)applicationDidFinishLaunching : (NSNotification*)notification
{
	g_virtualMachine->Initialize();
	[m_outputWindow setContentSize:NSMakeSize(640.0, 448.0)];
	[m_outputWindow center];
	NSOpenGLContext* context = [m_openGlView openGLContext];
	void* lowLevelContext = [context CGLContextObj];
	g_virtualMachine->CreateGSHandler(CGSH_OpenGLMacOSX::GetFactoryFunction(reinterpret_cast<CGLContextObj>(lowLevelContext)));
	g_virtualMachine->CreatePadHandler(CPH_HidMacOSX::GetFactoryFunction());
#ifdef _DEBUG	
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
#endif
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

-(void)OnBootCdrom0: (id)sender
{
	[self BootFromCdrom0];
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
	if(g_virtualMachine->LoadState("state.st0.zip"))
	{
		NSRunCriticalAlertPanel(@"Error occured while trying to load state.", @"", NULL, NULL, NULL);
	}
}

-(void)OnVfsManager : (id)sender
{
	[[CVfsManagerController defaultController] showManager];
}

-(void)BootFromElf : (NSString*)fileName
{
	g_virtualMachine->Reset();
	try
	{
		CPS2OS* os = g_virtualMachine->m_os;
		os->BootFromFile([fileName fileSystemRepresentation]);
#ifndef DEBUGGER_INCLUDED
		g_virtualMachine->Resume();
#endif
	}
	catch(const exception& excep)
	{
		NSString* errorMessage = [[NSString alloc] initWithCString:excep.what()];
		NSRunCriticalAlertPanel(@"Load ELF error:", errorMessage, NULL, NULL, NULL);
	}
}

-(void)BootFromCdrom0
{
	g_virtualMachine->Reset();
	try
	{
		CPS2OS* os = g_virtualMachine->m_os;
		os->BootFromCDROM();
#ifndef DEBUGGER_INCLUDED
		g_virtualMachine->Resume();
#endif
	}
	catch(const exception& excep)
	{
		NSString* errorMessage = [[NSString alloc] initWithCString:excep.what()];
		NSRunCriticalAlertPanel(@"Load ELF error:", errorMessage, NULL, NULL, NULL);
	}
}

@end
