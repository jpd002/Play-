#include "GSH_OpenGLUnix.h"

CGSH_OpenGLUnix::CGSH_OpenGLUnix(Window window)
: m_NativeWindow(window)
{

}


CGSH_OpenGLUnix::~CGSH_OpenGLUnix()
{

}

CGSHandler::FactoryFunction CGSH_OpenGLUnix::GetFactoryFunction(Window window)
{
    return std::bind(&CGSH_OpenGLUnix::GSHandlerFactory, window);
}

void CGSH_OpenGLUnix::InitializeImpl()
{
    SetupContext();
    CGSH_OpenGL::InitializeImpl();

}

void CGSH_OpenGLUnix::PresentBackbuffer()
{
    glXSwapBuffers ( m_NativeDisplay, m_NativeWindow );
}

void CGSH_OpenGLUnix::SetupContext()
{
    m_NativeDisplay = XOpenDisplay(NULL);

    XMapWindow (m_NativeDisplay, m_NativeWindow);

    static int attrListDbl[] =
    {
        GLX_X_RENDERABLE    , True,
        GLX_RED_SIZE        , 8,
        GLX_GREEN_SIZE      , 8,
        GLX_BLUE_SIZE       , 8,
        GLX_DEPTH_SIZE      , 24,
        GLX_DOUBLEBUFFER    , True,
        None
    };

    PFNGLXCHOOSEFBCONFIGPROC glX_ChooseFBConfig = (PFNGLXCHOOSEFBCONFIGPROC) glXGetProcAddress((GLubyte *) "glXChooseFBConfig");
    int fbcount = 0;
    GLXFBConfig *fbc = glX_ChooseFBConfig(m_NativeDisplay, DefaultScreen(m_NativeDisplay), attrListDbl, &fbcount);
    if (!fbc || fbcount < 1) {
        fprintf(stderr, "Failed glX_ChooseFBConfig\n");
    }

    PFNGLXCREATECONTEXTATTRIBSARBPROC glX_CreateContextAttribsARB = (PFNGLXCREATECONTEXTATTRIBSARBPROC)glXGetProcAddress((const GLubyte*) "glXCreateContextAttribsARB");
    if (!glX_CreateContextAttribsARB) {
        fprintf(stderr, "Failed toglX_CreateContextAttribsARB\n");
    }

    // Be sure the handler is installed
    XSync( m_NativeDisplay, false);

#define GLX_CONTEXT_MAJOR_VERSION_ARB       0x2091
#define GLX_CONTEXT_MINOR_VERSION_ARB       0x2092
    // Create a context
    int context_attribs[] =
    {
        GLX_CONTEXT_MAJOR_VERSION_ARB, 3,
        GLX_CONTEXT_MINOR_VERSION_ARB, 3,
        GLX_CONTEXT_PROFILE_MASK_ARB, GLX_CONTEXT_CORE_PROFILE_BIT_ARB,
        None
    };

    m_context = glX_CreateContextAttribsARB(m_NativeDisplay, fbc[0], 0, true, context_attribs);
    XFree(fbc);

    // Get latest error
    XSync( m_NativeDisplay, false);

    if (!m_context) {
        fprintf(stderr, "Failed to create the opengl context. Check your drivers support openGL %d.%d. Hint: opensource drivers don't\n", 3, 3 );
    }

    glXMakeCurrent(m_NativeDisplay, m_NativeWindow, m_context);

    glewExperimental = GL_TRUE;
    GLenum err = glewInit();
    if (err != GLEW_OK){
        fprintf(stderr, "glewInit() error: %s\n", glewGetErrorString(err));
        exit(1); // or handle the error in a nicer way
    }

    if (!GLEW_VERSION_2_1){  // check that the machine supports the 2.1 API.
        fprintf(stderr, "ver\n");
        exit(1); // or handle the error in a nicer way
    }
}

CGSHandler* CGSH_OpenGLUnix::GSHandlerFactory(Window window)
{
    return new CGSH_OpenGLUnix(window);
}

