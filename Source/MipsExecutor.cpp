#include "MipsExecutor.h"

using namespace std;

CMipsExecutor::CMipsExecutor(CMIPS& context) :
m_context(context)
{

}

CMipsExecutor::~CMipsExecutor()
{

}

int CMipsExecutor::Execute(int cycles)
{
    CBasicBlock* block = NULL;
    while(cycles > 0)
    {
        uint32 address = m_context.m_State.nPC;
        if(block == NULL || address != block->GetBeginAddress())
        {
            block = FindBlockStartingAt(address);
            if(block == NULL)
            {
                //We need to partition the space and compile the blocks
                PartitionFunction(address);
                block = FindBlockStartingAt(address);
                if(block == NULL)
                {
                    throw runtime_error("Couldn't create block starting at address.");
                }
            }
            if(!block->IsCompiled())
            {
                block->Compile();
            }
        }
        
        cycles -= block->Execute();
        if(m_context.m_State.nHasException) break;
#ifdef DEBUGGER_INCLUDED
        if(MustBreak()) break;
#endif
    }
    return cycles;
}

bool CMipsExecutor::MustBreak()
{
#ifdef DEBUGGER_INCLUDED
    uint32 currentPc = m_context.m_State.nPC;
    CBasicBlock* block = FindBlockAt(currentPc);
    for(CMIPS::BreakpointSet::const_iterator breakPointIterator(m_context.m_breakpoints.begin());
        breakPointIterator != m_context.m_breakpoints.end(); breakPointIterator++)
    {
        uint32 breakPointAddress = *breakPointIterator;
        if(currentPc == breakPointAddress) return true;
        if(block != NULL)
        {
            if(breakPointAddress >= block->GetBeginAddress() && breakPointAddress <= block->GetEndAddress()) return true;
        }
    }
#endif
    return false;
}

CBasicBlock* CMipsExecutor::FindBlockAt(uint32 address)
{
    BlockBeginMap::const_iterator beginIterator = m_blockBegin.lower_bound(address);
    BlockEndMap::const_iterator endIterator = m_blockEnd.lower_bound(address);
    if(beginIterator == m_blockBegin.end()) return NULL;
    if(endIterator == m_blockEnd.end()) return NULL;
    if(beginIterator->second != endIterator->second)
    {
        return NULL;
    }
    return beginIterator->second;
}

CBasicBlock* CMipsExecutor::FindBlockStartingAt(uint32 address)
{
    BlockBeginMap::const_iterator beginIterator = m_blockBegin.find(address);
    if(beginIterator == m_blockBegin.end()) return NULL;
    return beginIterator->second;
}

void CMipsExecutor::CreateBlock(uint32 start, uint32 end)
{
    assert(FindBlockAt(start) == NULL);
    assert(FindBlockAt(end) == NULL);
    CBasicBlock* block = new CBasicBlock(m_context, start, end);
    m_blocks.push_back(block);
    m_blockBegin[start] = block;
    m_blockEnd[end] = block;
}

void CMipsExecutor::PartitionFunction(uint32 functionAddress)
{
    typedef std::set<uint32> PartitionPointSet;
    uint32 endAddress = 0;
    PartitionPointSet partitionPoints;

    //Insert begin point
    partitionPoints.insert(functionAddress);

    //Find the end
    for(uint32 address = functionAddress; ; address += 4)
    {
        //Probably going too far...
        assert((address - functionAddress) <= 0x100000);
        uint32 opcode = m_context.m_pMemoryMap->GetWord(address);
        if(opcode == 0x03E00008)
        {
            //+4 for delay slot
            endAddress = address + 8;
            partitionPoints.insert(endAddress);
            break;
        }
    }

    //Find partition points within the function
    for(uint32 address = functionAddress; address <= endAddress; address += 4)
    {
        uint32 opcode = m_context.m_pMemoryMap->GetWord(address);
        bool isBranch = m_context.m_pArch->IsInstructionBranch(&m_context, address, opcode);
        if(isBranch)
        {
            partitionPoints.insert(address + 8);
            uint32 target = m_context.m_pArch->GetInstructionEffectiveAddress(&m_context, address, opcode);
            if(target > functionAddress && target < endAddress)
            {
                partitionPoints.insert(target);
            }
        }
        //SYSCALL
        if(opcode == 0x0000000C)
        {
            partitionPoints.insert(address + 4);
        }
    }

    uint32 currentPoint = 0;
    for(PartitionPointSet::const_iterator pointIterator(partitionPoints.begin());
        pointIterator != partitionPoints.end(); pointIterator++)
    {
        if(currentPoint != 0)
        {
            CreateBlock(currentPoint, *pointIterator - 4);
        }
        currentPoint = *pointIterator;
    }
}
