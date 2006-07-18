#ifndef _PROFILER_H_
#define _PROFILER_H_

#include <boost/ptr_container/ptr_map.hpp>
#include <boost/thread.hpp>
#include <string>
#include <list>
#include <ctime>
#include "Singleton.h"

#ifdef PROFILE

class CProfiler : public CSingleton<CProfiler>
{
public:
	friend				CSingleton;

	class CZone
	{
	public:
						CZone(const char*);
		void			AddTime(clock_t);
		const char*		GetName() const;
		clock_t			GetTime() const;
		void			Reset();

	private:
		std::string		m_sName;
		clock_t			m_nTime;
	};

	typedef std::list<CZone> ZoneList;

	void				BeginIteration();
	void				EndIteration();

	void				BeginZone(const char*);
	void				EndZone();

	ZoneList			GetStats();
	void				Reset();

private:
						CProfiler();
	virtual				~CProfiler();

	CZone&				GetCurrentZone();

	typedef boost::ptr_map<std::string, CZone>	ZoneMap;
	typedef std::list<CZone*>					ZoneStack;

	boost::mutex		m_Mutex;
	CZone				m_OtherZone;
	clock_t				m_nCurrentTime;
	ZoneMap				m_Zones;
	ZoneStack			m_ZoneStack;
	ZoneList			m_Stats;
};

#endif

#endif
