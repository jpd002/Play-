#import <SDWebImage/UIImageView+WebCache.h>
#import "CoverViewController.h"
#import "EmulatorViewController.h"
#import "../ui_shared/BootablesProcesses.h"
#import "../ui_shared/BootablesDbClient.h"
#import "PathUtils.h"
#import "BackgroundLayer.h"
#import "CoverViewCell.h"

@interface CoverViewController ()

@end

@implementation CoverViewController

static NSString* const reuseIdentifier = @"coverCell";

- (void)buildCollection
{
	assert(_bootables == nullptr);

	UIAlertController* alert = [UIAlertController alertControllerWithTitle: @"Building collection" message: @"Please wait..." preferredStyle: UIAlertControllerStyleAlert];
	
	CGRect aivRect = CGRectMake(0, 0, 40, 40);
	
	UIActivityIndicatorView* aiv = [[UIActivityIndicatorView alloc] initWithFrame: aivRect];
	[aiv startAnimating];
	
	UIViewController* vc = [[UIViewController alloc] init];
	vc.preferredContentSize = aivRect.size;
	[vc.view addSubview: aiv];
	[alert setValue: vc forKey: @"contentViewController"];
	
	[self presentViewController:alert animated:YES completion:nil];
	
	dispatch_queue_t queue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);
	dispatch_async(queue, ^{
		
		auto activeDirs = GetActiveBootableDirectories();
		if(activeDirs.empty())
		{
			dispatch_async(dispatch_get_main_queue(), ^{
				alert.message = @"Scanning games on filesystem...";
			});
			ScanBootables("/private/var/mobile");
		}
		else
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
		//thus, games from the previous installation won't be found (will be deleted by PurgeInexistingFiles).
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

		_bootables = new BootableArray(BootablesDb::CClient::GetInstance().GetBootables());

		//Done
		dispatch_async(dispatch_get_main_queue(), ^{
			[alert dismissViewControllerAnimated: YES completion: nil];
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
	
	[self buildCollection];
}

- (void)viewDidUnload
{
	assert(_bootables != nullptr);
	delete _bootables;

	[super viewDidUnload];
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
}

- (IBAction)onExit:(id)sender
{
	exit(0);
}

@end
