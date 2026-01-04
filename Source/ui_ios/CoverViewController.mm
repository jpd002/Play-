#import <SDWebImage/UIImageView+WebCache.h>
#import "CoverViewController.h"
#import "EmulatorViewController.h"
#import "SettingsViewController.h"
#import "../ui_shared/BootablesProcesses.h"
#import "../ui_shared/BootablesDbClient.h"
#import "PathUtils.h"
#import "BackgroundLayer.h"
#import "CoverViewCell.h"
#import "AltServerJitService.h"
#import "StikDebugJitService.h"

static bool IsJitAvailable()
{
	//If ppid != 1, it means we're being run in the debugger
	if(getppid() != 1) return true;
	if([[AltServerJitService sharedAltServerJitService] jitEnabled])
	{
		return true;
	}
	{
		//Check if we can scan the mobile directory (only possible if jailbroken)
		std::error_code errorCode;
		fs::directory_iterator dirIterator("/private/var/mobile", errorCode);
		if(!errorCode)
		{
			return true;
		}
	}
	return false;
}

@interface CoverViewController ()

@end

@implementation CoverViewController

static NSString* const reuseIdentifier = @"coverCell";

- (void)buildCollectionWithForcedFullScan:(BOOL)forceFullDeviceScan
{
	UIAlertController* alert = [UIAlertController alertControllerWithTitle:@"Building collection" message:@"Please wait..." preferredStyle:UIAlertControllerStyleAlert];

	CGRect aivRect = CGRectMake(0, 0, 40, 40);

	UIActivityIndicatorView* aiv = [[UIActivityIndicatorView alloc] initWithFrame:aivRect];
	[aiv startAnimating];

	UIViewController* vc = [[UIViewController alloc] init];
	vc.preferredContentSize = aivRect.size;
	[vc.view addSubview:aiv];
	[alert setValue:vc forKey:@"contentViewController"];

	[self presentViewController:alert animated:YES completion:nil];

	dispatch_queue_t queue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);
	dispatch_async(queue, ^{
	  auto activeDirs = GetActiveBootableDirectories();
	  if(forceFullDeviceScan)
	  {
		  dispatch_async(dispatch_get_main_queue(), ^{
			alert.message = @"Scanning games on filesystem...";
		  });
		  ScanBootables("/private/var/mobile");
	  }
	  else if(!activeDirs.empty())
	  {
		  dispatch_async(dispatch_get_main_queue(), ^{
			alert.message = @"Scanning games in active directories...";
		  });
		  for(const auto& activeDir : activeDirs)
		  {
			  ScanBootables(activeDir, false);
		  }
	  }

	  //Always scan games in app storage. The app's path change when it's reinstalled,
	  //thus, games from the previous installation won't be found (will be deleted in PurgeInexistingFiles).
	  dispatch_async(dispatch_get_main_queue(), ^{
		alert.message = @"Scanning games in app storage...";
	  });
	  ScanBootables(Framework::PathUtils::GetPersonalDataPath());

	  dispatch_async(dispatch_get_main_queue(), ^{
		alert.message = @"Purging inexisting files...";
	  });
	  PurgeInexistingFiles();

	  dispatch_async(dispatch_get_main_queue(), ^{
		alert.message = @"Fetching game titles...";
	  });
	  FetchGameTitles();

	  if(_bootables)
	  {
		  delete _bootables;
		  _bootables = nullptr;
	  }
	  _bootables = new BootableArray(BootablesDb::CClient::GetInstance().GetBootables());

	  //Done
	  dispatch_async(dispatch_get_main_queue(), ^{
		[alert dismissViewControllerAnimated:YES completion:nil];
		[self.collectionView reloadData];
	  });
	});
}

- (void)viewDidLoad
{
	[super viewDidLoad];

	CAGradientLayer* bgLayer = [BackgroundLayer blueGradient];
	bgLayer.frame = self.view.bounds;
	[self.view.layer insertSublayer:bgLayer atIndex:0];

	self.collectionView.allowsMultipleSelection = NO;
	if(@available(iOS 11.0, *))
	{
		self.collectionView.contentInsetAdjustmentBehavior = UIScrollViewContentInsetAdjustmentAlways;
	}

	[[AltServerJitService sharedAltServerJitService] startProcess];
	[self buildCollectionWithForcedFullScan:NO];
}
- (void)viewDidAppear:(BOOL)animated
{
	[super viewDidAppear:animated];

	// ========== AJOUT JIT iOS 26 ==========
	StikDebugJitService* jitService = [StikDebugJitService sharedService];

	if([jitService needsActivation])
	{
		[self showJITActivationAlert];
	}
	// ========== FIN AJOUT JIT iOS 26 ==========

	// ... reste du code existant ...
}
- (void)viewDidUnload
{
	assert(_bootables != nullptr);
	delete _bootables;

	[super viewDidUnload];
}
- (void)showJITActivationAlert
{
	StikDebugJitService* jitService = [StikDebugJitService sharedService];

	NSString* message;
	if([jitService isStikDebugInstalled])
	{
		message = @"iOS 26 requires JIT activation for PS2 emulation.\n\n"
		          @"Tap 'Activate JIT' to open StikDebug and enable JIT.\n"
		          @"Then return to Play! to start gaming.";
	}
	else
	{
		message = @"iOS 26 requires JIT activation for PS2 emulation.\n\n"
		          @"Please install StikDebug from your preferred source "
		          @"(AltStore, SideStore, etc.) to enable JIT.";
	}

	UIAlertController* alert = [UIAlertController alertControllerWithTitle:@"JIT Required"
	                                                               message:message
	                                                        preferredStyle:UIAlertControllerStyleAlert];

	if([jitService isStikDebugInstalled])
	{
		UIAlertAction* activateAction = [UIAlertAction actionWithTitle:@"Activate JIT"
		                                                         style:UIAlertActionStyleDefault
		                                                       handler:^(UIAlertAction* action) {
			                                                     [jitService requestActivationWithCompletion:^(BOOL success, NSError* error) {
				                                                   dispatch_async(dispatch_get_main_queue(), ^{
					                                                 if(success)
					                                                 {
						                                                 // JIT activated - user can now play games
						                                                 UIAlertController* successAlert = [UIAlertController
						                                                     alertControllerWithTitle:@"JIT Active"
						                                                                      message:@"JIT has been activated. You can now play PS2 games!"
						                                                               preferredStyle:UIAlertControllerStyleAlert];
						                                                 [successAlert addAction:[UIAlertAction actionWithTitle:@"OK"
						                                                                                                  style:UIAlertActionStyleDefault
						                                                                                                handler:nil]];
						                                                 [self presentViewController:successAlert animated:YES completion:nil];
					                                                 }
					                                                 else
					                                                 {
						                                                 // Show error
						                                                 UIAlertController* errorAlert = [UIAlertController
						                                                     alertControllerWithTitle:@"JIT Activation Failed"
						                                                                      message:error.localizedDescription
						                                                               preferredStyle:UIAlertControllerStyleAlert];
						                                                 [errorAlert addAction:[UIAlertAction actionWithTitle:@"Retry"
						                                                                                                style:UIAlertActionStyleDefault
						                                                                                              handler:^(UIAlertAction* action) {
							                                                                                            [self showJITActivationAlert];
						                                                                                              }]];
						                                                 [errorAlert addAction:[UIAlertAction actionWithTitle:@"Cancel"
						                                                                                                style:UIAlertActionStyleCancel
						                                                                                              handler:nil]];
						                                                 [self presentViewController:errorAlert animated:YES completion:nil];
					                                                 }
				                                                   });
			                                                     }];
		                                                       }];
		[alert addAction:activateAction];
	}

	UIAlertAction* cancelAction = [UIAlertAction actionWithTitle:@"Later"
	                                                       style:UIAlertActionStyleCancel
	                                                     handler:nil];
	[alert addAction:cancelAction];

	[self presentViewController:alert animated:YES completion:nil];
}
- (void)willAnimateRotationToInterfaceOrientation:(UIInterfaceOrientation)toInterfaceOrientation duration:(NSTimeInterval)duration
{
	// resize your layers based on the view’s new bounds
	[[[self.view.layer sublayers] objectAtIndex:0] setFrame:self.view.bounds];
}

- (BOOL)shouldAutorotate
{
	if([self isViewLoaded] && self.view.window)
	{
		return YES;
	}
	else
	{
		return NO;
	}
}

#pragma mark <UICollectionViewDataSource>

- (NSInteger)numberOfSectionsInCollectionView:(UICollectionView*)collectionView
{
	return 1;
}

- (NSString*)collectionView:(UICollectionView*)collectionView titleForHeaderInSection:(NSInteger)section
{
	return @"";
}

- (NSInteger)collectionView:(UICollectionView*)collectionView numberOfItemsInSection:(NSInteger)section
{
	return _bootables ? _bootables->size() : 0;
}

- (UICollectionViewCell*)collectionView:(UICollectionView*)collectionView cellForItemAtIndexPath:(NSIndexPath*)indexPath
{
	CoverViewCell* cell = (CoverViewCell*)[collectionView dequeueReusableCellWithReuseIdentifier:reuseIdentifier forIndexPath:indexPath];

	auto bootable = (*_bootables)[indexPath.row];
	UIImage* placeholder = [UIImage imageNamed:@"boxart.png"];
	cell.nameLabel.text = [NSString stringWithUTF8String:bootable.title.c_str()];
	cell.backgroundView = [[UIImageView alloc] initWithImage:placeholder];

	if(!bootable.coverUrl.empty())
	{
		NSString* coverUrl = [NSString stringWithUTF8String:bootable.coverUrl.c_str()];
		[(UIImageView*)cell.backgroundView sd_setImageWithURL:[NSURL URLWithString:coverUrl] placeholderImage:placeholder];
	}

	return cell;
}

#pragma mark <UICollectionViewDelegate>

- (BOOL)shouldPerformSegueWithIdentifier:(NSString*)identifier sender:(id)sender
{
	if([identifier isEqualToString:@"showEmulator"] && !IsJitAvailable())
	{
		UIAlertController* alert = [UIAlertController alertControllerWithTitle:@"JIT unavailable" message:@"JIT doesn't seem to be available at the moment. If JIT is not available, the emulator will crash. Do you wish to continue?" preferredStyle:UIAlertControllerStyleAlert];
		{
			UIAlertAction* continueAction = [UIAlertAction
			    actionWithTitle:@"Continue"
			              style:UIAlertActionStyleDefault
			            handler:^(UIAlertAction*) {
				          [self performSegueWithIdentifier:@"showEmulator" sender:sender];
			            }];
			[alert addAction:continueAction];
		}
		{
			UIAlertAction* cancelAction = [UIAlertAction
			    actionWithTitle:@"Cancel"
			              style:UIAlertActionStyleCancel
			            handler:^(UIAlertAction*){}];
			[alert addAction:cancelAction];
		}
		{
			UIAlertAction* helpAction = [UIAlertAction
			    actionWithTitle:@"Help"
			              style:UIAlertActionStyleDefault
			            handler:^(UIAlertAction*) {
				          [[UIApplication sharedApplication] openURL:[NSURL URLWithString:@"https://github.com/jpd002/Play-#running-on-ios"]];
			            }];
			[alert addAction:helpAction];
		}
		[self presentViewController:alert animated:YES completion:nil];
		return NO;
	}
	return YES;
}

- (void)prepareForSegue:(UIStoryboardSegue*)segue sender:(id)sender
{
	if([segue.identifier isEqualToString:@"showEmulator"])
	{
		NSIndexPath* indexPath = [[self.collectionView indexPathsForSelectedItems] objectAtIndex:0];
		auto bootable = (*_bootables)[indexPath.row];
		BootablesDb::CClient::GetInstance().SetLastBootedTime(bootable.path, time(nullptr));
		EmulatorViewController* emulatorViewController = segue.destinationViewController;
		emulatorViewController.bootablePath = [NSString stringWithUTF8String:bootable.path.native().c_str()];
		[self.collectionView deselectItemAtIndexPath:indexPath animated:NO];
	}
	else if([segue.identifier isEqualToString:@"showSettings"])
	{
		UINavigationController* navViewController = segue.destinationViewController;
		SettingsViewController* settingsViewController = (SettingsViewController*)navViewController.visibleViewController;
		settingsViewController.allowFullDeviceScan = true;
		settingsViewController.allowGsHandlerSelection = true;
		settingsViewController.completionHandler = ^(bool fullScanRequested) {
		  [[AltServerJitService sharedAltServerJitService] startProcess];
		  if(fullScanRequested)
		  {
			  [self buildCollectionWithForcedFullScan:YES];
		  }
		};
	}
}

- (IBAction)onExit:(id)sender
{
	exit(0);
}

@end
