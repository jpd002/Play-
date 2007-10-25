#include <windows.h>
//#include "..\Purei\Source\ELF.h"
//#include "..\Purei\Source\win32ui\ELFView.h"
//#include "StdStream.h"
#include "ElfViewFrame.h"

using namespace Framework;

int WINAPI WinMain(HINSTANCE instance, HINSTANCE prevInstance, LPSTR params, int showCmd)
{
    CElfViewFrame elfViewFrame;
    elfViewFrame.Center();
    elfViewFrame.Show(SW_SHOW);
//    CStdStream stream("D:\\rar\\Purei\\psf2.irx", "rb");    
//    CELF elf(&stream);
//    CELFView elfView(NULL);
//    elfView.SetELF(&elf);
//    elfView.Show(SW_SHOW);
    Win32::CWindow::StdMsgLoop(&elfViewFrame);
    return 0;
}
