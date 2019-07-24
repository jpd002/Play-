
#include "StatsManager.h"
#include "string_format.h"
#include "PS2VM.h"

void CStatsManager::OnNewFrame(uint32 drawCalls)
{
	std::lock_guard<std::mutex> statsLock(m_statsMutex);
	m_frames++;
	m_drawCalls += drawCalls;
}

uint32 CStatsManager::GetFrames()
{
	std::lock_guard<std::mutex> statsLock(m_statsMutex);
	return m_frames;
}

uint32 CStatsManager::GetDrawCalls()
{
	std::lock_guard<std::mutex> statsLock(m_statsMutex);
	return m_drawCalls;
}

#ifdef PROFILE

std::string CStatsManager::GetProfilingInfo()
{
	std::lock_guard<std::mutex> profileZonesLock(m_profilerZonesMutex);

	std::string result;
	uint64 totalTime = 0;

	for(const auto& zonePair : m_profilerZones)
	{
		const auto& zoneInfo = zonePair.second;
		totalTime += zoneInfo.currentValue;
	}

	for(const auto& zonePair : m_profilerZones)
	{
		const auto& zoneInfo = zonePair.second;
		float avgRatioSpent = (totalTime != 0) ? static_cast<double>(zoneInfo.currentValue) / static_cast<double>(totalTime) : 0;
		float avgMsSpent = (m_frames != 0) ? static_cast<double>(zoneInfo.currentValue) / static_cast<double>(m_frames * 1000) : 0;
		float minMsSpent = (zoneInfo.minValue != ~0ULL) ? static_cast<double>(zoneInfo.minValue) / static_cast<double>(1000) : 0;
		float maxMsSpent = static_cast<double>(zoneInfo.maxValue) / static_cast<double>(1000);

		result += string_format("%10s %6.2f%% %6.2fms %6.2fms %6.2fms\r\n",
		                        zonePair.first.c_str(), avgRatioSpent * 100.f, avgMsSpent, minMsSpent, maxMsSpent);
	}

	if(!m_profilerZones.empty())
	{
		result += "\r\n";
	}
	
	{
		m_cpuUtilisation.eeIdleTicks = std::max<int32>(m_cpuUtilisation.eeIdleTicks, 0);
		m_cpuUtilisation.iopIdleTicks = std::max<int32>(m_cpuUtilisation.iopIdleTicks, 0);
		
		float eeIdleRatio = static_cast<float>(m_cpuUtilisation.eeIdleTicks) / static_cast<float>(m_cpuUtilisation.eeTotalTicks);
		float iopIdleRatio = static_cast<float>(m_cpuUtilisation.iopIdleTicks) / static_cast<float>(m_cpuUtilisation.iopTotalTicks);
		
		if(m_cpuUtilisation.eeTotalTicks == 0) eeIdleRatio = 1.f;
		if(m_cpuUtilisation.iopTotalTicks == 0) iopIdleRatio = 1.f;
		
		result += string_format("EE Usage:  %6.2f%%\r\n", (1.f - eeIdleRatio) * 100.f);
		result += string_format("IOP Usage: %6.2f%%\r\n", (1.f - iopIdleRatio) * 100.f);
	}
	
	return result;
}

#endif

void CStatsManager::ClearStats()
{
	std::lock_guard<std::mutex> statsLock(m_statsMutex);
	m_frames = 0;
	m_drawCalls = 0;
#ifdef PROFILE
	for(auto& zonePair : m_profilerZones)
	{
		zonePair.second.currentValue = 0;
	}
	m_cpuUtilisation = CPS2VM::CPU_UTILISATION_INFO();
#endif
}

#ifdef PROFILE

void CStatsManager::OnProfileFrameDone(CPS2VM* virtualMachine, const CProfiler::ZoneArray& zones)
{
	std::lock_guard<std::mutex> profileZonesLock(m_profilerZonesMutex);

	for(auto& zone : zones)
	{
		auto& zoneInfo = m_profilerZones[zone.name];
		zoneInfo.currentValue += zone.totalTime;
		if(zone.totalTime != 0)
		{
			zoneInfo.minValue = std::min<uint64>(zoneInfo.minValue, zone.totalTime);
		}
		zoneInfo.maxValue = std::max<uint64>(zoneInfo.maxValue, zone.totalTime);
	}
	
	auto cpuUtilisation = virtualMachine->GetCpuUtilisationInfo();
	m_cpuUtilisation.eeTotalTicks += cpuUtilisation.eeTotalTicks;
	m_cpuUtilisation.eeIdleTicks += cpuUtilisation.eeIdleTicks;
	m_cpuUtilisation.iopTotalTicks += cpuUtilisation.iopTotalTicks;
	m_cpuUtilisation.iopIdleTicks += cpuUtilisation.iopIdleTicks;
}

#endif
