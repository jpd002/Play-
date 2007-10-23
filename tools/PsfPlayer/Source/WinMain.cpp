#include <windows.h>
#include "PsfFs.h"

using namespace std;

int WINAPI WinMain(HINSTANCE instance, HINSTANCE prevInstance, char* params, int showCmd)
{
    string filename = "C:\\nsf\\kh\\Kingdom Hearts (Sequences Only)\\101 - Dearly Beloved.psf2";
//    string filename = "C:\\nsf\\FF10\\111 Game Over.minipsf2";
    CPsfFs PsfFs(filename.c_str());
    const CPsfFs::FILE* file = PsfFs.GetFile("psf2.irx");

    return 1;
}
