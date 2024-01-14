#include "PathUtils.h"
#import "EmulatorViewController.h"
#import "SaveStateViewController.h"
#import "SettingsViewController.h"
#import "RenderView.h"
#include "../PS2VM.h"
#include "../PS2VM_Preferences.h"
#include "../AppConfig.h"
#include "PreferenceDefs.h"
#include "GSH_OpenGLiOS.h"
#ifdef HAS_GSH_VULKAN
#include "GSH_VulkaniOS.h"
#endif
#include "../ui_shared/BootablesProcesses.h"
#include "PH_Generic.h"
#include "../../tools/PsfPlayer/Source/SH_OpenAL.h"
#include "../ui_shared/StatsManager.h"

CPS2VM* g_virtualMachine = nullptr;
CGSHandler::NewFrameEvent::Connection g_gsNewFrameConnection;
#ifdef PROFILE
CPS2VM::NewFrameEvent::Connection g_newFrameConnection;
#endif

@interface EmulatorViewController () <SaveStateDelegate>
@property(nonatomic, strong) SaveStateViewController* saveStateViewController;
@end

@implementation EmulatorViewController

+ (void)registerPreferences
{
	CAppConfig::GetInstance().RegisterPreferenceBoolean(PREFERENCE_UI_SHOWFPS, false);
	CAppConfig::GetInstance().RegisterPreferenceBoolean(PREFERENCE_UI_SHOWVIRTUALPAD, true);
	CAppConfig::GetInstance().RegisterPreferenceBoolean(PREFERENCE_AUDIO_ENABLEOUTPUT, true);
	CAppConfig::GetInstance().RegisterPreferenceInteger(PREFERENCE_VIDEO_GS_HANDLER, PREFERENCE_VALUE_VIDEO_GS_HANDLER_OPENGL);
	CAppConfig::GetInstance().RegisterPreferenceInteger(PREFERENCE_UI_VIRTUALPADOPACITY, 100);
	CAppConfig::GetInstance().RegisterPreferenceBoolean(PREFERENCE_UI_HIDEVIRTUALPAD_CONTROLLER_CONNECTED, true);
	CAppConfig::GetInstance().RegisterPreferenceBoolean(PREFERENCE_UI_VIRTUALPAD_HAPTICFEEDBACK, true);
}

- (void)viewDidLoad
{
	[[NSNotificationCenter defaultCenter] addObserverForName:UIApplicationWillResignActiveNotification
	                                                  object:nil
	                                                   queue:[NSOperationQueue mainQueue]
	                                              usingBlock:^(NSNotification* note) {
		                                            [self appResignedActive];
	                                              }];

	[[NSNotificationCenter defaultCenter] addObserverForName:UIApplicationDidBecomeActiveNotification
	                                                  object:nil
	                                                   queue:[NSOperationQueue mainQueue]
	                                              usingBlock:^(NSNotification* note) {
		                                            [self appBecameActive];
	                                              }];

	self.iCadeReader = [[iCadeReaderView alloc] init];
	[self.view addSubview:self.iCadeReader];
	self.iCadeReader.delegate = self;
	self.iCadeReader.active = YES;

	self.saveStateViewController = [[UIStoryboard storyboardWithName:@"Main" bundle:nil] instantiateViewControllerWithIdentifier:@"SaveStateViewController"];
	self.saveStateViewController.delegate = self;
	[self addChildViewController:self.saveStateViewController];
	[self.saveStateViewController didMoveToParentViewController:self];
	[self.view addSubview:self.saveStateViewController.view];
	self.saveStateViewController.view.hidden = YES;
	self.saveStateViewController.view.translatesAutoresizingMaskIntoConstraints = false;
	[[self.saveStateViewController.view.topAnchor constraintEqualToAnchor:self.view.topAnchor] setActive:YES];
	[[self.saveStateViewController.view.bottomAnchor constraintEqualToAnchor:self.view.bottomAnchor] setActive:YES];
	[[self.saveStateViewController.view.leadingAnchor constraintEqualToAnchor:self.view.leadingAnchor] setActive:YES];
	[[self.saveStateViewController.view.trailingAnchor constraintEqualToAnchor:self.view.trailingAnchor] setActive:YES];
}

- (void)viewDidAppear:(BOOL)animated
{
	assert(g_virtualMachine == nullptr);
	g_virtualMachine = new CPS2VM();
	g_virtualMachine->Initialize();

	auto gsHandlerId = CAppConfig::GetInstance().GetPreferenceInteger(PREFERENCE_VIDEO_GS_HANDLER);
	switch(gsHandlerId)
	{
	default:
		[[fallthrough]];
	case PREFERENCE_VALUE_VIDEO_GS_HANDLER_OPENGL:
		g_virtualMachine->CreateGSHandler(CGSH_OpenGLiOS::GetFactoryFunction((CAEAGLLayer*)self.view.layer));
		break;
#ifdef HAS_GSH_VULKAN
	case PREFERENCE_VALUE_VIDEO_GS_HANDLER_VULKAN:
		g_virtualMachine->CreateGSHandler(CGSH_VulkaniOS::GetFactoryFunction((CAMetalLayer*)self.view.layer));
		break;
#endif
	}

	self.connectObserver = [[NSNotificationCenter defaultCenter] addObserverForName:GCControllerDidConnectNotification
	                                                                         object:nil
	                                                                          queue:[NSOperationQueue mainQueue]
	                                                                     usingBlock:^(NSNotification* note) {
		                                                                   if([[GCController controllers] count] == 1)
		                                                                   {
			                                                                   [self toggleHardwareController:YES];
		                                                                   }
	                                                                     }];
	self.disconnectObserver = [[NSNotificationCenter defaultCenter] addObserverForName:GCControllerDidDisconnectNotification
	                                                                            object:nil
	                                                                             queue:[NSOperationQueue mainQueue]
	                                                                        usingBlock:^(NSNotification* note) {
		                                                                      if(![[GCController controllers] count])
		                                                                      {
			                                                                      [self toggleHardwareController:NO];
		                                                                      }
	                                                                        }];

	if([[GCController controllers] count])
	{
		[self toggleHardwareController:YES];
	}

	g_virtualMachine->CreatePadHandler(CPH_Generic::GetFactoryFunction());

	[self setupSoundHandler];
	[self updateOnScreenWidgets];

	g_virtualMachine->Pause();
	g_virtualMachine->Reset();

	auto bootablePath = fs::path([self.bootablePath fileSystemRepresentation]);
	if(IsBootableExecutablePath(bootablePath))
	{
		g_virtualMachine->m_ee->m_os->BootFromFile(bootablePath);
	}
	else
	{
		CAppConfig::GetInstance().SetPreferencePath(PREF_PS2_CDROM0_PATH, bootablePath);
		g_virtualMachine->Reset();
		g_virtualMachine->m_ee->m_os->BootFromCDROM();
	}

	g_virtualMachine->Resume();
}

- (void)viewDidDisappear:(BOOL)animated
{
	[[NSNotificationCenter defaultCenter] removeObserver:self.connectObserver];
	[[NSNotificationCenter defaultCenter] removeObserver:self.disconnectObserver];
	[self toggleHardwareController:NO];

	[self resetStatsTimer];
	g_virtualMachine->Pause();
	g_virtualMachine->Destroy();
	delete g_virtualMachine;
	g_virtualMachine = nullptr;
	g_gsNewFrameConnection.reset();
#ifdef PROFILE
	g_newFrameConnection.reset();
#endif
}

- (void)appResignedActive
{
	if(g_virtualMachine)
	{
		g_virtualMachine->Pause();
	}
}

- (void)appBecameActive
{
	if(g_virtualMachine)
	{
		g_virtualMachine->Resume();
	}
}

- (void)toggleHardwareController:(BOOL)useHardware
{
	if(useHardware)
	{
		self.gController = [GCController controllers][0];
		if(self.gController.extendedGamepad)
		{
			self.gController.extendedGamepad.valueChangedHandler =
			    ^(GCExtendedGamepad* gamepad, GCControllerElement* element) {
				  auto padHandler = static_cast<CPH_Generic*>(g_virtualMachine->GetPadHandler());
				  if(!padHandler) return;
				  if(element == gamepad.buttonA)
					  padHandler->SetButtonState(PS2::CControllerInfo::CROSS, gamepad.buttonA.pressed);
				  else if(element == gamepad.buttonB)
					  padHandler->SetButtonState(PS2::CControllerInfo::CIRCLE, gamepad.buttonB.pressed);
				  else if(element == gamepad.buttonX)
					  padHandler->SetButtonState(PS2::CControllerInfo::SQUARE, gamepad.buttonX.pressed);
				  else if(element == gamepad.buttonY)
					  padHandler->SetButtonState(PS2::CControllerInfo::TRIANGLE, gamepad.buttonY.pressed);
				  else if(element == gamepad.leftShoulder)
					  padHandler->SetButtonState(PS2::CControllerInfo::L1, gamepad.leftShoulder.pressed);
				  else if(element == gamepad.rightShoulder)
					  padHandler->SetButtonState(PS2::CControllerInfo::R1, gamepad.rightShoulder.pressed);
				  else if(element == gamepad.leftTrigger)
					  padHandler->SetButtonState(PS2::CControllerInfo::L2, gamepad.leftTrigger.pressed);
				  else if(element == gamepad.rightTrigger)
					  padHandler->SetButtonState(PS2::CControllerInfo::R2, gamepad.rightTrigger.pressed);
				  else if(element == gamepad.dpad)
				  {
					  padHandler->SetButtonState(PS2::CControllerInfo::DPAD_UP, gamepad.dpad.up.pressed);
					  padHandler->SetButtonState(PS2::CControllerInfo::DPAD_DOWN, gamepad.dpad.down.pressed);
					  padHandler->SetButtonState(PS2::CControllerInfo::DPAD_LEFT, gamepad.dpad.left.pressed);
					  padHandler->SetButtonState(PS2::CControllerInfo::DPAD_RIGHT, gamepad.dpad.right.pressed);
				  }
				  else if(element == gamepad.leftThumbstick)
				  {
					  padHandler->SetAxisState(PS2::CControllerInfo::ANALOG_LEFT_X, gamepad.leftThumbstick.xAxis.value);
					  padHandler->SetAxisState(PS2::CControllerInfo::ANALOG_LEFT_Y, -gamepad.leftThumbstick.yAxis.value);
				  }
				  else if(element == gamepad.rightThumbstick)
				  {
					  padHandler->SetAxisState(PS2::CControllerInfo::ANALOG_RIGHT_X, gamepad.rightThumbstick.xAxis.value);
					  padHandler->SetAxisState(PS2::CControllerInfo::ANALOG_RIGHT_Y, -gamepad.rightThumbstick.yAxis.value);
				  }
#if __IPHONE_OS_VERSION_MAX_ALLOWED >= 120100
				  if(@available(iOS 12.1, *))
				  {
					  if(element == gamepad.leftThumbstickButton)
						  padHandler->SetButtonState(PS2::CControllerInfo::L3, gamepad.leftThumbstickButton.pressed);
					  else if(element == gamepad.rightThumbstickButton)
						  padHandler->SetButtonState(PS2::CControllerInfo::R3, gamepad.rightThumbstickButton.pressed);
				  }
#endif
#if __IPHONE_OS_VERSION_MAX_ALLOWED >= 130000
				  if(@available(iOS 13.0, *))
				  {
					  if(element == gamepad.buttonOptions)
						  padHandler->SetButtonState(PS2::CControllerInfo::SELECT, gamepad.buttonOptions.pressed);
					  else if(element == gamepad.buttonMenu)
						  padHandler->SetButtonState(PS2::CControllerInfo::START, gamepad.buttonMenu.pressed);
				  }
#endif
			    };
		}
	}
	else
	{
		if(self.gController)
		{
			if(self.gController.extendedGamepad)
			{
				self.gController.extendedGamepad.valueChangedHandler = nil;
			}
		}
		self.gController = nil;
	}
	[self updateOnScreenWidgets];
}

- (void)resetStatsTimer
{
	if(self.fpsCounterTimer)
	{
		[self.fpsCounterTimer invalidate];
		self.fpsCounterTimer = nil;
	}
}

- (void)updateOnScreenWidgets
{
	auto screenBounds = [[UIScreen mainScreen] bounds];
	if(@available(iOS 11, *))
	{
		UIEdgeInsets insets = self.view.safeAreaInsets;
		screenBounds = UIEdgeInsetsInsetRect(screenBounds, insets);
	}

	//Remove previous widgets
	if(self.virtualPadView)
	{
		[self.virtualPadView removeFromSuperview];
		self.virtualPadView = nil;
	}

	if(self.fpsCounterLabel)
	{
		[self.fpsCounterLabel removeFromSuperview];
		self.fpsCounterLabel = nil;
	}

#ifdef PROFILE
	if(self.profilerStatsLabel)
	{
		[self.profilerStatsLabel removeFromSuperview];
		self.profilerStatsLabel = nil;
	}
#endif

	[self resetStatsTimer];

	if(CAppConfig::GetInstance().GetPreferenceBoolean(PREFERENCE_UI_SHOWVIRTUALPAD) &&
	   !(self.gController && CAppConfig::GetInstance().GetPreferenceBoolean(PREFERENCE_UI_HIDEVIRTUALPAD_CONTROLLER_CONNECTED)))
	{
		auto padHandler = static_cast<CPH_Generic*>(g_virtualMachine->GetPadHandler());
		self.virtualPadView = [[VirtualPadView alloc] initWithFrame:screenBounds padHandler:padHandler];
		[self.view addSubview:self.virtualPadView];
		[self.view sendSubviewToBack:self.virtualPadView];
		int prefTransparency = CAppConfig::GetInstance().GetPreferenceInteger(PREFERENCE_UI_VIRTUALPADOPACITY);
		self.virtualPadView.alpha = CGFloat(prefTransparency / 100.0);
	}

	if(CAppConfig::GetInstance().GetPreferenceBoolean(PREFERENCE_UI_SHOWFPS))
	{
		[self setupFpsCounterWithBounds:screenBounds];
	}
}

- (void)setupSoundHandler
{
	if(CAppConfig::GetInstance().GetPreferenceBoolean(PREFERENCE_AUDIO_ENABLEOUTPUT))
	{
		g_virtualMachine->CreateSoundHandler(&CSH_OpenAL::HandlerFactory);
	}
	else
	{
		g_virtualMachine->DestroySoundHandler();
	}
}

- (void)setupFpsCounterWithBounds:(CGRect)screenBounds
{
	self.fpsCounterLabel = [[UILabel alloc] initWithFrame:screenBounds];
	self.fpsCounterLabel.textColor = [UIColor whiteColor];
	[self.view addSubview:self.fpsCounterLabel];

#ifdef PROFILE
	self.profilerStatsLabel = [[UILabel alloc] initWithFrame:screenBounds];
	self.profilerStatsLabel.textColor = [UIColor whiteColor];
	self.profilerStatsLabel.font = [UIFont fontWithName:@"Courier" size:10.f];
	self.profilerStatsLabel.numberOfLines = 0;
	[self.view addSubview:self.profilerStatsLabel];
#endif

	g_gsNewFrameConnection = g_virtualMachine->GetGSHandler()->OnNewFrame.Connect(std::bind(&CStatsManager::OnGsNewFrame, &CStatsManager::GetInstance(), std::placeholders::_1));
#ifdef PROFILE
	g_newFrameConnection = g_virtualMachine->OnNewFrame.Connect(std::bind(&CStatsManager::OnNewFrame, &CStatsManager::GetInstance(), g_virtualMachine));
#endif
	self.fpsCounterTimer = [NSTimer scheduledTimerWithTimeInterval:1 target:self selector:@selector(updateFpsCounter) userInfo:nil repeats:YES];
}

- (void)updateFpsCounter
{
	uint32 drawCallCount = CStatsManager::GetInstance().GetDrawCalls();
	uint32 frames = CStatsManager::GetInstance().GetFrames();
	uint32 dcpf = (frames != 0) ? (drawCallCount / frames) : 0;
	self.fpsCounterLabel.text = [NSString stringWithFormat:@"%d f/s, %d dc/f", frames, dcpf];
	[self.fpsCounterLabel sizeToFit];
#ifdef PROFILE
	self.profilerStatsLabel.text = [NSString stringWithUTF8String:CStatsManager::GetInstance().GetProfilingInfo().c_str()];
#endif
	CStatsManager::GetInstance().ClearStats();
}

- (void)onLoadStateButtonClick
{
	self.saveStateViewController.action = SaveStateActionLoad;
	self.saveStateViewController.view.hidden = false;
}

- (void)onSaveStateButtonClick
{
	self.saveStateViewController.action = SaveStateActionSave;
	self.saveStateViewController.view.hidden = false;
}

- (void)onExitButtonClick
{
	[self dismissViewControllerAnimated:YES completion:nil];
}

- (IBAction)onPauseButtonClick:(id)sender
{
	UIAlertControllerStyle style = UIAlertControllerStyleAlert;
	if([UIDevice currentDevice].userInterfaceIdiom == UIUserInterfaceIdiomPhone)
	{
		style = UIAlertControllerStyleActionSheet;
	}

	UIAlertController* alert = [UIAlertController alertControllerWithTitle:nil message:nil preferredStyle:style];

	//Load State
	{
		UIAlertAction* action = [UIAlertAction
		    actionWithTitle:@"Load State"
		              style:UIAlertActionStyleDefault
		            handler:^(UIAlertAction*) {
			          [self onLoadStateButtonClick];
		            }];
		[alert addAction:action];
	}

	//Save State
	{
		UIAlertAction* action = [UIAlertAction
		    actionWithTitle:@"Save State"
		              style:UIAlertActionStyleDefault
		            handler:^(UIAlertAction*) {
			          [self onSaveStateButtonClick];
		            }];
		[alert addAction:action];
	}

	//Settings
	{
		UIAlertAction* action = [UIAlertAction
		    actionWithTitle:@"Settings"
		              style:UIAlertActionStyleDefault
		            handler:^(UIAlertAction*) {
			          [self performSegueWithIdentifier:@"showSettings" sender:self];
		            }];
		[alert addAction:action];
	}

	//Resume
	{
		UIAlertAction* action = [UIAlertAction
		    actionWithTitle:@"Resume"
		              style:UIAlertActionStyleCancel
		            handler:nil];
		[alert addAction:action];
	}

	//Exit
	{
		UIAlertAction* action = [UIAlertAction
		    actionWithTitle:@"Exit"
		              style:UIAlertActionStyleDestructive
		            handler:^(UIAlertAction*) {
			          [self onExitButtonClick];
		            }];
		[alert addAction:action];
	}

	[self presentViewController:alert animated:YES completion:nil];
}

- (BOOL)prefersStatusBarHidden
{
	return YES;
}

- (UIInterfaceOrientationMask)supportedInterfaceOrientations
{
	return UIInterfaceOrientationMaskLandscape;
}

- (void)prepareForSegue:(UIStoryboardSegue*)segue sender:(id)sender
{
	if([segue.identifier isEqualToString:@"showSettings"])
	{
		UINavigationController* navViewController = segue.destinationViewController;
		SettingsViewController* settingsViewController = (SettingsViewController*)navViewController.visibleViewController;
		settingsViewController.completionHandler = ^(bool) {
		  [self setupSoundHandler];
		  [self updateOnScreenWidgets];
		  auto gsHandler = g_virtualMachine->GetGSHandler();
		  if(gsHandler)
		  {
			  gsHandler->NotifyPreferencesChanged();
		  }
		};
	}
}

#pragma mark -
#pragma mark SaveStateDelegate
- (void)saveStateUsingPosition:(uint32_t)position
{
	auto statePath = g_virtualMachine->GenerateStatePath(position);
	g_virtualMachine->SaveState(statePath);
	NSLog(@"Saved state to '%s'.", statePath.string().c_str());
}

- (void)loadStateUsingPosition:(uint32_t)position
{
	auto statePath = g_virtualMachine->GenerateStatePath(position);
	g_virtualMachine->LoadState(statePath);
	NSLog(@"Loaded state from '%s'.", statePath.string().c_str());
}

@end
