#include <windows.h>
#include "ElfViewFrame.h"
#include <boost/algorithm/string.hpp>

using namespace Framework;
using namespace std;
using namespace boost;

int WINAPI WinMain(HINSTANCE instance, HINSTANCE prevInstance, LPSTR params, int showCmd)
{
    try
    {
        string path(params);
        erase_all(path, "\"");
        CElfViewFrame elfViewFrame(path.c_str());
        elfViewFrame.Center();
        elfViewFrame.Show(SW_SHOW);
        Win32::CWindow::StdMsgLoop(&elfViewFrame);
    }
    catch(const exception& except)
    {
        MessageBoxA(NULL, except.what(), NULL, 16);
//        MessageBoxA(NULL, params, NULL, 16);
    }
    return 0;
}
