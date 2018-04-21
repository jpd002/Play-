#include "ElfViewFrame.h"
#include <boost/algorithm/string.hpp>
#include <windows.h>

int WINAPI WinMain(HINSTANCE instance, HINSTANCE prevInstance, LPSTR params, int showCmd)
{
	std::string path(params);
	boost::erase_all(path, "\"");
	CElfViewFrame elfViewFrame(path.c_str());
	elfViewFrame.Center();
	elfViewFrame.Show(SW_SHOW);
	Framework::Win32::CWindow::DlgMsgLoop(elfViewFrame);
	return 0;
}
