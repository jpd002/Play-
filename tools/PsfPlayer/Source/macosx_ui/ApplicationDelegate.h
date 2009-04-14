//
//  ApplicationDelegate.h
//  PsfPlayer
//
//  Created by Jean-Philip Desjardins on 26/12/08.
//  Copyright 2008 __MyCompanyName__. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import "Playlist.h"
#include "PsfVm.h"

@interface ApplicationDelegate : NSObject 
{
	CPsfVm*					m_virtualMachine;
	IBOutlet Playlist*		m_playlist;
	IBOutlet NSTableView*	m_playListView;
}

-(id)init;
-(void)OnFileOpen: (id)sender;
-(void)LoadPsf: (NSString*)fileName;

@end
