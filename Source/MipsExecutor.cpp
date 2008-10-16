#include "MipsExecutor.h"

using namespace std;

CMipsExecutor::CMipsExecutor(CMIPS& context) :
m_context(context)
{

}

CMipsExecutor::~CMipsExecutor()
{
    Clear();
}

void CMipsExecutor::Clear()
{
    for(BlockList::iterator blockIterator(m_blocks.begin());
        blockIterator != m_blocks.end(); blockIterator++)
    {
        delete *blockIterator;
    }
    m_blocks.clear();
    m_blockBegin.clear();
    m_blockEnd.clear();
}

int CMipsExecutor::Execute(int cycles)
{
    CBasicBlock* block = NULL;
    while(cycles > 0)
    {
#ifdef _PSX
        uint32 address = m_context.m_pAddrTranslator(&m_context, 0, m_context.m_State.nPC);
#else
		uint32 address = m_context.m_State.nPC;
#endif
        if(block == NULL || address != block->GetBeginAddress())
        {
            CBasicBlock* prevBlock = block;
            //Check if we can use the hint instead of looking through the map
            if(prevBlock != NULL)
            {
                block = prevBlock->GetBranchHint();
            }
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
            }
            if(prevBlock != NULL)
            {
                prevBlock->SetBranchHint(block);
            }
            if(!block->IsCompiled())
            {
                block->Compile();
            }
        }
        else if(block != NULL)
        {
            block->SetSelfLoopCount(block->GetSelfLoopCount() + 1);
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
#ifdef _PSX
    uint32 currentPc = m_context.m_pAddrTranslator(&m_context, 0, m_context.m_State.nPC);
#else
	uint32 currentPc = m_context.m_State.nPC;
#endif
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
    {
        CBasicBlock* block = FindBlockAt(start);
        if(block != NULL)
        {
            //If the block starts and ends at the same place, block already exists and doesn't need
            //to be re-created
            uint32 otherBegin = block->GetBeginAddress();
            uint32 otherEnd = block->GetEndAddress();
            if((otherBegin == start) && (otherEnd == end))
            {
                return;
            }
            if(otherEnd == end)
            {
                //Repartition the existing block if end of both blocks are the same
                DeleteBlock(block);
                CreateBlock(otherBegin, start - 4);
                assert(FindBlockAt(start) == NULL);
            }
//            else if(otherBegin == start)
//            {
//                DeleteBlock(block);
//                CreateBlock(end + 4, otherEnd);
//                assert(FindBlockAt(end) == NULL);
//            }
            else
            {
                //Delete the currently existing block otherwise
                printf("MipsExecutor: Warning. Deleting block at %0.8X.\r\n", block->GetEndAddress());
                DeleteBlock(block);
            }
        }
    }
    assert(FindBlockAt(end) == NULL);
    {
        CBasicBlock* block = new CBasicBlock(m_context, start, end);
        m_blocks.push_back(block);
        m_blockBegin[start] = block;
        m_blockEnd[end] = block;
    }
    assert(m_blocks.size() == m_blockBegin.size());
    assert(m_blockBegin.size() == m_blockEnd.size());
}

void CMipsExecutor::DeleteBlock(CBasicBlock* block)
{
    //Clear any hints that point to this block
    for(BlockList::const_iterator blockIterator(m_blocks.begin());
        blockIterator != m_blocks.end(); blockIterator++)
    {
        CBasicBlock* currBlock = (*blockIterator);
        if(currBlock->GetBranchHint() == block)
        {
            currBlock->SetBranchHint(NULL);
        }
    }

    //Remove block from our lists
    m_blocks.remove(block);
    m_blockBegin.erase(block->GetBeginAddress());
    m_blockEnd.erase(block->GetEndAddress());
    assert(m_blocks.size() == m_blockBegin.size());
    assert(m_blockBegin.size() == m_blockEnd.size());
    delete block;
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
        if((address - functionAddress) > 0x10000)
        {
            printf("MipsExecutor: Warning. Found no JR after a big distance.\r\n");
            endAddress = address;
            partitionPoints.insert(endAddress);
            break;
        }
        uint32 opcode = m_context.m_pMemoryMap->GetInstruction(address);
        if(opcode == 0x03E00008)
        {
            //+4 for delay slot
            endAddress = address + 4;
            partitionPoints.insert(endAddress + 4);
            break;
        }
    }

    //Find partition points within the function
    for(uint32 address = functionAddress; address <= endAddress; address += 4)
    {
        uint32 opcode = m_context.m_pMemoryMap->GetInstruction(address);
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
        //SYSCALL or ERET
        if(opcode == 0x0000000C || opcode == 0x42000018)
        {
            partitionPoints.insert(address + 4);
        }
        //Check if there's a block already exising that this address
        if(address != endAddress)
        {
            CBasicBlock* possibleBlock = FindBlockStartingAt(address);
            if(possibleBlock != NULL)
            {
                assert(possibleBlock->GetEndAddress() <= endAddress);
                //Add its beginning and end in the partition points
                partitionPoints.insert(possibleBlock->GetBeginAddress());
                partitionPoints.insert(possibleBlock->GetEndAddress() + 4);
            }
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
