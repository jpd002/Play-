#include "PathUtils.h"
#import "EmulatorViewController.h"
#import "GlEsView.h"
#include "../PS2VM.h"
#include "../PS2VM_Preferences.h"
#include "../AppConfig.h"
#include "GSH_OpenGLiOS.h"
#include "IosUtils.h"
#include "PH_iOS.h"

CPS2VM* g_virtualMachine = nullptr;

@interface EmulatorViewController ()

@end

@implementation EmulatorViewController

-(void)viewDidLoad
{
	CGRect screenBounds = [[UIScreen mainScreen] bounds];
	
	auto view = [[GlEsView alloc] initWithFrame: screenBounds];
	self.view = view;
    
    self.connectObserver = [[NSNotificationCenter defaultCenter] addObserverForName:GCControllerDidConnectNotification object:nil queue:[NSOperationQueue mainQueue] usingBlock:^(NSNotification *note) {
        if ([[GCController controllers] count] == 1) {
            [self toggleHardwareController:YES];
        }
    }];
    self.disconnectObserver = [[NSNotificationCenter defaultCenter] addObserverForName:GCControllerDidDisconnectNotification object:nil queue:[NSOperationQueue mainQueue] usingBlock:^(NSNotification *note) {
        if (![[GCController controllers] count]) {
            [self toggleHardwareController:NO];
        }
    }];
    
    if ([[GCController controllers] count]) {
        [self toggleHardwareController:YES];
    }
    
    self.iCadeReader = [[iCadeReaderView alloc] init];
    [self.view addSubview:self.iCadeReader];
    self.iCadeReader.delegate = self;
    self.iCadeReader.active = YES;
}

-(void)viewDidAppear: (BOOL)animated
{
	g_virtualMachine = new CPS2VM();
	g_virtualMachine->Initialize();
	g_virtualMachine->CreateGSHandler(CGSH_OpenGLiOS::GetFactoryFunction((CAEAGLLayer*)self.view.layer));

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

- (void)toggleHardwareController:(BOOL)useHardware {
    if (useHardware) {
        self.gController = [GCController controllers][0];
        if (self.gController.gamepad) {
            [self.gController.gamepad.buttonA setValueChangedHandler:^(GCControllerButtonInput *button, float value, BOOL pressed) {
                
                auto padHandler = g_virtualMachine->GetPadHandler();
                if(padHandler)
                {
                    static_cast<CPH_iOS*>(padHandler)->SetButtonState(PS2::CControllerInfo::SQUARE, pressed);
                }
                
            }];
            [self.gController.gamepad.buttonB setValueChangedHandler:^(GCControllerButtonInput *button, float value, BOOL pressed) {
                
                auto padHandler = g_virtualMachine->GetPadHandler();
                if(padHandler)
                {
                    static_cast<CPH_iOS*>(padHandler)->SetButtonState(PS2::CControllerInfo::CIRCLE, pressed);
                }
                
            }];
            [self.gController.gamepad.buttonX setValueChangedHandler:^(GCControllerButtonInput *button, float value, BOOL pressed) {
                
                auto padHandler = g_virtualMachine->GetPadHandler();
                if(padHandler)
                {
                    static_cast<CPH_iOS*>(padHandler)->SetButtonState(PS2::CControllerInfo::CROSS, pressed);
                }
                
            }];
            [self.gController.gamepad.buttonY setValueChangedHandler:^(GCControllerButtonInput *button, float value, BOOL pressed) {
                
                auto padHandler = g_virtualMachine->GetPadHandler();
                if(padHandler)
                {
                    static_cast<CPH_iOS*>(padHandler)->SetButtonState(PS2::CControllerInfo::TRIANGLE, pressed);
                }
                
            }];
            [self.gController.gamepad.dpad setValueChangedHandler:^(GCControllerDirectionPad *dpad, float xValue, float yValue){
                auto padHandler = g_virtualMachine->GetPadHandler();
                if(padHandler)
                {
                    static_cast<CPH_iOS*>(padHandler)->SetAxisState(PS2::CControllerInfo::ANALOG_LEFT_X, xValue);
                    static_cast<CPH_iOS*>(padHandler)->SetAxisState(PS2::CControllerInfo::ANALOG_LEFT_Y, yValue);
                }
            }];
            //Add controller pause handler here
        }
        if (self.gController.extendedGamepad) {
            [self.gController.extendedGamepad.buttonA setValueChangedHandler:^(GCControllerButtonInput *button, float value, BOOL pressed) {
                
                auto padHandler = g_virtualMachine->GetPadHandler();
                if(padHandler)
                {
                    static_cast<CPH_iOS*>(padHandler)->SetButtonState(PS2::CControllerInfo::SQUARE, pressed);
                }
                
            }];
            [self.gController.extendedGamepad.buttonB setValueChangedHandler:^(GCControllerButtonInput *button, float value, BOOL pressed) {
                
                auto padHandler = g_virtualMachine->GetPadHandler();
                if(padHandler)
                {
                    static_cast<CPH_iOS*>(padHandler)->SetButtonState(PS2::CControllerInfo::CIRCLE, pressed);
                }
                
            }];
            [self.gController.extendedGamepad.buttonX setValueChangedHandler:^(GCControllerButtonInput *button, float value, BOOL pressed) {
                
                auto padHandler = g_virtualMachine->GetPadHandler();
                if(padHandler)
                {
                    static_cast<CPH_iOS*>(padHandler)->SetButtonState(PS2::CControllerInfo::CROSS, pressed);
                }
                
            }];
            [self.gController.extendedGamepad.buttonY setValueChangedHandler:^(GCControllerButtonInput *button, float value, BOOL pressed) {
                
                auto padHandler = g_virtualMachine->GetPadHandler();
                if(padHandler)
                {
                    static_cast<CPH_iOS*>(padHandler)->SetButtonState(PS2::CControllerInfo::TRIANGLE, pressed);
                }
                
            }];
            [self.gController.extendedGamepad.dpad setValueChangedHandler:^(GCControllerDirectionPad *dpad, float xValue, float yValue){
                auto padHandler = g_virtualMachine->GetPadHandler();
                if(padHandler)
                {
                    static_cast<CPH_iOS*>(padHandler)->SetAxisState(PS2::CControllerInfo::ANALOG_LEFT_X, xValue);
                    static_cast<CPH_iOS*>(padHandler)->SetAxisState(PS2::CControllerInfo::ANALOG_LEFT_Y, yValue);
                }
            }];
            [self.gController.extendedGamepad.leftThumbstick.xAxis setValueChangedHandler:^(GCControllerAxisInput *axis, float value){
                auto padHandler = g_virtualMachine->GetPadHandler();
                if(padHandler)
                {
                    static_cast<CPH_iOS*>(padHandler)->SetAxisState(PS2::CControllerInfo::ANALOG_LEFT_X, value);
                }
            }];
            [self.gController.extendedGamepad.leftThumbstick.yAxis setValueChangedHandler:^(GCControllerAxisInput *axis, float value){
                auto padHandler = g_virtualMachine->GetPadHandler();
                if(padHandler)
                {
                    static_cast<CPH_iOS*>(padHandler)->SetAxisState(PS2::CControllerInfo::ANALOG_LEFT_Y, value);
                }
            }];
        }
    } else {
        self.gController = nil;
    }
}

-(BOOL)prefersStatusBarHidden
{
	return YES;
}

-(UIInterfaceOrientationMask)supportedInterfaceOrientations
{
	return UIInterfaceOrientationMaskLandscape;
}

- (void)dealloc
{
    [[NSNotificationCenter defaultCenter] removeObserver:self.connectObserver];
    [[NSNotificationCenter defaultCenter] removeObserver:self.disconnectObserver];
}

@end
