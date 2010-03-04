#ifndef _MIPSMODULE_H_
#define _MIPSMODULE_H_

#include <string>
#include <list>
#include "Types.h"

//Just used for debugging purposes
struct MIPSMODULE
{
	std::string		name;
	uint32			begin;
	uint32			end;
	void*			param;
};

typedef std::list<MIPSMODULE> MipsModuleList;

#endif
