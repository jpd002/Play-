#pragma once

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

	typedef std::vector<uint32> CallStackItemArray;

										CMIPSAnalysis(CMIPS*);
										~CMIPSAnalysis();
	void								Analyse(uint32, uint32, uint32 = -1);
	const SUBROUTINE*					FindSubroutine(uint32) const;
	void								Clear();

	void								InsertSubroutine(uint32, uint32, uint32, uint32, uint32, uint32);
	void								ChangeSubroutineStart(uint32, uint32);
	void								ChangeSubroutineEnd(uint32, uint32);

	static CallStackItemArray			GetCallStack(CMIPS*, uint32 pc, uint32 sp, uint32 ra);

private:
	typedef std::map<uint32, SUBROUTINE, std::greater<uint32>> SubroutineList;

	void								AnalyseSubroutines(uint32, uint32, uint32);
	void								AnalyseStringReferences(uint32, uint32);

	CMIPS*								m_ctx;
	SubroutineList						m_subroutines;
};
