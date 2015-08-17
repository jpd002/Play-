#import "MainViewController.h"
#import "EmulatorViewController.h"
#import "IosUtils.h"
#import "DiskUtils.h"
#import "CoverViewCell.h"
#import "BackgroundLayer.h"

@interface MainViewController ()

@end

@implementation MainViewController

- (void)awakeFromNib
{
    self.mainView = [ViewOrientation getInstance];
    self.mainView.isShowingLandscapeView = YES;
    [[UIDevice currentDevice] beginGeneratingDeviceOrientationNotifications];
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(orientationChanged:)
                                                 name:UIDeviceOrientationDidChangeNotification
                                               object:nil];
}

-(void)viewDidLoad
{
	[super viewDidLoad];

	CAGradientLayer *bgLayer = [BackgroundLayer blueGradient];
	bgLayer.frame = self.view.bounds;
	[self.view.layer insertSublayer:bgLayer atIndex:0];
	
    self.database = [[SqliteDatabase alloc] init];
    if (self.mainView.diskImages == nil)
    {
         self.mainView.diskImages = [self.mainView buildCollection];
    }

    [self.tableView reloadData];
}

- (void)willAnimateRotationToInterfaceOrientation:(UIInterfaceOrientation)toInterfaceOrientation duration:(NSTimeInterval)duration
{
	// resize your layers based on the view’s new bounds
	[[[self.view.layer sublayers] objectAtIndex:0] setFrame:self.view.bounds];
}

- (void)orientationChanged:(NSNotification *)notification
{
	UIDeviceOrientation deviceOrientation = [UIDevice currentDevice].orientation;
	if (UIDeviceOrientationIsPortrait(deviceOrientation) &&
		self.mainView.isShowingLandscapeView)
	{
		[self performSegueWithIdentifier:@"DisplayCoverView" sender:self];
		self.mainView.isShowingLandscapeView = YES;
	}
	else if (UIDeviceOrientationIsLandscape(deviceOrientation) &&
			 !self.mainView.isShowingLandscapeView)
	{
		[self dismissViewControllerAnimated:YES completion:nil];
		self.mainView.isShowingLandscapeView = NO;
	}
	else if (UIDeviceOrientationIsLandscape(deviceOrientation) &&
			 self.mainView.isShowingLandscapeView)
	{
		[self.presentingViewController dismissViewControllerAnimated:YES completion:nil];
	}
}

- (BOOL)shouldAutorotate {
	if ([self isViewLoaded] && self.view.window) {
		return YES;
	} else {
		return NO;
	}
}

-(void)prepareForSegue: (UIStoryboardSegue*)segue sender: (id)sender
{
    if ([segue.identifier isEqualToString:@"showEmulator"]) {
        NSIndexPath* indexPath = self.tableView.indexPathForSelectedRow;
        NSString* filePath = [[self.mainView.diskImages objectAtIndex: indexPath.row] objectForKey:@"file"];
        NSString* homeDirPath = [@"~" stringByExpandingTildeInPath];
        NSString* absolutePath = [homeDirPath stringByAppendingPathComponent: filePath];
        EmulatorViewController* emulatorViewController = segue.destinationViewController;
        emulatorViewController.imagePath = absolutePath;
    }
}

-(NSInteger)numberOfSectionsInTableView: (UITableView*)tableView
{
	return 1;
}

-(NSInteger)tableView: (UITableView *)tableView numberOfRowsInSection: (NSInteger)section 
{
	return [self.mainView.diskImages count];
}

-(NSString*)tableView: (UITableView*)tableView titleForHeaderInSection: (NSInteger)section 
{
	return @"";
}

- (CGFloat)tableView:(UITableView *)tableView heightForRowAtIndexPath:(NSIndexPath *)indexPath {
    return 160;
    // Assign the specific cell height to prevent issues with custom size
}

-(UITableViewCell*)tableView: (UITableView*)tableView cellForRowAtIndexPath: (NSIndexPath*)indexPath 
{
    static NSString *CellIdentifier = @"detailCell";
    
    CoverViewCell *cell = [tableView dequeueReusableCellWithIdentifier:CellIdentifier forIndexPath:indexPath];
    NSDictionary *disk = [self.mainView.diskImages objectAtIndex: indexPath.row];
    NSString* diskId = [disk objectForKey:@"serial"];
    
    NSDictionary *game = [self.database getDiskInfo:diskId];
    if ([[game objectForKey:@"title"] isEqual:@""]) {
        cell.nameLabel.text = [[disk objectForKey:@"file"] lastPathComponent];
    } else {
        cell.nameLabel.text = [game objectForKey:@"title"];
    }
    if (![[game objectForKey:@"boxart"] isEqual:@"404"]) {
        NSString *imageIcon = [[NSString alloc] initWithFormat:@"http://thegamesdb.net/banners/%@", [game objectForKey:@"boxart"]];
        [cell.coverImage setImageWithURL:[NSURL URLWithString:imageIcon] placeholderImage:[UIImage imageNamed:@"boxart.png"]];
    }
    cell.overviewLabel.text = [game objectForKey:@"overview"];
    
	return cell;
}

-(void)tableView: (UITableView*)tableView didSelectRowAtIndexPath: (NSIndexPath*)indexPath
{
	[self performSegueWithIdentifier: @"showEmulator" sender: self];
}

@end
