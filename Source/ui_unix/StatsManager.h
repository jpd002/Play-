#pragma once

#include <mutex>
#include <map>
#include "Types.h"
#include "Singleton.h"
#include "Profiler.h"

class CStatsManager : public CSingleton<CStatsManager>
{
public:
	void			OnNewFrame(uint32);
	
	uint32			GetFrames();
	uint32			GetDrawCalls();
#ifdef PROFILE
	std::string		GetProfilingInfo();
#endif

	void			ClearStats();
	
#ifdef PROFILE
	void			OnProfileFrameDone(const CProfiler::ZoneArray&);
#endif
	
private:
	std::mutex		m_statsMutex;
	
	uint32			m_frames = 0;
	uint32			m_drawCalls = 0;
	
#ifdef PROFILE
	struct ZONEINFO
	{
		uint64 currentValue = 0;
		uint64 minValue = ~0ULL;
		uint64 maxValue = 0;
	};

	typedef std::map<std::string, ZONEINFO> ZoneMap;

	std::mutex				m_profilerZonesMutex;
	ZoneMap					m_profilerZones;
#endif
};
