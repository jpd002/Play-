#ifdef WIN32
#include <windows.h>
#endif
#include "PsfVm.h"
#include "TextDebugger.h"

using namespace std;

#ifdef WIN32
int WINAPI WinMain(HINSTANCE instance, HINSTANCE, char*, int)
#else
int main(int argc, char** argv)
#endif
{
#ifdef WIN32
    AllocConsole();
#endif
    string filename = "C:\\nsf\\kh\\Kingdom Hearts (Sequences Only)\\101 - Dearly Beloved.psf2";
//    string filename = "C:\\nsf\\FF10\\111 Game Over.minipsf2";
    CPsfVm virtualMachine(filename.c_str());

    CTextDebugger debugger(virtualMachine.GetCpu());
    debugger.Run();
    return 1;
}
