//
//  PlaylistItem.mm
//  PsfPlayer
//
//  Created by Jean-Philip Desjardins on 27/12/08.
//  Copyright 2008 __MyCompanyName__. All rights reserved.
//

#import "PlaylistItem.h"


@implementation PlaylistItem

-(id)init: (NSString*)path game: (NSString*)game title: (NSString*)title length: (NSString*)length
{
	m_path = [[NSString alloc] initWithString: path];
	m_game = [[NSString alloc] initWithString: game];
	m_title = [[NSString alloc] initWithString: title];
	m_length = [[NSString alloc] initWithString: length];
	return [super init];
}

-(void)dealloc
{
	[m_path release];
	[m_game release];
	[m_title release];
	[m_length release];
	[super dealloc];
}

-(NSString*)game
{
	return m_game;
}

-(NSString*)title
{
	return m_title;
}

-(NSString*)length
{
	return m_length;
}

@end
