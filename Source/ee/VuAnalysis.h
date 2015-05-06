#pragma once

#include "Types.h"
#include <map>
#include <vector>

class CMIPS;

class CVuAnalysis
{
public:
	static void		Analyse(CMIPS*, uint32, uint32);

private:
	static uint32	FindBlockStart(CMIPS*, uint32);
};
