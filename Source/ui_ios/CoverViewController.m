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

@interface CoverViewController ()

@end

@implementation CoverViewController

static NSString * const reuseIdentifier = @"coverCell";

- (void)awakeFromNib
{
    self.coverView = [ViewOrientation getInstance];
    self.coverView.isShowingLandscapeView = NO;

    [[UIDevice currentDevice] beginGeneratingDeviceOrientationNotifications];
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(orientationChanged:)
                                                 name:UIDeviceOrientationDidChangeNotification
                                               object:nil];
}

- (void)orientationChanged:(NSNotification *)notification
{
    UIDeviceOrientation deviceOrientation = [UIDevice currentDevice].orientation;
    if (UIDeviceOrientationIsLandscape(deviceOrientation) &&
        !self.coverView.isShowingLandscapeView)
    {
        [self performSegueWithIdentifier:@"DisplayMainView" sender:self];
        self.coverView.isShowingLandscapeView = YES;
    }
    else if (UIDeviceOrientationIsPortrait(deviceOrientation) &&
             self.coverView.isShowingLandscapeView)
    {
        [self dismissViewControllerAnimated:YES completion:nil];
        self.coverView.isShowingLandscapeView = NO;
    }
    else if (UIDeviceOrientationIsPortrait(deviceOrientation) &&
             !self.coverView.isShowingLandscapeView)
    {
        [self.presentingViewController dismissViewControllerAnimated:YES completion:nil];
    }
}

- (void)viewDidLoad {
    [super viewDidLoad];

    self.database = [[SqliteDatabase alloc] init];
    
    NSString* path = [@"~" stringByExpandingTildeInPath];
    NSFileManager* localFileManager = [[NSFileManager alloc] init];
    
    NSDirectoryEnumerator* dirEnum = [localFileManager enumeratorAtPath: path];
    
    NSMutableArray* images = [[NSMutableArray alloc] init];
    
    NSString* file = nil;
    while(file = [dirEnum nextObject])
    {
        std::string diskId;
        if(
           IosUtils::IsLoadableExecutableFileName(file) ||
           (IosUtils::IsLoadableDiskImageFileName(file) &&
            DiskUtils::TryGetDiskId([[NSString stringWithFormat:@"%@/%@", path, file] UTF8String], &diskId))
           )
        {
            NSMutableDictionary *disk = [[NSMutableDictionary alloc] init];
            [disk setValue:[NSString stringWithUTF8String:diskId.c_str()] forKey:@"serial"];
            [disk setValue:file forKey:@"file"];
            [images addObject: disk];
            NSLog(@"%@: %s", file, diskId.c_str());
        }
    }
    
    self.images = [images sortedArrayUsingComparator:^NSComparisonResult(id a, id b) {
        NSString *first = [[a objectForKey:@"file"] lastPathComponent];
        NSString *second = [[b objectForKey:@"file"] lastPathComponent];
        return [first caseInsensitiveCompare:second];
    }];
    
    // Uncomment the following line to preserve selection between presentations
    // self.clearsSelectionOnViewWillAppear = NO;
    
    // Register cell classes
    [self.collectionView registerClass:[UICollectionViewCell class] forCellWithReuseIdentifier:reuseIdentifier];
    
    // Do any additional setup after loading the view.
    
    [self.collectionView reloadData];
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


- (NSInteger)collectionView:(UICollectionView *)collectionView numberOfItemsInSection:(NSInteger)section {
    return self.images.count;
}

- (UICollectionViewCell *)collectionView:(UICollectionView *)collectionView cellForItemAtIndexPath:(NSIndexPath *)indexPath {
    UICollectionViewCell *cell = [collectionView dequeueReusableCellWithReuseIdentifier:reuseIdentifier forIndexPath:indexPath];
    
    NSDictionary *disk = [self.images objectAtIndex: indexPath.row];
    NSString* diskId = [disk objectForKey:@"serial"];
    
    NSDictionary *game = [self.database getDiskInfo:diskId];

    if (![[game objectForKey:@"boxart"] isEqual:@"404"]) {
        NSString *imageIcon = [[NSString alloc] initWithFormat:@"http://thegamesdb.net/banners/%@", [game objectForKey:@"boxart"]];
        cell.backgroundView = [[UIImageView alloc] initWithImage:[UIImage imageNamed:@"boxart.png"]];
        [(UIImageView *)cell.backgroundView setImageWithURL:[NSURL URLWithString:imageIcon] placeholderImage:[UIImage imageNamed:@"boxart.png"]];
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

-(void)collectionView: (UICollectionView*)collectionView didSelectRowAtIndexPath: (NSIndexPath*)indexPath
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
