#include <stdio.h>
#include "MIPSAnalysis.h"
#include "MIPS.h"

using namespace std;

#define MASK		(0x1FFFFFFF)
#define SHIFT		(3)
#define HASH(a)		((((a) & MASK) << SHIFT) >> 16)

CMIPSAnalysis::CMIPSAnalysis(CMIPS* pCtx)
{
	m_pCtx = pCtx;
}

CMIPSAnalysis::~CMIPSAnalysis()
{

}

void CMIPSAnalysis::Clear()
{
	m_Subroutines.clear();
}

void CMIPSAnalysis::InsertSubroutine(uint32 nStart, uint32 nEnd, uint32 nAllocStart, uint32 nAllocEnd, uint32 nStackSize, uint32 nReturnAddrPos)
{
	uint32 nHashStart, nHashEnd;
	SUBROUTINE Subroutine;

	nHashStart	= HASH(nStart);
	nHashEnd	= HASH(nEnd);

	Subroutine.nStart				= nStart;
	Subroutine.nEnd					= nEnd;
	Subroutine.nStackAllocStart		= nAllocStart;
	Subroutine.nStackAllocEnd		= nAllocEnd;
	Subroutine.nStackSize			= nStackSize;
	Subroutine.nReturnAddrPos		= nReturnAddrPos;

	for(uint32 i = nHashStart; i <= nHashEnd; i++)
	{
		m_Subroutines.insert(SubroutineList::value_type(i, Subroutine));
	}
}

void CMIPSAnalysis::Analyse(uint32 nStart, uint32 nEnd)
{
	uint32 nCandidate, nOp, nTemp;
	uint32 nStackAmount, nReturnAddr;
	int nFound;

    nStart &= ~0x3;
    nEnd &= ~0x3;

	nFound = 0;
	nCandidate = nStart;
    nReturnAddr = 0;

	while(nCandidate != nEnd)
	{
		nOp = m_pCtx->m_pMemoryMap->GetWord(nCandidate);
		if((nOp & 0xFFFF0000) == 0x27BD0000)
		{
            //Found the head of a routine (stack allocation)
			nStackAmount = 0 - (int16)(nOp & 0xFFFF);
			//Look for a JR RA
			nTemp = nCandidate;
			while(nTemp != nEnd)
			{
				nOp = m_pCtx->m_pMemoryMap->GetWord(nTemp);

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
					
					nOp = m_pCtx->m_pMemoryMap->GetWord(nTemp - 4);
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

					nOp = m_pCtx->m_pMemoryMap->GetWord(nTemp + 4);
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
	printf("CMIPSAnalysis: Found %i subroutines in the range [0x%0.8X, 0x%0.8X].\r\n", nFound, nStart, nEnd);
}

CMIPSAnalysis::SUBROUTINE* CMIPSAnalysis::FindSubroutine(uint32 nAddress)
{
	pair<SubroutineList::iterator, SubroutineList::iterator> Iterators;
	Iterators = m_Subroutines.equal_range(HASH(nAddress));
	
	for(SubroutineList::iterator itSubroutine(Iterators.first);
		itSubroutine != Iterators.second; itSubroutine++)
	{
		SUBROUTINE& Subroutine = (*itSubroutine).second;

		if(nAddress >= Subroutine.nStart)
		{
			if(nAddress <= Subroutine.nEnd)
			{
				return &Subroutine;
			}
		}
	}

	return NULL;
}
