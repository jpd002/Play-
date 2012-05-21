#ifndef _MIPSANALYSIS_H_
#define _MIPSANALYSIS_H_

#include "Types.h"
#include <map>
#include <vector>

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

	struct CALLSTACKITEM
	{
		uint32			function;
		uint32			caller;
	};

	typedef std::vector<CALLSTACKITEM> CallStackItemArray;

										CMIPSAnalysis(CMIPS*);
										~CMIPSAnalysis();
	void								Analyse(uint32, uint32, uint32 = -1);
	const SUBROUTINE*					FindSubroutine(uint32) const;
	void								Clear();

	static CallStackItemArray			GetCallStack(CMIPS*, uint32 pc, uint32 sp, uint32 ra);

private:
	typedef std::map<uint32, SUBROUTINE, std::greater<uint32>> SubroutineList;

	void								InsertSubroutine(uint32, uint32, uint32, uint32, uint32, uint32);
	void								ChangeSubroutineStart(uint32, uint32);

	CMIPS*								m_pCtx;
	SubroutineList						m_subroutines;
};

#endif
