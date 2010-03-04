#include "Log.h"
#include "Psp_KernelLibrary.h"
#include "COP_SCU.h"

#define LOGNAME		("Psp_KernelLibrary")

using namespace Psp;

CKernelLibrary::CKernelLibrary()
{

}

CKernelLibrary::~CKernelLibrary()
{

}

std::string CKernelLibrary::GetName() const
{
	return "Kernel_Library";
}

void CKernelLibrary::Invoke(uint32 methodId, CMIPS& context)
{
	switch(methodId)
	{
	case 0x092968F4:
		//CpuSuspendIntr
		context.m_State.nCOP0[CCOP_SCU::STATUS] &= ~0x00010001;
		break;
	case 0x5F10D406:
		//CpuResumeIntr
		context.m_State.nCOP0[CCOP_SCU::STATUS] |= 0x00010001;
		break;
	default:
		CLog::GetInstance().Print(LOGNAME, "Unknown function called 0x%0.8X.\r\n", methodId);
		break;
	}
}
