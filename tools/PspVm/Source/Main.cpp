#include <windows.h>
#include "PspVm.h"
#include "MiniDebugger.h"
#include <boost/filesystem/path.hpp>
#include "PsfBase.h"
#include "StdStream.h"

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
//	boost::filesystem::path loadPath("c:\\downloads\\ryukyu\\main.prx");
//	boost::filesystem::path loadPath("c:\\downloads\\at3mix\\main.prx");
//	boost::filesystem::path loadPath("c:\\downloads\\playsmf\\main.prx.sym");
//	boost::filesystem::path loadPath("H:\\PSP_GAME\\SYSDIR\\BOOT.BIN");
	boost::filesystem::path loadPath("G:\\Projects\\Purei\\tools\\PsfPacker\\output\\Impression.psfp");

	CPspVm virtualMachine;
	CPsfBase psfFile(Framework::CStdStream(loadPath.string().c_str(), "rb"));
	virtualMachine.GetBios().GetPsfDevice()->AppendArchive(psfFile);

#ifdef _DEBUG
	std::string tagPackageName = loadPath.leaf();
	virtualMachine.LoadDebugTags(tagPackageName.c_str());
#endif

	virtualMachine.LoadModule("host0:/PSP_GAME/SYSDIR/BOOT.BIN");

	CMiniDebugger debugger(virtualMachine, virtualMachine.GetDebugInfo());
	debugger.Center();
	debugger.Show(SW_SHOW);
	debugger.Run();

#ifdef _DEBUG
	virtualMachine.SaveDebugTags(tagPackageName.c_str());
#endif

	return 0;
}
