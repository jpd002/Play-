#pragma once

#include <mutex>
#include <map>
#include "Types.h"
#include "Singleton.h"
#include "Profiler.h"
#include "../PS2VM.h"

class CStatsManager : public CSingleton<CStatsManager>
{
public:
	void OnNewFrame(CPS2VM*);
	void OnGsNewFrame(uint32);

	static float ComputeCpuUsageRatio(int32 idleTicks, int32 totalTicks);

	uint32 GetFrames();
	uint32 GetDrawCalls();
	CPS2VM::CPU_UTILISATION_INFO GetCpuUtilisationInfo();
#ifdef PROFILE
	std::string GetProfilingInfo();
#endif

	void ClearStats();

private:
	std::mutex m_statsMutex;

	uint32 m_frames = 0;
	uint32 m_drawCalls = 0;

	CPS2VM::CPU_UTILISATION_INFO m_cpuUtilisation;

#ifdef PROFILE
	struct ZONEINFO
	{
		uint64 currentValue = 0;
		uint64 minValue = ~0ULL;
		uint64 maxValue = 0;
	};

	typedef std::map<std::string, ZONEINFO> ZoneMap;

	std::mutex m_profilerZonesMutex;
	ZoneMap m_profilerZones;
#endif
};
