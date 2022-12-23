#import <UIKit/UIKit.h>

@protocol PlaylistSelectViewControllerDelegate

- (void)onPlaylistSelected:(NSString*)path;

@end

@interface PlaylistSelectViewController : UIViewController
{
	NSArray* m_archives;
}

@property(nonatomic, assign) id<PlaylistSelectViewControllerDelegate> delegate;

- (IBAction)onCancel:(id)handler;

@end
