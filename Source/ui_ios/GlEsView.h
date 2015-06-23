#import <UIKit/UIKit.h>
#include "opengl/OpenGlDef.h"

@interface GlEsView : UIView
{
@private
	EAGLContext *context;
	
	GLint framebufferWidth;
	GLint framebufferHeight;
	
	GLuint defaultFramebuffer, colorRenderbuffer, depthRenderbuffer;
	
	BOOL hasRetinaDisplay;
}

@property (nonatomic, retain) EAGLContext* context;

@end
