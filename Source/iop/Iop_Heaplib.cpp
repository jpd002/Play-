#include "Iop_Heaplib.h"
#include "../Log.h"

using namespace Iop;

#define LOG_NAME ("iop_heaplib")

#define HEAP_PTR 0x12121212

#define FUNCTION_CREATEHEAP "CreateHeap"
#define FUNCTION_ALLOCHEAPMEMORY "AllocHeapMemory"
#define FUNCTION_FREEHEAPMEMORY "FreeHeapMemory"

CHeaplib::CHeaplib(CSysmem& sysMem)
    : m_sysMem(sysMem)
{
}

std::string CHeaplib::GetId() const
{
	return "heaplib";
}

std::string CHeaplib::GetFunctionName(unsigned int functionId) const
{
	switch(functionId)
	{
	case 4:
		return FUNCTION_CREATEHEAP;
		break;
	case 6:
		return FUNCTION_ALLOCHEAPMEMORY;
		break;
	case 7:
		return FUNCTION_FREEHEAPMEMORY;
		break;
	default:
		return "unknown";
		break;
	}
}

void CHeaplib::Invoke(CMIPS& context, unsigned int functionId)
{
	switch(functionId)
	{
	case 4:
		context.m_State.nGPR[CMIPS::V0].nD0 = CreateHeap(
		    context.m_State.nGPR[CMIPS::A0].nV0,
		    context.m_State.nGPR[CMIPS::A1].nV0);
		break;
	case 6:
		context.m_State.nGPR[CMIPS::V0].nD0 = AllocHeapMemory(
		    context.m_State.nGPR[CMIPS::A0].nV0,
		    context.m_State.nGPR[CMIPS::A1].nV0);
		break;
	case 7:
		context.m_State.nGPR[CMIPS::V0].nD0 = FreeHeapMemory(
		    context.m_State.nGPR[CMIPS::A0].nV0,
		    context.m_State.nGPR[CMIPS::A1].nV0);
		break;
	default:
		CLog::GetInstance().Warn(LOG_NAME, "Unknown function called (%d).\r\n",
		                         functionId);
		break;
	}
}

int32 CHeaplib::CreateHeap(uint32 heapSize, uint32 flags)
{
	CLog::GetInstance().Print(LOG_NAME, FUNCTION_CREATEHEAP "(heapSize = 0x%08X, flags = %d);\r\n",
	                          heapSize, flags);
	return HEAP_PTR;
}

int32 CHeaplib::AllocHeapMemory(uint32 heapPtr, uint32 size)
{
	CLog::GetInstance().Print(LOG_NAME, FUNCTION_ALLOCHEAPMEMORY "(heapPtr = 0x%08X, size = 0x%08X);\r\n",
	                          heapPtr, size);
	assert(heapPtr == HEAP_PTR);
	return m_sysMem.AllocateMemory(size, 0, 0);
}

int32 CHeaplib::FreeHeapMemory(uint32 heapPtr, uint32 allocPtr)
{
	CLog::GetInstance().Print(LOG_NAME, FUNCTION_FREEHEAPMEMORY "(heapPtr = 0x%08X, allocPtr = 0x%08X);\r\n",
	                          heapPtr, allocPtr);
	assert(heapPtr == HEAP_PTR);
	m_sysMem.FreeMemory(allocPtr);
	return 0;
}
