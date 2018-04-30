//
//  Playlist.h
//  PsfPlayer
//
//  Created by Jean-Philip Desjardins on 27/12/08.
//  Copyright 2008 __MyCompanyName__. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import "PlaylistItem.h"

@interface Playlist : NSObject
{
	NSMutableArray* m_playList;
}

- (Playlist*)init;
- (void)dealloc;
- (void)save;
- (int)numberOfRowsInTableView:(NSTableView*)tableView;
- (id)tableView:(NSTableView*)tableView objectValueForTableColumn:(NSTableColumn*)tableColumn row:(int)row;
- (void)addItem:(PlaylistItem*)item;
//-(CVfsManagerBinding*)getBindingAt: (unsigned int)index;

@end
