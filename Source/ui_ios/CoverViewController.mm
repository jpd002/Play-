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

-(void)awakeFromNib
{
	[super awakeFromNib];
	
	ScanBootables(Framework::PathUtils::GetPersonalDataPath());
	PurgeInexistingFiles();
	FetchGameTitles();
}

-(void)viewDidLoad
{
	[super viewDidLoad];
	
	assert(_bootables == nullptr);
	_bootables = new BootableArray(BootablesDb::CClient::GetInstance().GetBootables());

	CAGradientLayer* bgLayer = [BackgroundLayer blueGradient];
	bgLayer.frame = self.view.bounds;
	[self.view.layer insertSublayer: bgLayer atIndex: 0];

	self.collectionView.allowsMultipleSelection = NO;
}

-(void)viewDidUnload
{
	assert(_bootables != nullptr);
	delete _bootables;
	
	[super viewDidUnload];
}

-(void)willAnimateRotationToInterfaceOrientation: (UIInterfaceOrientation)toInterfaceOrientation duration: (NSTimeInterval)duration
{
	// resize your layers based on the view’s new bounds
	[[[self.view.layer sublayers] objectAtIndex: 0] setFrame: self.view.bounds];
}

-(BOOL)shouldAutorotate
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

-(NSInteger)numberOfSectionsInCollectionView: (UICollectionView*)collectionView
{
	return 1;
}

-(NSString*)collectionView: (UICollectionView*)collectionView titleForHeaderInSection: (NSInteger)section
{
	return @"";
}

-(NSInteger)collectionView: (UICollectionView*)collectionView numberOfItemsInSection: (NSInteger)section
{
	return _bootables->size();
}

-(UICollectionViewCell*)collectionView: (UICollectionView*)collectionView cellForItemAtIndexPath: (NSIndexPath*)indexPath
{
	CoverViewCell* cell = (CoverViewCell*)[collectionView dequeueReusableCellWithReuseIdentifier: reuseIdentifier forIndexPath: indexPath];
	
	auto bootable = (*_bootables)[indexPath.row];
	UIImage* placeholder = [UIImage imageNamed: @"boxart.png"];
	cell.nameLabel.text = [NSString stringWithUTF8String: bootable.title.c_str()];
	cell.backgroundView = [[UIImageView alloc] initWithImage: placeholder];
	
	if(!bootable.coverUrl.empty())
	{
		NSString* coverUrl = [NSString stringWithUTF8String: bootable.coverUrl.c_str()];
		[(UIImageView*)cell.backgroundView sd_setImageWithURL: [NSURL URLWithString: coverUrl] placeholderImage: placeholder];
	}
	
	return cell;
}

#pragma mark <UICollectionViewDelegate>

-(void)prepareForSegue: (UIStoryboardSegue*)segue sender: (id)sender
{
	if([segue.identifier isEqualToString: @"showEmulator"])
	{
		NSIndexPath* indexPath = [[self.collectionView indexPathsForSelectedItems] objectAtIndex: 0];
		auto bootable = (*_bootables)[indexPath.row];
		BootablesDb::CClient::GetInstance().SetLastBootedTime(bootable.path, time(nullptr));
		EmulatorViewController* emulatorViewController = segue.destinationViewController;
		emulatorViewController.bootablePath = [NSString stringWithUTF8String: bootable.path.native().c_str()];
		[self.collectionView deselectItemAtIndexPath: indexPath animated: NO];
	}
}

-(IBAction)onExit: (id)sender
{
	exit(0);
}

@end
