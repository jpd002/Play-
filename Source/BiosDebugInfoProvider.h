#ifndef _BIOSDEBUGINFOPROVIDER_H_
#define _BIOSDEBUGINFOPROVIDER_H_

#include <string>
#include <vector>
#include "Types.h"

struct BIOS_DEBUG_MODULE_INFO
{
	std::string		name;
	uint32			begin;
	uint32			end;
	void*			param;
};

typedef std::vector<BIOS_DEBUG_MODULE_INFO> BiosDebugModuleInfoArray;
typedef BiosDebugModuleInfoArray::iterator BiosDebugModuleInfoIterator;

struct BIOS_DEBUG_THREAD_INFO
{
	uint32			id;
	uint32			priority;
	uint32			pc;
	uint32			ra;
	uint32			sp;
	std::string		stateDescription;
};

typedef std::vector<BIOS_DEBUG_THREAD_INFO> BiosDebugThreadInfoArray;

class CBiosDebugInfoProvider
{
public:
	virtual									~CBiosDebugInfoProvider() {}
#ifdef DEBUGGER_INCLUDED
	virtual BiosDebugModuleInfoArray		GetModuleInfos() const = 0;
	virtual BiosDebugThreadInfoArray		GetThreadInfos() const = 0;
#endif
};

#endif
