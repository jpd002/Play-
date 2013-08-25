#import <UIKit/UIKit.h>
#import "PlaylistViewController.h"
#import "FileInfoViewController.h"
#import "PsfVm.h"

@interface MainTabBarController : UITabBarController<PlaylistViewControllerDelegate, FileInfoViewControllerDelegate>
{
	CPsfVm*						m_virtualMachine;

	uint64						m_frames;
	bool						m_playing;
	
	FileInfoViewController*		m_fileInfoViewController;
}

@end
