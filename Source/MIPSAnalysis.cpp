#include <stdio.h>
#include "MIPSAnalysis.h"
#include "MIPS.h"

CMIPSAnalysis::CMIPSAnalysis(CMIPS* ctx)
: m_ctx(ctx)
{

}

CMIPSAnalysis::~CMIPSAnalysis()
{

}

void CMIPSAnalysis::Clear()
{
	m_subroutines.clear();
}

void CMIPSAnalysis::Analyse(uint32 start, uint32 end, uint32 entryPoint)
{
	AnalyseSubroutines(start, end, entryPoint);
	AnalyseStringReferences(start, end);
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

void CMIPSAnalysis::ChangeSubroutineStart(uint32 currStart, uint32 newStart)
{
	auto subroutineIterator = m_subroutines.find(currStart);
	assert(subroutineIterator != std::end(m_subroutines));

	SUBROUTINE subroutine(subroutineIterator->second);
	subroutine.nStart = newStart;

	m_subroutines.erase(subroutineIterator);
	m_subroutines.insert(SubroutineList::value_type(newStart, subroutine));
}

void CMIPSAnalysis::ChangeSubroutineEnd(uint32 start, uint32 newEnd)
{
	assert(start < newEnd);

	auto subroutineIterator = m_subroutines.find(start);
	assert(subroutineIterator != std::end(m_subroutines));

	auto& subroutine(subroutineIterator->second);
	subroutine.nEnd = newEnd;
}

void CMIPSAnalysis::AnalyseSubroutines(uint32 nStart, uint32 nEnd, uint32 entryPoint)
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
			uint32 nOp = m_ctx->m_pMemoryMap->GetInstruction(nCandidate);
			if((nOp & 0xFFFF0000) == 0x27BD0000)
			{
				//Found the head of a routine (stack allocation)
				uint32 nStackAmount = 0 - (int16)(nOp & 0xFFFF);
				//Look for a JR RA
				uint32 nTemp = nCandidate;
				while(nTemp != nEnd)
				{
					nOp = m_ctx->m_pMemoryMap->GetInstruction(nTemp);

					//Check SW/SD RA, 0x0000(SP)
					if(
						((nOp & 0xFFFF0000) == 0xAFBF0000) ||		//SW
						((nOp & 0xFFFF0000) == 0xFFBF0000))			//SD
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
					
						nOp = m_ctx->m_pMemoryMap->GetInstruction(nTemp - 4);
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

						nOp = m_ctx->m_pMemoryMap->GetInstruction(nTemp + 4);
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
			uint32 nOp = m_ctx->m_pMemoryMap->GetInstruction(address);
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
				uint32 nOp = m_ctx->m_pMemoryMap->GetInstruction(address);

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

static bool TryGetStringAtAddress(CMIPS* context, uint32 address, std::string& result)
{
	uint8 byteBefore = context->m_pMemoryMap->GetByte(address - 1);
	if(byteBefore != 0) return false;
	while(1)
	{
		uint8 byte = context->m_pMemoryMap->GetByte(address);
		if(byte == 0) break;
		if(byte > 0x7F) return false;
		if((byte < 0x20) && 
			(byte != '\t') &&
			(byte != '\n') &&
			(byte != '\r'))
		{
			return false;
		}
		result += byte;
		address++;
	}
	return (result.length() != 0);
}

void CMIPSAnalysis::AnalyseStringReferences(uint32 start, uint32 end)
{
	for(auto subroutinePair : m_subroutines)
	{
		const auto& subroutine = subroutinePair.second;
		uint32 registerValue[0x20] = { 0 };
		bool registerWritten[0x20] = { false };
		for(uint32 address = subroutine.nStart; address <= subroutine.nEnd; address += 4)
		{
			uint32 op = m_ctx->m_pMemoryMap->GetWord(address);

			//LUI
			if((op & 0xFC000000) == 0x3C000000)
			{
				uint32 rt = (op >> 16) & 0x1F;
				uint32 imm = static_cast<int16>(op);
				registerWritten[rt] = true;
				registerValue[rt] = imm << 16;
			}
			//ADDIU
			else if((op & 0xFC000000) == 0x24000000)
			{
				uint32 rs = (op >> 21) & 0x1F;
				uint32 rt = (op >> 16) & 0x1F;
				uint32 imm = static_cast<int16>(op);
				if((rs == rt) && registerWritten[rs])
				{
					//Check string
					uint32 targetAddress = registerValue[rs] + imm;
					registerWritten[rs] = false;
					if(targetAddress >= start && targetAddress <= end)
					{
						std::string stringConstant;
						if(TryGetStringAtAddress(m_ctx, targetAddress, stringConstant))
						{
							if(m_ctx->m_Comments.Find(address) == nullptr)
							{
								m_ctx->m_Comments.InsertTag(address, stringConstant.c_str());
							}
						}
					}
				}
			}
		}
	}
}

static bool IsValidProgramAddress(uint32 address)
{
	return (address != 0) && ((address & 0x03) == 0);
}

CMIPSAnalysis::CallStackItemArray CMIPSAnalysis::GetCallStack(CMIPS* context, uint32 pc, uint32 sp, uint32 ra)
{
	CallStackItemArray result;

	auto routine = context->m_pAnalysis->FindSubroutine(pc);
	if(!routine)
	{
		if(IsValidProgramAddress(pc)) result.push_back(pc);
		if(pc != ra)
		{
			if(IsValidProgramAddress(ra)) result.push_back(ra);
		}
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
		result.push_back(pc);

		//Go to previous routine
		pc = ra;

		//Check if we can go on...
		routine = context->m_pAnalysis->FindSubroutine(pc);
		if(!routine)
		{
			if(IsValidProgramAddress(ra)) result.push_back(ra);
			break;
		}

		//Get the next RA
		ra = context->m_pMemoryMap->GetWord(sp + routine->nReturnAddrPos);
		sp += routine->nStackSize;
	}

	return result;
}
