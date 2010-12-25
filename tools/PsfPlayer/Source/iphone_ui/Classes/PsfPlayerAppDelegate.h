#import <UIKit/UIKit.h>
#include "PsfVm.h"
#include "PlaylistSelectViewController.h"
#include "PlaylistViewController.h"
#include "Playlist.h"

@interface PsfPlayerAppDelegate : NSObject <UIApplicationDelegate, UITabBarControllerDelegate, UIActionSheetDelegate> 
{
    UIWindow*								m_window;
    UITabBarController*						m_tabBarController;
	IBOutlet PlaylistSelectViewController*	m_playlistSelectViewController;
	IBOutlet PlaylistViewController*		m_playlistViewController;

	CPsfVm*									m_virtualMachine;
	CPlaylist*								m_playlist;
	
	UIActionSheet*							m_prebufferOverlay;
}

@property (nonatomic, retain) IBOutlet UIWindow*			m_window;
@property (nonatomic, retain) IBOutlet UITabBarController*	m_tabBarController;

-(IBAction)onOpenPlaylist: (id)sender;
-(void)onPlaylistSelected;
-(void)onPlaylistItemSelected;
-(void)showPrebufferOverlay;
-(void)dismissPrebufferOverlay;

@end
