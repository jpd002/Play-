#include <stdio.h>
#include "MIPSAnalysis.h"
#include "MIPS.h"

CMIPSAnalysis::CMIPSAnalysis(CMIPS* pCtx)
: m_pCtx(pCtx)
{

}

CMIPSAnalysis::~CMIPSAnalysis()
{

}

void CMIPSAnalysis::Clear()
{
	m_subroutines.clear();
}

void CMIPSAnalysis::InsertSubroutine(uint32 nStart, uint32 nEnd, uint32 nAllocStart, uint32 nAllocEnd, uint32 nStackSize, uint32 nReturnAddrPos)
{
	assert(FindSubroutine(nStart) == NULL);
	assert(FindSubroutine(nEnd) == NULL);

	SUBROUTINE subroutine;
	subroutine.nStart				= nStart;
	subroutine.nEnd					= nEnd;
	subroutine.nStackAllocStart		= nAllocStart;
	subroutine.nStackAllocEnd		= nAllocEnd;
	subroutine.nStackSize			= nStackSize;
	subroutine.nReturnAddrPos		= nReturnAddrPos;

	m_subroutines.insert(SubroutineList::value_type(nStart, subroutine));
}

void CMIPSAnalysis::ChangeSubroutineStart(uint32 currStart, uint32 newStart)
{
	auto subroutineIterator = m_subroutines.find(currStart);
	assert(subroutineIterator != std::end(m_subroutines));

	SUBROUTINE subroutine(subroutineIterator->second);
	subroutine.nStart = newStart;

	m_subroutines.erase(subroutineIterator);
	m_subroutines.insert(SubroutineList::value_type(newStart, subroutine));
}

void CMIPSAnalysis::Analyse(uint32 nStart, uint32 nEnd, uint32 entryPoint)
{
	nStart &= ~0x3;
	nEnd &= ~0x3;

	int nFound = 0;

	//First pass : Find stack alloc/release ranges
	{
		uint32 nCandidate = nStart;
		while(nCandidate != nEnd)
		{
			uint32 nReturnAddr = 0;
			uint32 nOp = m_pCtx->m_pMemoryMap->GetInstruction(nCandidate);
			if((nOp & 0xFFFF0000) == 0x27BD0000)
			{
				//Found the head of a routine (stack allocation)
				uint32 nStackAmount = 0 - (int16)(nOp & 0xFFFF);
				//Look for a JR RA
				uint32 nTemp = nCandidate;
				while(nTemp != nEnd)
				{
					nOp = m_pCtx->m_pMemoryMap->GetInstruction(nTemp);

					//Check SD RA, 0x0000(SP)
					if((nOp & 0xFFFF0000) == 0xFFBF0000)
					{
						nReturnAddr = (nOp & 0xFFFF);
					}

					//Check for JR RA or J
					if((nOp == 0x03E00008) || ((nOp & 0xFC000000) == 0x08000000))
					{
						//Check if there's a stack unwinding instruction above or below

						//Check above
						//ADDIU SP, SP, 0x????
						//JR RA
					
						nOp = m_pCtx->m_pMemoryMap->GetInstruction(nTemp - 4);
						if((nOp & 0xFFFF0000) == 0x27BD0000)
						{
							if(nStackAmount == (int16)(nOp & 0xFFFF))
							{
								//That's good...
								InsertSubroutine(nCandidate, nTemp + 4, nCandidate, nTemp - 4, nStackAmount, nReturnAddr);
								nCandidate = nTemp + 4;
								nFound++;
								break;
							}
						}

						//Check below
						//JR RA
						//ADDIU SP, SP, 0x????

						nOp = m_pCtx->m_pMemoryMap->GetInstruction(nTemp + 4);
						if((nOp & 0xFFFF0000) == 0x27BD0000)
						{
							if(nStackAmount == (int16)(nOp & 0xFFFF))
							{
								//That's good
								InsertSubroutine(nCandidate, nTemp + 4, nCandidate, nTemp + 4, nStackAmount, nReturnAddr);
								nCandidate = nTemp + 4;
								nFound++;
							}
							break;
						}
						//No stack unwinding was found... just forget about this one
						//break;
					}
					nTemp += 4;
				}
			}
			nCandidate += 4;
		}
	}

	//Second pass : Search for all JAL targets then scan for functions
	{
		std::set<uint32> subroutineAddresses;
		for(uint32 address = nStart; address <= nEnd; address += 4)
		{
			uint32 nOp = m_pCtx->m_pMemoryMap->GetInstruction(address);
			if(
				(nOp & 0xFC000000) == 0x0C000000 ||
				(nOp & 0xFC000000) == 0x08000000)
			{
				uint32 jumpTarget = (nOp & 0x03FFFFFF) * 4;
				if(jumpTarget < nStart) continue;
				if(jumpTarget >= nEnd) continue;
				subroutineAddresses.insert(jumpTarget);
			}
		}

		if(entryPoint != -1)
		{
			subroutineAddresses.insert(entryPoint);
		}

		for(auto subroutineAddressIterator(std::begin(subroutineAddresses));
			subroutineAddressIterator != std::end(subroutineAddresses); ++subroutineAddressIterator)
		{
			uint32 subroutineAddress = *subroutineAddressIterator;
			if(subroutineAddress == 0) continue;

			//Don't bother if we already found it
			if(FindSubroutine(subroutineAddress)) continue;

			//Otherwise, try to find a function that already exists
			for(uint32 address = subroutineAddress; address <= nEnd; address += 4)
			{
				uint32 nOp = m_pCtx->m_pMemoryMap->GetInstruction(address);

				//Check for JR RA or J
				if((nOp == 0x03E00008) || ((nOp & 0xFC000000) == 0x08000000))
				{
					InsertSubroutine(subroutineAddress, address + 4, 0, 0, 0, 0);
					nFound++;
					break;
				}

				auto subroutine = FindSubroutine(address);
				if(subroutine)
				{
					//Function already exists, merge.
					ChangeSubroutineStart(subroutine->nStart, subroutineAddress);
					break;
				}
			}
		}
	}

	printf("CMIPSAnalysis: Found %d subroutines in the range [0x%0.8X, 0x%0.8X].\r\n", nFound, nStart, nEnd);
}

const CMIPSAnalysis::SUBROUTINE* CMIPSAnalysis::FindSubroutine(uint32 nAddress) const
{
	auto subroutineIterator = m_subroutines.lower_bound(nAddress);
	if(subroutineIterator == std::end(m_subroutines)) return nullptr;

	auto& subroutine = subroutineIterator->second;
	if(nAddress >= subroutine.nStart && nAddress <= subroutine.nEnd)
	{
		return &subroutine;
	}
	else
	{
		return nullptr;
	}
}

CMIPSAnalysis::CallStackItemArray CMIPSAnalysis::GetCallStack(CMIPS* context, uint32 pc, uint32 sp, uint32 ra)
{
	CallStackItemArray result;

	auto routine = context->m_pAnalysis->FindSubroutine(pc);
	if(!routine)
	{
		return result;
	}

	//We need to get to a state where we're ready to dig into the previous function's
	//stack

	//Check if we need to check into the stack to get the RA
	if(context->m_pAnalysis->FindSubroutine(ra) == routine)
	{
		ra = context->m_pMemoryMap->GetWord(sp + routine->nReturnAddrPos);
		sp += routine->nStackSize;
	}
	else
	{
		//We haven't called a sub routine yet... The RA is good, but we
		//don't know wether stack memory has been allocated or not
		
		//ADDIU SP, SP, 0x????
		//If the PC is after this instruction, then, we've allocated stack

		if(pc > routine->nStackAllocStart)
		{
			if(pc <= routine->nStackAllocEnd)
			{
				sp += routine->nStackSize;
			}
		}

	}

	while(1)
	{
		//Add the current function
		CALLSTACKITEM item;
		item.function = routine->nStart;
		item.caller = ra - 4;
		result.push_back(item);

		//Go to previous routine
		pc = ra - 4;

		//Check if we can go on...
		routine = context->m_pAnalysis->FindSubroutine(pc);
		if(!routine) break;

		//Get the next RA
		ra = context->m_pMemoryMap->GetWord(sp + routine->nReturnAddrPos);
		sp += routine->nStackSize;
	}

	return result;
}
