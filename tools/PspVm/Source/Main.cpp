#include <windows.h>
#include "PspVm.h"
#include "MiniDebugger.h"
#include <boost/filesystem/path.hpp>

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
//	boost::filesystem::path loadPath("c:\\downloads\\ryukyu\\main.prx");
//	boost::filesystem::path loadPath("c:\\downloads\\at3mix\\main.prx");
	boost::filesystem::path loadPath("c:\\downloads\\playsmf\\main.prx.sym");
//	boost::filesystem::path loadPath("H:\\PSP_GAME\\SYSDIR\\BOOT.BIN");

	CPspVm virtualMachine;

#ifdef _DEBUG
	std::string tagPackageName = loadPath.leaf();
	virtualMachine.LoadDebugTags(tagPackageName.c_str());
#endif

	virtualMachine.LoadModule(loadPath.string().c_str());

	CMiniDebugger debugger(virtualMachine, virtualMachine.GetDebugInfo());
	debugger.Center();
	debugger.Show(SW_SHOW);
	debugger.Run();

#ifdef _DEBUG
	virtualMachine.SaveDebugTags(tagPackageName.c_str());
#endif

	return 0;
}
