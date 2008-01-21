#include "Iop_Sysmem.h"
#include "../Log.h"

using namespace Iop;
using namespace std;

#define LOG_NAME ("iop_sysmem")

CSysmem::CSysmem(uint32 memoryBegin, uint32 memoryEnd, CStdio& stdio, CSIF& sif) :
m_memoryBegin(memoryBegin),
m_memoryEnd(memoryEnd),
m_stdio(stdio),
m_memorySize(memoryEnd - memoryBegin)
{
    //Initialize block map
    m_blockMap[m_memorySize] = 0;

    //Register sif module
    sif.RegisterModule(MODULE_ID, this);
}

CSysmem::~CSysmem()
{

}

string CSysmem::GetId() const
{
    return "sysmem";
}

void CSysmem::Invoke(CMIPS& context, unsigned int functionId)
{
    switch(functionId)
    {
    case 4:
        context.m_State.nGPR[CMIPS::V0].nD0 = static_cast<int32>(AllocateMemory(
            context.m_State.nGPR[CMIPS::A1].nV[0],
            context.m_State.nGPR[CMIPS::A0].nV[0]
            ));
        break;
    case 5:
        context.m_State.nGPR[CMIPS::V0].nD0 = static_cast<int32>(FreeMemory(
            context.m_State.nGPR[CMIPS::A0].nV[0]
            ));
        break;
    case 14:
        m_stdio.__printf(context);
        break;
    default:
        CLog::GetInstance().Print(LOG_NAME, "%s(%0.8X): Unknown function (%d) called.\r\n", __FUNCTION__, context.m_State.nPC, functionId);
        break;
    }
}

void CSysmem::Invoke(uint32 method, uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	switch(method)
	{
	case 0x01:
		assert(retSize == 4);
		ret[0] = SifAllocate(args[0]);
		break;
	case 0x04:
		assert(retSize == 4);
		ret[0] = SifAllocateSystemMemory(args[0], args[1], args[2]);
		break;
	default:
        CLog::GetInstance().Print(LOG_NAME, "Unknown method invoked (0x%0.8X).\r\n", method);
		break;
	}
}

uint32 CSysmem::AllocateMemory(uint32 size, uint32 flags)
{
    uint32 begin = 0;
    const uint32 blockSize = 0x400;
    size = ((size + (blockSize - 1)) / blockSize) * blockSize;
    for(BlockMapType::iterator blockIterator(m_blockMap.begin());
        blockIterator != m_blockMap.end(); blockIterator++)
    {
        uint32 end = blockIterator->first;
        if((end - begin) >= size)
        {
            break;
        }
        begin = blockIterator->first + blockIterator->second;
    }
    if(begin != m_memorySize)
    {
        m_blockMap[begin] = size;
        return begin + m_memoryBegin;
    }
    return NULL;
}

uint32 CSysmem::FreeMemory(uint32 address)
{
    address -= m_memoryBegin;
    BlockMapType::iterator block(m_blockMap.find(address));
    if(block != m_blockMap.end())
    {
        m_blockMap.erase(block);
    }
    else
    {
        CLog::GetInstance().Print(LOG_NAME, "%s: Trying to unallocate an unexisting memory block (0x%0.8X).\r\n", __FUNCTION__, address);
    }
    return 0;
}

uint32 CSysmem::SifAllocate(uint32 nSize)
{
	CLog::GetInstance().Print(LOG_NAME, "Allocate(size = 0x%0.8X);\r\n", nSize);
	//return 0x01;
	return nSize;
}

uint32 CSysmem::SifAllocateSystemMemory(uint32 nFlags, uint32 nSize, uint32 nPtr)
{
	//Ys 1&2 Eternal Story calls this
	CLog::GetInstance().Print(LOG_NAME, "AllocateSystemMemory(flags = 0x%0.8X, size = 0x%0.8X, ptr = 0x%0.8X);\r\n", nFlags, nSize, nPtr);
	return 0x01;
}
