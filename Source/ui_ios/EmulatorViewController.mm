#include "PathUtils.h"
#import "EmulatorViewController.h"
#import "GlEsView.h"
#include "../PS2VM.h"
#include "../PS2VM_Preferences.h"
#include "../AppConfig.h"
#include "GSH_OpenGLiOS.h"
#include "IosUtils.h"

CPS2VM* g_virtualMachine = nullptr;

@interface EmulatorViewController ()

@end

@implementation EmulatorViewController

-(void)viewDidLoad
{
	CGRect screenBounds = [[UIScreen mainScreen] bounds];
	
	auto view = [[GlEsView alloc] initWithFrame: screenBounds];
	self.view = view;
	
	g_virtualMachine = new CPS2VM();
	g_virtualMachine->Initialize();
	g_virtualMachine->CreateGSHandler(CGSH_OpenGLiOS::GetFactoryFunction((CAEAGLLayer*)view.layer));

	g_virtualMachine->Pause();
	g_virtualMachine->Reset();

	if(IosUtils::IsLoadableExecutableFileName(self.imagePath))
	{
		g_virtualMachine->m_ee->m_os->BootFromFile([self.imagePath UTF8String]);
	}
	else
	{
		CAppConfig::GetInstance().SetPreferenceString(PS2VM_CDROM0PATH, [self.imagePath UTF8String]);
		g_virtualMachine->Reset();
		g_virtualMachine->m_ee->m_os->BootFromCDROM(CPS2OS::ArgumentList());
	}

	g_virtualMachine->Resume();
}

-(BOOL)prefersStatusBarHidden
{
	return YES;
}

@end
