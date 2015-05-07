#import "ApplicationDelegate.h"
#import "VfsManagerController.h"
#import "GSH_OpenGLMacOSX.h"
#import "PH_HidMacOSX.h"
#import "Globals.h"
#import "../ee/PS2OS.h"
#import "../PS2VM_Preferences.h"
#import "../AppConfig.h"

@implementation ApplicationDelegate

-(void)applicationDidFinishLaunching: (NSNotification*)notification
{
	g_virtualMachine->Initialize();
	
	outputWindowController = [[OutputWindowController alloc] initWithWindowNibName: @"OutputWindow"];
	[outputWindowController.window setContentSize: NSMakeSize(640.0, 448.0)];
	[outputWindowController.window center];
	[outputWindowController showWindow: nil];
	[outputWindowController setDelegate: self];
	
	NSOpenGLContext* context = [outputWindowController.openGlView openGLContext];
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
	
	//if(bootFromElfPath != nil)
	//{
	//	[self BootFromElf:bootFromElfPath];
	//}
#endif
}

-(void)applicationDidBecomeActive: (NSNotification*)notification
{
	[self updatePresentationParams];
}

-(void)outputWindowDidResize: (NSSize)size
{
	[self updatePresentationParams];
}

-(void)applicationWillTerminate: (NSNotification*)notification
{
	g_virtualMachine->Pause();
}

-(IBAction)bootElfMenuSelected: (id)sender
{
	NSOpenPanel* openPanel = [NSOpenPanel openPanel];
	NSArray* fileTypes = [NSArray arrayWithObject: @"elf"];
	openPanel.allowedFileTypes = fileTypes;
	openPanel.canChooseDirectories = NO;
	if([openPanel runModal] != NSModalResponseOK)
	{
		return;
	}
	NSURL* url = openPanel.URL;
	NSString* filePath = [url path];
	[self bootFromElf: filePath];
}

-(IBAction)bootDiskImageMenuSelected: (id)sender
{
	NSOpenPanel* openPanel = [NSOpenPanel openPanel];
	NSArray* fileTypes = [NSArray arrayWithObjects: @"iso", @"isz", @"cso", nil];
	openPanel.allowedFileTypes = fileTypes;
	openPanel.canChooseDirectories = NO;
	if([openPanel runModal] != NSModalResponseOK)
	{
		return;
	}
	NSURL* url = openPanel.URL;
	NSString* filePath = [url path];
	CAppConfig::GetInstance().SetPreferenceString(PS2VM_CDROM0PATH, [filePath UTF8String]);
	
	[self bootFromCdrom0];
}

-(IBAction)bootCdrom0MenuSelected: (id)sender
{
	[self bootFromCdrom0];
}

-(IBAction)fitToScreenMenuSelected: (id)sender
{
	CAppConfig::GetInstance().SetPreferenceInteger(PREF_CGSHANDLER_PRESENTATION_MODE, CGSHandler::PRESENTATION_MODE_FIT);
	[self updatePresentationParams];
}

-(IBAction)fillScreenMenuSelected: (id)sender
{
	CAppConfig::GetInstance().SetPreferenceInteger(PREF_CGSHANDLER_PRESENTATION_MODE, CGSHandler::PRESENTATION_MODE_FILL);
	[self updatePresentationParams];
}

-(IBAction)actualSizeMenuSelected: (id)sender
{
	CAppConfig::GetInstance().SetPreferenceInteger(PREF_CGSHANDLER_PRESENTATION_MODE, CGSHandler::PRESENTATION_MODE_ORIGINAL);
	[self updatePresentationParams];
}

-(IBAction)fullScreenMenuSelected: (id)sender
{
	[outputWindowController.window toggleFullScreen: nil];
}

-(IBAction)pauseResumeMenuSelected: (id)sender
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

-(IBAction)saveStateMenuSelected: (id)sender
{
	g_virtualMachine->SaveState("state.st0.zip");
}

-(IBAction)loadStateMenuSelected: (id)sender
{
	if(g_virtualMachine->LoadState("state.st0.zip"))
	{
		NSRunCriticalAlertPanel(@"Error occured while trying to load state.", @"", NULL, NULL, NULL);
	}
}

-(IBAction)vfsManagerMenuSelected: (id)sender
{
	[[VfsManagerController defaultController] showManager];
}

-(void)bootFromElf : (NSString*)fileName
{
	g_virtualMachine->Pause();
	g_virtualMachine->Reset();
	try
	{
		CPS2OS* os = g_virtualMachine->m_ee->m_os;
		os->BootFromFile([fileName fileSystemRepresentation]);
		[self updateTitle];
		g_virtualMachine->Resume();
	}
	catch(const std::exception& exception)
	{
		NSString* errorMessage = [[NSString alloc] initWithUTF8String: exception.what()];
		NSRunCriticalAlertPanel(@"Load ELF error:", errorMessage, NULL, NULL, NULL);
	}
}

-(void)bootFromCdrom0
{
	g_virtualMachine->Pause();	
	g_virtualMachine->Reset();
	try
	{
		CPS2OS* os = g_virtualMachine->m_ee->m_os;
		os->BootFromCDROM(CPS2OS::ArgumentList());
		[self updateTitle];
		g_virtualMachine->Resume();
	}
	catch(const std::exception& exception)
	{
		NSString* errorMessage = [[NSString alloc] initWithUTF8String: exception.what()];
		NSRunCriticalAlertPanel(@"Load ELF error:", errorMessage, NULL, NULL, NULL);
	}
}

-(void)updateTitle
{
	CPS2OS* os = g_virtualMachine->m_ee->m_os;
	auto executableName = os->GetExecutableName();
	outputWindowController.window.title = [NSString stringWithFormat: @"Play! - %s", executableName];
}

-(BOOL)validateUserInterfaceItem: (id<NSValidatedUserInterfaceItem>)item
{
	bool hasElf = g_virtualMachine->m_ee->m_os->GetELF() != NULL;
	auto presentationMode = static_cast<CGSHandler::PRESENTATION_MODE>(CAppConfig::GetInstance().GetPreferenceInteger(PREF_CGSHANDLER_PRESENTATION_MODE));
	if(
	   item == pauseResumeMenuItem ||
	   item == loadStateMenuItem ||
	   item == saveStateMenuItem)
	{
		return hasElf;
	}
	if(item.action == @selector(fitToScreenMenuSelected:))
	{
		[(NSMenuItem*)item setState: (presentationMode == CGSHandler::PRESENTATION_MODE_FIT) ? NSOnState : NSOffState];
	}
	if(item.action == @selector(fillScreenMenuSelected:))
	{
		[(NSMenuItem*)item setState: (presentationMode == CGSHandler::PRESENTATION_MODE_FILL) ? NSOnState : NSOffState];
	}
	if(item.action == @selector(actualSizeMenuSelected:))
	{
		[(NSMenuItem*)item setState: (presentationMode == CGSHandler::PRESENTATION_MODE_ORIGINAL) ? NSOnState : NSOffState];
	}
	return YES;
}

-(void)updatePresentationParams
{
	auto presentationMode = static_cast<CGSHandler::PRESENTATION_MODE>(CAppConfig::GetInstance().GetPreferenceInteger(PREF_CGSHANDLER_PRESENTATION_MODE));
	NSSize contentSize = [outputWindowController contentSize];
	CGSHandler::PRESENTATION_PARAMS presentationParams;
	presentationParams.windowWidth = static_cast<unsigned int>(contentSize.width);
	presentationParams.windowHeight = static_cast<unsigned int>(contentSize.height);
	presentationParams.mode = presentationMode;
	g_virtualMachine->m_ee->m_gs->SetPresentationParams(presentationParams);
	g_virtualMachine->m_ee->m_gs->Flip();
}

@end
