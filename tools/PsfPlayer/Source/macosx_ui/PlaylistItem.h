//
//  PlaylistItem.h
//  PsfPlayer
//
//  Created by Jean-Philip Desjardins on 27/12/08.
//  Copyright 2008 __MyCompanyName__. All rights reserved.
//

#import <Cocoa/Cocoa.h>

@interface PlaylistItem : NSObject 
{
	NSString*	m_path;
	NSString*	m_game;
	NSString*	m_title;
	NSString*	m_length;
}

-(id)init: (NSString*)path game: (NSString*)game title: (NSString*)title length: (NSString*)length;
-(void)dealloc;
-(NSString*)game;
-(NSString*)title;
-(NSString*)length;

@end
