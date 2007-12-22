#import <Cocoa/Cocoa.h>
#import "../MIPS.h"

@interface CRegView : NSView 
{
	CMIPS*	m_context;
}

-(void)drawRect : (NSRect)rect;
-(void)setContext : (CMIPS*)context;
-(bool)isFlipped;
-(void)onMachineStateChange;

@end
