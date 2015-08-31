//
//  CoverViewController.m
//  Play
//
//  Created by Lounge Katt on 8/15/15.
//  Copyright (c) 2015 Jean-Philip Desjardins. All rights reserved.
//

#import "CoverViewController.h"
#import "EmulatorViewController.h"
#import "IosUtils.h"
#import "DiskUtils.h"
#import "BackgroundLayer.h"
#import "CoverViewCell.h"

@interface CoverViewController ()

@end

@implementation CoverViewController

static NSString * const reuseIdentifier = @"coverCell";

- (void)awakeFromNib
{
    self.coverView = [CollectionView getInstance];
}

- (void)viewDidLoad {
    [super viewDidLoad];
	
	CAGradientLayer *bgLayer = [BackgroundLayer blueGradient];
	bgLayer.frame = self.view.bounds;
	[self.view.layer insertSublayer:bgLayer atIndex:0];
    
    // Uncomment the following line to preserve selection between presentations
    // self.clearsSelectionOnViewWillAppear = NO;

    self.collectionView.allowsMultipleSelection = NO;
	
    self.database = [[SqliteDatabase alloc] init];
    if (self.coverView.diskImages == nil)
    {
        self.coverView.diskImages = [self.coverView buildCollection];
    }
    
    [self.collectionView reloadData];
}

- (void)willAnimateRotationToInterfaceOrientation:(UIInterfaceOrientation)toInterfaceOrientation duration:(NSTimeInterval)duration
{
	// resize your layers based on the view’s new bounds
	[[[self.view.layer sublayers] objectAtIndex:0] setFrame:self.view.bounds];
}

- (BOOL)shouldAutorotate {
	if ([self isViewLoaded] && self.view.window) {
		return YES;
	} else {
		return NO;
	}
}

- (void)didReceiveMemoryWarning {
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

/*
#pragma mark - Navigation

// In a storyboard-based application, you will often want to do a little preparation before navigation
- (void)prepareForSegue:(UIStoryboardSegue *)segue sender:(id)sender {
    // Get the new view controller using [segue destinationViewController].
    // Pass the selected object to the new view controller.
}
*/

#pragma mark <UICollectionViewDataSource>

- (NSInteger)numberOfSectionsInCollectionView:(UICollectionView *)collectionView {
    return 1;
}

- (NSString*)collectionView:(UICollectionView *)collectionView titleForHeaderInSection: (NSInteger)section
{
    return @"";
}

- (NSInteger)collectionView:(UICollectionView *)collectionView numberOfItemsInSection:(NSInteger)section {
    return self.coverView.diskImages.count;
}

- (UICollectionViewCell *)collectionView:(UICollectionView *)collectionView cellForItemAtIndexPath:(NSIndexPath *)indexPath {
	CoverViewCell *cell = (CoverViewCell *)[collectionView dequeueReusableCellWithReuseIdentifier:reuseIdentifier forIndexPath:indexPath];
    
    NSDictionary *disk = [self.coverView.diskImages objectAtIndex: indexPath.row];
    NSString* diskId = [disk objectForKey:@"serial"];
    
    NSDictionary *game = [self.database getDiskInfo:diskId];

	cell.backgroundView = [[UIImageView alloc] initWithImage:[UIImage imageNamed:@"boxart.png"]];
    if ([game objectForKey:@"boxart"] != nil && ![[game objectForKey:@"boxart"] isEqual:@"404"]) {
        NSString *imageIcon = [[NSString alloc] initWithFormat:@"http://thegamesdb.net/banners/%@", [game objectForKey:@"boxart"]];
        [(UIImageView *)cell.backgroundView setImageWithURL:[NSURL URLWithString:imageIcon] placeholderImage:[UIImage imageNamed:@"boxart.png"]];
	} else {
		cell.nameLabel.text = [[[disk objectForKey:@"file"] lastPathComponent] stringByDeletingPathExtension];
	}
		
    // Configure the cell
    
    return cell;
}

#pragma mark <UICollectionViewDelegate>

/*
// Uncomment this method to specify if the specified item should be highlighted during tracking
- (BOOL)collectionView:(UICollectionView *)collectionView shouldHighlightItemAtIndexPath:(NSIndexPath *)indexPath {
	return YES;
}
*/

/*
// Uncomment this method to specify if the specified item should be selected
- (BOOL)collectionView:(UICollectionView *)collectionView shouldSelectItemAtIndexPath:(NSIndexPath *)indexPath {
    return YES;
}
*/

-(void)prepareForSegue: (UIStoryboardSegue*)segue sender: (id)sender
{
    if ([segue.identifier isEqualToString:@"showEmulator"]) {
        NSIndexPath* indexPath = [[self.collectionView indexPathsForSelectedItems] objectAtIndex:0];
        NSString* filePath = [[self.coverView.diskImages objectAtIndex: indexPath.row] objectForKey:@"file"];
        NSString* homeDirPath = [@"~" stringByExpandingTildeInPath];
        NSString* absolutePath = [homeDirPath stringByAppendingPathComponent: filePath];
        EmulatorViewController* emulatorViewController = segue.destinationViewController;
        emulatorViewController.imagePath = absolutePath;
        [self.collectionView deselectItemAtIndexPath:indexPath animated:NO];
    }
}

-(void)collectionView: (UICollectionView*)collectionView didSelectItemAtIndexPath: (NSIndexPath*)indexPath
{
    [self performSegueWithIdentifier: @"showEmulator" sender: self];
}

/*
// Uncomment these methods to specify if an action menu should be displayed for the specified item, and react to actions performed on the item
- (BOOL)collectionView:(UICollectionView *)collectionView shouldShowMenuForItemAtIndexPath:(NSIndexPath *)indexPath {
	return NO;
}

- (BOOL)collectionView:(UICollectionView *)collectionView canPerformAction:(SEL)action forItemAtIndexPath:(NSIndexPath *)indexPath withSender:(id)sender {
	return NO;
}

- (void)collectionView:(UICollectionView *)collectionView performAction:(SEL)action forItemAtIndexPath:(NSIndexPath *)indexPath withSender:(id)sender {
	
}
*/

@end
