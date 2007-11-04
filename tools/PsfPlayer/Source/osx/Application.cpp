#include "Application.h"
#include "DebuggerWnd.h"
#include <string.h>

using namespace std;

const string filename = "/Users/jpd001/226\ Castle\ Zvahl.psf2";

CApplication::CApplication() :
m_virtualMachine(filename.c_str())
{
	m_debuggerWnd = new CDebuggerWnd(m_virtualMachine.GetCpu());
}

CApplication::~CApplication()
{

}

Boolean CApplication::HandleCommand(const HICommandExtended& inCommand)
{
    switch (inCommand.commandID)
    {
	case kHICommandNew:
		{
			NavDialogCreationOptions creationOptions;
			NavGetDefaultDialogCreationOptions(&creationOptions);
			memset(&creationOptions, 0, sizeof(creationOptions));
			creationOptions.version = kNavDialogCreationOptionsVersion;
			creationOptions.optionFlags = kNavDefaultNavDlogOptions;
//			//creationOptions.location = Point(-1, -1);
			creationOptions.clientName = CFSTR("ElfView");

			NavDialogRef dialogRef = NULL;
			OSStatus returnValue = NavCreateChooseFileDialog(&creationOptions, NULL, NULL, NULL, NULL, NULL, &dialogRef);
			NavDialogRun(dialogRef);
			NavReplyRecord reply;
			NavDialogGetReply(dialogRef, &reply);
			NavDisposeReply(&reply);
			NavDialogDispose(dialogRef);
			returnValue++;
		}
		return true;
            
	default:
		return false;
    }
}
