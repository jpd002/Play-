//
//  Playlist.mm
//  PsfPlayer
//
//  Created by Jean-Philip Desjardins on 27/12/08.
//  Copyright 2008 __MyCompanyName__. All rights reserved.
//

#import "Playlist.h"

@implementation Playlist

-(Playlist*)init
{
	m_playList = [[NSMutableArray alloc] init];
	return [super init];
}

-(void)dealloc
{
	[m_playList release];	
	[super dealloc];
}

-(void)save
{
	
}

-(int)numberOfRowsInTableView: (NSTableView*)tableView
{
	return [m_playList count];
}

-(id)tableView: (NSTableView*)tableView objectValueForTableColumn:(NSTableColumn*)tableColumn row:(int)row
{
	if(row >= [m_playList count]) return @"";
	PlaylistItem* item = [m_playList objectAtIndex: row];
	if([[tableColumn identifier] isEqualToString: @"game"])
	{
		return [item game];
	}
	else if([[tableColumn identifier] isEqualToString: @"title"])
	{
		return [item title];
	}
	else if([[tableColumn identifier] isEqualToString: @"length"])
	{
		return [item length];
	}
	return @"";
}

-(void)addItem: (PlaylistItem*)item
{
	[m_playList addObject: item];
}

@end
