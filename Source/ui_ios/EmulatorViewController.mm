#include "PathUtils.h"
#import "EmulatorViewController.h"
#import "GlEsView.h"
#include "../PS2VM.h"
#include "../PS2VM_Preferences.h"
#include "../AppConfig.h"
#include "PreferenceDefs.h"
#include "GSH_OpenGLiOS.h"
#include "IosUtils.h"
#include "PH_Generic.h"

CPS2VM* g_virtualMachine = nullptr;

@interface EmulatorViewController ()

@end

@implementation EmulatorViewController

+(void)registerPreferences
{
	CAppConfig::GetInstance().RegisterPreferenceBoolean(PREFERENCE_UI_SHOWFPS, false);
	CAppConfig::GetInstance().RegisterPreferenceBoolean(PREFERENCE_UI_SHOWVIRTUALPAD, true);
}

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

	g_virtualMachine->CreatePadHandler(CPH_Generic::GetFactoryFunction());
	
	const CGRect screenBounds = [[UIScreen mainScreen] bounds];

	if(CAppConfig::GetInstance().GetPreferenceBoolean(PREFERENCE_UI_SHOWVIRTUALPAD))
	{
		auto padHandler = static_cast<CPH_Generic*>(g_virtualMachine->GetPadHandler());
		self.virtualPadView = [[VirtualPadView alloc] initWithFrame: screenBounds padHandler: padHandler];
		[self.view addSubview: self.virtualPadView];
	}

	if(CAppConfig::GetInstance().GetPreferenceBoolean(PREFERENCE_UI_SHOWFPS))
	{
		[self setupFpsCounter];
	}

	UIButton* but = [UIButton buttonWithType: UIButtonTypeRoundedRect];
	[but setTitle: @"Save" forState: UIControlStateNormal];
	[but setBackgroundColor: [UIColor whiteColor]];
	[but addTarget: self action: @selector(onSaveStateButtonClick) forControlEvents: UIControlEventTouchUpInside];
	but.frame = CGRectMake(screenBounds.size.width - 50, (screenBounds.size.height - 25) / 2, 50, 25);
	[self.view addSubview: but];
	
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
		g_virtualMachine->m_ee->m_os->BootFromCDROM();
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
                    static_cast<CPH_Generic*>(padHandler)->SetButtonState(PS2::CControllerInfo::SQUARE, pressed);
                }
                
            }];
            [self.gController.gamepad.buttonB setValueChangedHandler:^(GCControllerButtonInput *button, float value, BOOL pressed) {
                
                auto padHandler = g_virtualMachine->GetPadHandler();
                if(padHandler)
                {
                    static_cast<CPH_Generic*>(padHandler)->SetButtonState(PS2::CControllerInfo::CIRCLE, pressed);
                }
                
            }];
            [self.gController.gamepad.buttonX setValueChangedHandler:^(GCControllerButtonInput *button, float value, BOOL pressed) {
                
                auto padHandler = g_virtualMachine->GetPadHandler();
                if(padHandler)
                {
                    static_cast<CPH_Generic*>(padHandler)->SetButtonState(PS2::CControllerInfo::CROSS, pressed);
                }
                
            }];
            [self.gController.gamepad.buttonY setValueChangedHandler:^(GCControllerButtonInput *button, float value, BOOL pressed) {
                
                auto padHandler = g_virtualMachine->GetPadHandler();
                if(padHandler)
                {
                    static_cast<CPH_Generic*>(padHandler)->SetButtonState(PS2::CControllerInfo::TRIANGLE, pressed);
                }
                
            }];
            [self.gController.gamepad.dpad setValueChangedHandler:^(GCControllerDirectionPad *dpad, float xValue, float yValue){
                auto padHandler = g_virtualMachine->GetPadHandler();
                if(padHandler)
                {
                    static_cast<CPH_Generic*>(padHandler)->SetAxisState(PS2::CControllerInfo::ANALOG_LEFT_X, xValue);
                    static_cast<CPH_Generic*>(padHandler)->SetAxisState(PS2::CControllerInfo::ANALOG_LEFT_Y, yValue);
                }
            }];
            //Add controller pause handler here
        }
        if (self.gController.extendedGamepad) {
            [self.gController.extendedGamepad.buttonA setValueChangedHandler:^(GCControllerButtonInput *button, float value, BOOL pressed) {
                
                auto padHandler = g_virtualMachine->GetPadHandler();
                if(padHandler)
                {
                    static_cast<CPH_Generic*>(padHandler)->SetButtonState(PS2::CControllerInfo::SQUARE, pressed);
                }
                
            }];
            [self.gController.extendedGamepad.buttonB setValueChangedHandler:^(GCControllerButtonInput *button, float value, BOOL pressed) {
                
                auto padHandler = g_virtualMachine->GetPadHandler();
                if(padHandler)
                {
                    static_cast<CPH_Generic*>(padHandler)->SetButtonState(PS2::CControllerInfo::CIRCLE, pressed);
                }
                
            }];
            [self.gController.extendedGamepad.buttonX setValueChangedHandler:^(GCControllerButtonInput *button, float value, BOOL pressed) {
                
                auto padHandler = g_virtualMachine->GetPadHandler();
                if(padHandler)
                {
                    static_cast<CPH_Generic*>(padHandler)->SetButtonState(PS2::CControllerInfo::CROSS, pressed);
                }
                
            }];
            [self.gController.extendedGamepad.buttonY setValueChangedHandler:^(GCControllerButtonInput *button, float value, BOOL pressed) {
                
                auto padHandler = g_virtualMachine->GetPadHandler();
                if(padHandler)
                {
                    static_cast<CPH_Generic*>(padHandler)->SetButtonState(PS2::CControllerInfo::TRIANGLE, pressed);
                }
                
            }];
            [self.gController.extendedGamepad.dpad setValueChangedHandler:^(GCControllerDirectionPad *dpad, float xValue, float yValue){
                auto padHandler = g_virtualMachine->GetPadHandler();
                if(padHandler)
                {
                    static_cast<CPH_Generic*>(padHandler)->SetAxisState(PS2::CControllerInfo::ANALOG_LEFT_X, xValue);
                    static_cast<CPH_Generic*>(padHandler)->SetAxisState(PS2::CControllerInfo::ANALOG_LEFT_Y, yValue);
                }
            }];
            [self.gController.extendedGamepad.leftThumbstick.xAxis setValueChangedHandler:^(GCControllerAxisInput *axis, float value){
                auto padHandler = g_virtualMachine->GetPadHandler();
                if(padHandler)
                {
                    static_cast<CPH_Generic*>(padHandler)->SetAxisState(PS2::CControllerInfo::ANALOG_LEFT_X, value);
                }
            }];
            [self.gController.extendedGamepad.leftThumbstick.yAxis setValueChangedHandler:^(GCControllerAxisInput *axis, float value){
                auto padHandler = g_virtualMachine->GetPadHandler();
                if(padHandler)
                {
                    static_cast<CPH_Generic*>(padHandler)->SetAxisState(PS2::CControllerInfo::ANALOG_LEFT_Y, value);
                }
            }];
        }
    } else {
        self.gController = nil;
    }
}

-(void)setupFpsCounter
{
	auto screenBounds = [[UIScreen mainScreen] bounds];

	self.fpsCounterLabel = [[UILabel alloc] initWithFrame: screenBounds];
	self.fpsCounterLabel.textColor = [UIColor whiteColor];
	[self.view addSubview: self.fpsCounterLabel];

	g_virtualMachine->GetGSHandler()->OnNewFrame.connect(
		[self] (uint32 drawCallCount)
		{
			@synchronized(self)
			{
				self.frames++;
				self.drawCallCount += drawCallCount;
			}
		}
	);
	
	[NSTimer scheduledTimerWithTimeInterval: 1 target: self selector: @selector(updateFpsCounter) userInfo: nil repeats: YES];
}

-(void)updateFpsCounter
{
	@synchronized(self)
	{
		uint32 dcpf = (self.frames != 0) ? (self.drawCallCount / self.frames) : 0;
		self.fpsCounterLabel.text = [NSString stringWithFormat: @"%d f/s, %d dc/f",
			self.frames, dcpf];
		self.frames = 0;
		self.drawCallCount = 0;
	}
	[self.fpsCounterLabel sizeToFit];
}

-(void)onSaveStateButtonClick
{
	auto dataPath = Framework::PathUtils::GetPersonalDataPath();
	auto statePath = dataPath / "state.sta";
	g_virtualMachine->SaveState(statePath.c_str());
	NSLog(@"Saved state to '%s'.", statePath.string().c_str());
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
