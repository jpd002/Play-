#pragma once

#include "Types.h"
#include <map>
#include <vector>
#include <functional>

class CMIPS;

class CMIPSAnalysis
{
public:
	struct SUBROUTINE
	{
		uint32			start;
		uint32			end;
		uint32			stackAllocStart;
		uint32			stackAllocEnd;
		uint32			stackSize;
		uint32			returnAddrPos;
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
	void								AnalyseStringReferences();

	void								FindSubroutinesByStackAllocation(uint32, uint32);
	void								FindSubroutinesByJumpTargets(uint32, uint32, uint32);
	void								ExpandSubroutines(uint32, uint32);

	CMIPS*								m_ctx;
	SubroutineList						m_subroutines;
};
