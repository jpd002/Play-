#pragma once

#include <string>
#include <stack>
#include <thread>
#include <vector>
#include <chrono>
#include "Singleton.h"
#include "Types.h"

class CProfiler : public CSingleton<CProfiler>
{
public:
	typedef uint32 ZoneHandle;

	struct ZONE
	{
		std::string		name;
		uint64			totalTime = 0;
	};

	typedef std::vector<ZONE> ZoneArray;
	typedef std::chrono::high_resolution_clock::time_point TimePoint;

						CProfiler();
	virtual				~CProfiler();

	ZoneHandle			RegisterZone(const char*);

	void				EnterZone(ZoneHandle);
	void				ExitZone();

	ZoneArray			GetStats() const;
	void				Reset();

	void				SetWorkThread();
	
private:
	typedef std::stack<ZoneHandle> ZoneStack;
	
	void				AddTimeToZone(ZoneHandle, uint64);

	ZoneArray			m_zones;
	ZoneStack			m_zoneStack;
	TimePoint			m_currentTime;
	
#ifdef _DEBUG
	std::thread::id		m_workThreadId;
#endif
};

class CProfilerZone
{
public:
							CProfilerZone(CProfiler::ZoneHandle);
							~CProfilerZone();
};
