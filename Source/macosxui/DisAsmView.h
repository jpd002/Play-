#import <Cocoa/Cocoa.h>
#import "../MIPS.h"

@interface CDisAsmView : NSView 
{
	CMIPS* m_context;
	uint32 m_viewAddress;
	uint32 m_selectionAddress;
	NSFont* m_font;
	float m_textHeight;
}

-(id)initWithFrame : (NSRect)frameRect;
-(void)dealloc;
-(bool)acceptsFirstResponder;
-(void)drawRect : (NSRect)rect;
-(void)gotoPc: (id)sender;
-(bool)isFlipped;
-(void)setContext : (CMIPS*)context;
-(void)keyDown : (NSEvent*)event;
-(void)mouseDown : (NSEvent*)event;
-(void)ensurePcVisible;
-(void)onMachineStateChange;
-(void)onDownArrowKey;
-(void)onUpArrowKey;
-(bool)isAddressInvisible: (uint32)address;
-(uint32)maxViewAddress;

@end
