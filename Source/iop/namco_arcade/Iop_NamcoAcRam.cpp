#include "Iop_NamcoAcRam.h"
#include "../../Log.h"

using namespace Iop;
using namespace Iop::Namco;

#define LOG_NAME ("iop_namco_acram")

#define FUNCTION_INIT "Init"
#define FUNCTION_READ "Read"
#define FUNCTION_WRITE "Write"

CAcRam::CAcRam(uint8* iopRam)
	: m_iopRam(iopRam)
{
}

std::string CAcRam::GetId() const
{
	return "acram";
}

std::string CAcRam::GetFunctionName(unsigned int functionId) const
{
	switch(functionId)
	{
	case 8:
		return FUNCTION_INIT;
	case 9:
		return FUNCTION_READ;
	case 11:
		return FUNCTION_WRITE;
	default:
		return "unknown";
	}
}

void CAcRam::Invoke(CMIPS& context, unsigned int functionId)
{
	switch(functionId)
	{
	case 8:
		{
			uint32 handlePtr = context.m_State.nGPR[CMIPS::A0].nV0;
			uint32 ptr1 = context.m_State.nGPR[CMIPS::A1].nV0;
			uint32 ptr2 = context.m_State.nGPR[CMIPS::A2].nV0;
			uint32 ptr3 = context.m_State.nGPR[CMIPS::A3].nV0;
			CLog::GetInstance().Warn(LOG_NAME, "Init(handlePtr = 0x%08X, ptr1 = 0x%08X, ptr2 = 0x%08X, ptr3 = 0x%08X);\r\n",
									 handlePtr, ptr1, ptr2, ptr3);
			assert(handlePtr != 0);
			context.m_pMemoryMap->SetWord(handlePtr + 0x08, ptr1);
			context.m_pMemoryMap->SetWord(handlePtr + 0x0C, ptr2);
			context.m_pMemoryMap->SetWord(handlePtr + 0x1C, ptr3);
			context.m_State.nGPR[CMIPS::V0].nV0 = handlePtr;
		}
		break;
	case 9:
		{
			uint32 handlePtr = context.m_State.nGPR[CMIPS::A0].nV0;
			uint32 ramAddr = context.m_State.nGPR[CMIPS::A1].nV0;
			uint32 iopAddr = context.m_State.nGPR[CMIPS::A2].nV0;
			uint32 size = context.m_State.nGPR[CMIPS::A3].nV0;
			CLog::GetInstance().Warn(LOG_NAME, "Read(infoPtr = 0x%08X, ramAddr = 0x%08X, iopAddr = 0x%08X, size = %d);\r\n",
									 handlePtr, ramAddr, iopAddr, size);
			assert((ramAddr + size) <= g_extRamSize);
			memcpy(m_iopRam + iopAddr, m_extRam + ramAddr, size);

			uint32 otherResultPtr = context.m_pMemoryMap->GetWord(handlePtr + 0x0C);
			context.m_pMemoryMap->SetWord(otherResultPtr, ~0U);
			context.m_State.nGPR[CMIPS::V0].nV0 = 1; //?
		}
		break;
	case 11:
		{
			uint32 handlePtr = context.m_State.nGPR[CMIPS::A0].nV0;
			uint32 ramAddr = context.m_State.nGPR[CMIPS::A1].nV0;
			uint32 iopAddr = context.m_State.nGPR[CMIPS::A2].nV0;
			uint32 size = context.m_State.nGPR[CMIPS::A3].nV0;
			CLog::GetInstance().Warn(LOG_NAME, "Write(infoPtr = 0x%08X, dstAddr = 0x%08X, srcAddr = 0x%08X, size = %d);\r\n",
									 handlePtr, ramAddr, iopAddr, size);
			assert((ramAddr + size) <= g_extRamSize);
			memcpy(m_extRam + ramAddr, m_iopRam + iopAddr, size);
			uint32 otherResultPtr = context.m_pMemoryMap->GetWord(handlePtr + 0x0C);
			context.m_pMemoryMap->SetWord(otherResultPtr, ~0U);
			context.m_State.nGPR[CMIPS::V0].nV0 = 1; //?
		}
		break;
	default:
		CLog::GetInstance().Warn(LOG_NAME, "Unknown method invoked (0x%08X).\r\n", functionId);
		break;
	}
}
