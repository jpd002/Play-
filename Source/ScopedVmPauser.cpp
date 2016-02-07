#include "ScopedVmPauser.h"
#include "VirtualMachine.h"

CScopedVmPauser::CScopedVmPauser(CVirtualMachine& virtualMachine)
: m_virtualMachine(virtualMachine)
{
	if(m_virtualMachine.GetStatus() == CVirtualMachine::RUNNING)
	{
		m_paused = true;
		m_virtualMachine.Pause();
	}
}

CScopedVmPauser::~CScopedVmPauser()
{
	if(m_paused)
	{
		m_virtualMachine.Resume();
	}
}
