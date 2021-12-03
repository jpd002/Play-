#include <cstdio>
#include <emscripten/bind.h>
#include "Ps2VmJs.h"
#include "gs/GSH_Null.h"

CPs2VmJs* g_virtualMachine = nullptr;

int main(int argc, const char** argv)
{
	return 0;
}

extern "C" void initVm()
{
	g_virtualMachine = new CPs2VmJs();
	g_virtualMachine->Initialize();
	g_virtualMachine->CreateGSHandler(CGSH_Null::GetFactoryFunction());

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
