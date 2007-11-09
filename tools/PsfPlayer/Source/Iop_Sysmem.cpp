#include "Iop_Sysmem.h"

using namespace Iop;
using namespace std;

CSysmem::CSysmem(uint32 memoryBegin, uint32 memoryEnd) :
m_memoryBegin(memoryBegin),
m_memoryEnd(memoryEnd),
m_memorySize(memoryEnd - memoryBegin)
{
    //Initialize block map
    m_blockMap[m_memorySize] = 0;
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
    default:
        printf("%s(%0.8X): Unknown function (%d) called.\r\n", __FUNCTION__, context.m_State.nPC, functionId);
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

void CSysmem::FreeMemory(uint32 address)
{
    address -= m_memoryBegin;
    BlockMapType::iterator block(m_blockMap.find(address));
    if(block != m_blockMap.end())
    {
        m_blockMap.erase(block);
    }
    else
    {
        printf("%s: Trying to unallocate an unexisting memory block (0x%0.8X).\r\n", __FUNCTION__, address);
    }
}
