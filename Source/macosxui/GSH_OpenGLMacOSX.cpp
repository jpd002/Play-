#include "GSH_OpenGLMacOSX.h"
#include "CoreFoundation/CoreFoundation.h"
#include "StdStream.h"

CGSH_OpenGLMacOSX::CGSH_OpenGLMacOSX(CGLContextObj context) :
m_context(context)
{

}

CGSH_OpenGLMacOSX::~CGSH_OpenGLMacOSX()
{

}

CGSHandler::FactoryFunction CGSH_OpenGLMacOSX::GetFactoryFunction(CGLContextObj context)
{
	return std::bind(&CGSH_OpenGLMacOSX::GSHandlerFactory, context);
}

void CGSH_OpenGLMacOSX::InitializeImpl()
{
	CGLSetCurrentContext(m_context);
	CGSH_OpenGL::InitializeImpl();
}

void CGSH_OpenGLMacOSX::ReleaseImpl()
{
	
}

void CGSH_OpenGLMacOSX::FlipImpl()
{
	CGLFlushDrawable(m_context);
}

void CGSH_OpenGLMacOSX::UpdateViewportImpl()
{
	
}

std::string GetBundleFilePath(CFStringRef filename, CFStringRef extension)
{
	CFBundleRef bundle = CFBundleGetMainBundle();
	CFURLRef url = CFBundleCopyResourceURL(bundle, filename, extension, NULL);
	if(url == NULL)
	{
		throw std::exception();
	}
	CFStringRef pathString = CFURLCopyFileSystemPath(url, kCFURLPOSIXPathStyle);
	const char* pathCString = CFStringGetCStringPtr(pathString, kCFStringEncodingMacRoman);
	if(pathCString == NULL) 
	{
		throw std::exception();
	}
	return std::string(pathCString);
}

void CGSH_OpenGLMacOSX::ReadFramebuffer(uint32, uint32, void*)
{
	
}

void CGSH_OpenGLMacOSX::PresentBackbuffer()
{
	
}

void CGSH_OpenGLMacOSX::LoadShaderSource(Framework::OpenGl::CShader* shader, SHADER shaderId)
{
	CFStringRef extension = NULL;
	switch(shaderId)
	{
	case SHADER_VERTEX:
		extension = CFSTR("vert");
		break;
	case SHADER_FRAGMENT:
		extension = CFSTR("frag");
		break;
	default:
		throw std::exception();
		break;
	}
	std::string shaderPath = GetBundleFilePath(CFSTR("TexClamp"), extension);
	Framework::CStdStream stream(shaderPath.c_str(), "rb");
	stream.Seek(0, Framework::STREAM_SEEK_END);
	uint64 fileSize = stream.Tell();
	stream.Seek(0, Framework::STREAM_SEEK_SET);
	char* readBuffer = new char[fileSize];
	stream.Read(readBuffer, fileSize);
	shader->SetSource(readBuffer, fileSize);
	delete [] readBuffer;
}

void CGSH_OpenGLMacOSX::ProcessImageTransfer(uint32, uint32)
{
	
}

CGSHandler* CGSH_OpenGLMacOSX::GSHandlerFactory(CGLContextObj context)
{
	return new CGSH_OpenGLMacOSX(context);
}
