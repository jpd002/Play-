#ifndef _MIPSANALYSIS_H_
#define _MIPSANALYSIS_H_

#include "Types.h"
#include <map>

class CMIPS;

class CMIPSAnalysis
{
public:
	struct SUBROUTINE
	{
		uint32			nStart;
		uint32			nEnd;
		uint32			nStackAllocStart;
		uint32			nStackAllocEnd;
		uint32			nStackSize;
		uint32			nReturnAddrPos;
	};

										CMIPSAnalysis(CMIPS*);
										~CMIPSAnalysis();
	void								Analyse(uint32, uint32);
	SUBROUTINE*							FindSubroutine(uint32);
	void								Clear();
private:
	typedef std::multimap<uint32, SUBROUTINE> SubroutineList;

	void								InsertSubroutine(uint32, uint32, uint32, uint32, uint32, uint32);
	CMIPS*								m_pCtx;
	SubroutineList						m_Subroutines;
};

#endif
