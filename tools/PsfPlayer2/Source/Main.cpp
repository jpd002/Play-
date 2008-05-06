#include "PsfBase.h"
#include "StdStream.h"
#include "PsxVm.h"
#include "MiniDebugger.h"

using namespace Framework;

int main(int argc, char** argv)
{
	CStdStream input("C:\\Media\\LoM_psf\\102 Nostalgic song.PSF", "rb");
	CPsfBase psfFile(input);
	CPsxVm virtualMachine;
	virtualMachine.Reset();
	virtualMachine.LoadExe(psfFile.GetProgram());

    CMiniDebugger debugger(virtualMachine);
    debugger.Show(SW_SHOW);
    debugger.Run();

	return 0;
}
