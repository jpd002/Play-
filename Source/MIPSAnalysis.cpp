#include <stdio.h>
#include "MIPSAnalysis.h"
#include "MIPS.h"

using namespace Framework;

#define MASK		(0x1FFFFFFF)
#define SHIFT		(3)
#define HASH(a)		((((a) & MASK) << SHIFT) >> 16)

CMIPSAnalysis::CMIPSAnalysis(CMIPS* pCtx)
{
	m_pCtx = pCtx;
	m_Hashed = new CSubList[0x10000];
}

CMIPSAnalysis::~CMIPSAnalysis()
{
	Clear();
	delete [] m_Hashed;
}

void CMIPSAnalysis::Clear()
{
	unsigned int i;

	for(i = 0; i < 0x10000; i++)
	{
		while(m_Hashed[i].Count())
		{
			m_Hashed[i].Pull();
		}
	}

	while(m_Subroutine.Count())
	{
		free(m_Subroutine.Pull());
	}
}

void CMIPSAnalysis::InsertSubroutine(uint32 nStart, uint32 nEnd, uint32 nAllocStart, uint32 nAllocEnd, uint32 nStackSize, uint32 nReturnAddrPos)
{
	uint32 nHashStart, nHashEnd, i;
	MIPSSUBROUTINE* pS;

	nHashStart	= HASH(nStart);
	nHashEnd	= HASH(nEnd);

	pS = (MIPSSUBROUTINE*)malloc(sizeof(MIPSSUBROUTINE));
	pS->nStart				= nStart;
	pS->nEnd				= nEnd;
	pS->nStackAllocStart	= nAllocStart;
	pS->nStackAllocEnd		= nAllocEnd;
	pS->nStackSize			= nStackSize;
	pS->nReturnAddrPos		= nReturnAddrPos;
	m_Subroutine.Insert(pS);

	for(i = nHashStart; i <= nHashEnd; i++)
	{
		m_Hashed[i].Insert(pS);
	}
}

void CMIPSAnalysis::Analyse(uint32 nStart, uint32 nEnd)
{
	uint32 nCandidate, nOp, nTemp;
	uint32 nStackAmount, nReturnAddr;
	int nFound;
	
	nFound = 0;
	nCandidate = nStart;

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
							break;
						}
					}
					//No stack unwinding was found... just forget about this one
					break;
				}
				nTemp += 4;
			}
		}
		nCandidate += 4;
	}
	printf("CMIPSAnalysis: Found %i subroutines in the range [0x%0.8X, 0x%0.8X].\r\n", nFound, nStart, nEnd);
}

MIPSSUBROUTINE* CMIPSAnalysis::FindSubroutine(uint32 nAddress)
{
	MIPSSUBROUTINE* pS;
	CSubList::ITERATOR It;

	for(It = m_Hashed[HASH(nAddress)].Begin(); It.HasNext(); It++)
	{
		pS = (*It);
		if(nAddress >= pS->nStart)
		{
			if(nAddress <= pS->nEnd)
			{
				return pS;
			}
		}
	}

	return NULL;
}
