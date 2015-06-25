#include "PathUtils.h"
#import "MainViewController.h"
#import "GlEsView.h"
#include "../PS2VM.h"
#include "GSH_OpenGLiOS.h"

CPS2VM* g_virtualMachine = nullptr;

@interface MainViewController ()

@end

@implementation MainViewController

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

	auto filePath = Framework::PathUtils::GetPersonalDataPath() / boost::filesystem::path("plasma_tunnel.elf");
	g_virtualMachine->m_ee->m_os->BootFromFile(filePath.string().c_str());
	
	g_virtualMachine->Resume();
}

@end
