#ifndef _PROFILER_H_
#define _PROFILER_H_

#ifdef PROFILE

#include <string>
#include <ctime>
#include <map>
#include <stack>
#include <thread>
#include "Singleton.h"

#define PROFILE_OTHERZONE "Other"

class CProfiler : public CSingleton<CProfiler>
{
public:
	typedef std::map<std::string, clock_t> ZoneMap;
		
	friend				CSingleton;

	void				BeginIteration();
	void				EndIteration();

	void				BeginZone(const char*);
	void				EndZone();

	ZoneMap				GetStats();
	void				Reset();

	void				SetWorkThread();
	
private:
	typedef std::stack<std::string> ZoneStack;
	
						CProfiler();
	virtual				~CProfiler();

	void				AddTimeToCurrentZone(clock_t);

	std::mutex			m_mutex;
	clock_t				m_currentTime;
	ZoneMap				m_zones;
	ZoneStack			m_zoneStack;
	
#ifdef _DEBUG
	std::thread::id		m_workThreadId;
#endif
};

class CProfilerZone
{
public:
					CProfilerZone(const char*);
					~CProfilerZone();
	
private:
	const char*		m_name;
};

#endif

#endif
