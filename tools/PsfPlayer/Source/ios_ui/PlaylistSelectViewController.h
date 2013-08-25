#import <UIKit/UIKit.h>

@protocol PlaylistSelectViewControllerDelegate

-(void)onPlaylistSelected: (NSString*)path;

@end

@interface PlaylistSelectViewController : UIViewController
{
	NSMutableArray*		m_archives;
}

@property (nonatomic, assign) id<PlaylistSelectViewControllerDelegate> delegate;

-(IBAction)onCancel: (id)handler;

@end
