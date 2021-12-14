#include <cstdio>
#include <emscripten/bind.h>
#include "Ps2VmJs.h"
#include "GSH_OpenGLJs.h"

CPs2VmJs* g_virtualMachine = nullptr;
EMSCRIPTEN_WEBGL_CONTEXT_HANDLE g_context = 0;

int main(int argc, const char** argv)
{
	return 0;
}

extern "C" void initVm()
{
	EmscriptenWebGLContextAttributes attr;
	emscripten_webgl_init_context_attributes(&attr);
	attr.majorVersion = 2;
	attr.minorVersion = 0;
	g_context = emscripten_webgl_create_context("#outputCanvas", &attr);
	assert(g_context >= 0);

	g_virtualMachine = new CPs2VmJs();
	g_virtualMachine->Initialize();
	g_virtualMachine->CreateGSHandler(CGSH_OpenGLJs::GetFactoryFunction(g_context));

	//g_virtualMachine->Reset();
	printf("Reset VM\r\n");
}

void loadElf(std::string path)
{
	printf("Loading '%s'...\r\n", path.c_str());
	g_virtualMachine->m_ee->m_os->BootFromFile(path);
	printf("Starting...\r\n");
	g_virtualMachine->Resume();
}

EMSCRIPTEN_BINDINGS(PsfPlayer)
{
	using namespace emscripten;

	function("loadElf", &loadElf);
}
