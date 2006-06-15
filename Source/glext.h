#ifndef _GLEXT_H_
#define _GLEXT_H_

#ifndef GL_FOG_COORDINATE_SOURCE_EXT
#define GL_FOG_COORDINATE_SOURCE_EXT	(0x8450)
#endif

#ifndef GL_FOG_COORDINATE_EXT
#define GL_FOG_COORDINATE_EXT			(0x8451)
#endif

#ifndef GL_UNSIGNED_SHORT_1_5_5_5_REV
#define GL_UNSIGNED_SHORT_1_5_5_5_REV	(0x8366)
#endif

#ifndef GL_CONSTANT_ALPHA_EXT
#define GL_CONSTANT_ALPHA_EXT			(0x8003)
#endif

#ifndef GL_FUNC_ADD_EXT
#define GL_FUNC_ADD_EXT					(0x8006)
#endif

#ifndef GL_FUNC_REVERSE_SUBTRACT_EXT
#define GL_FUNC_REVERSE_SUBTRACT_EXT	(0x800B)
#endif

#ifndef GL_ONE_MINUS_CONSTANT_ALPHA_EXT
#define GL_ONE_MINUS_CONSTANT_ALPHA_EXT	(0x8004)
#endif

#ifndef GL_BGRA
#define GL_BGRA							(0x80E1)
#endif

#ifndef GL_CLAMP_TO_EDGE  
#define GL_CLAMP_TO_EDGE				(0x812F)
#endif

typedef void (APIENTRY * PFNGLBLENDCOLOREXTPROC)(GLclampf, GLclampf, GLclampf, GLclampf);
typedef void (APIENTRY * PFNGLBLENDEQUATIONEXTPROC)(GLenum);
typedef void (APIENTRY * PFNGLFOGCOORDFEXTPROC)(GLfloat);

#endif
