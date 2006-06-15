#ifndef _MIPSANALYSIS_H_
#define _MIPSANALYSIS_H_

#include "Types.h"
#include "List.h"

struct MIPSSUBROUTINE
{
	uint32			nStart;
	uint32			nEnd;
	uint32			nStackAllocStart;
	uint32			nStackAllocEnd;
	uint32			nStackSize;
	uint32			nReturnAddrPos;
};

class CMIPS;

typedef Framework::CList<MIPSSUBROUTINE> CSubList;

class CMIPSAnalysis
{
public:
										CMIPSAnalysis(CMIPS*);
										~CMIPSAnalysis();
	void								Analyse(uint32, uint32);
	MIPSSUBROUTINE*						FindSubroutine(uint32);
	void								Clear();
private:
	void								InsertSubroutine(uint32, uint32, uint32, uint32, uint32, uint32);
	CSubList							m_Subroutine;
	CSubList*							m_Hashed;
	CMIPS*								m_pCtx;
};

#endif
