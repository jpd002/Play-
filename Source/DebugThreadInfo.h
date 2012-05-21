#ifndef _DEBUGTHREADINFO_H_
#define _DEBUGTHREADINFO_H_

struct DEBUG_THREAD_INFO
{
	uint32			id;
	uint32			priority;
	uint32			pc;
	uint32			ra;
	uint32			sp;
	std::string		stateDescription;
};

typedef std::vector<DEBUG_THREAD_INFO> DebugThreadInfoArray;

#endif
