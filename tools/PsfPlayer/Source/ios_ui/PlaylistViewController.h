#import <UIKit/UIKit.h>
#import "Playlist.h"
#import "PlaylistSelectViewController.h"

@protocol PlaylistViewControllerDelegate

-(void)onPlaylistSelected: (CPlaylist*)playlist;
-(void)onPlaylistItemSelected: (unsigned int)itemIndex;

@end

@interface PlaylistViewController : UIViewController<PlaylistSelectViewControllerDelegate>
{
	CPlaylist*				m_playlist;
	IBOutlet UITableView*	m_tableView;
}

@property (nonatomic, assign) IBOutlet id<PlaylistViewControllerDelegate> delegate;

-(IBAction)onOpenPlaylist: (id)sender;

@end
