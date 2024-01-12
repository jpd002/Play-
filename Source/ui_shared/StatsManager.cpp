
#include "StatsManager.h"
#include "string_format.h"
#include "PS2VM.h"

void CStatsManager::OnNewFrame(CPS2VM* virtualMachine)
{
	{
		std::lock_guard<std::mutex> statsLock(m_statsMutex);
		auto cpuUtilisation = virtualMachine->GetCpuUtilisationInfo();
		m_cpuUtilisation.eeTotalTicks += cpuUtilisation.eeTotalTicks;
		m_cpuUtilisation.eeIdleTicks += cpuUtilisation.eeIdleTicks;
		m_cpuUtilisation.iopTotalTicks += cpuUtilisation.iopTotalTicks;
		m_cpuUtilisation.iopIdleTicks += cpuUtilisation.iopIdleTicks;
	}

#ifdef PROFILE
	std::lock_guard<std::mutex> profileZonesLock(m_profilerZonesMutex);

	auto zones = CProfiler::GetInstance().GetStats();
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
#endif
}

void CStatsManager::OnGsNewFrame(uint32 drawCalls)
{
	std::lock_guard<std::mutex> statsLock(m_statsMutex);
	m_frames++;
	m_drawCalls += drawCalls;
}

float CStatsManager::ComputeCpuUsageRatio(int32 idleTicks, int32 totalTicks)
{
	idleTicks = std::max<int32>(idleTicks, 0);
	float idleRatio = static_cast<float>(idleTicks) / static_cast<float>(totalTicks);
	if(totalTicks == 0) idleRatio = 1.f;
	return (1.f - idleRatio) * 100.f;
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

CPS2VM::CPU_UTILISATION_INFO CStatsManager::GetCpuUtilisationInfo()
{
	std::lock_guard<std::mutex> statsLock(m_statsMutex);
	return m_cpuUtilisation;
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

	//Times are in nanoseconds (1m ns in a s)
	static const uint64 timeScale = 1000000;

	for(const auto& zonePair : m_profilerZones)
	{
		const auto& zoneInfo = zonePair.second;
		float avgRatioSpent = (totalTime != 0) ? static_cast<double>(zoneInfo.currentValue) / static_cast<double>(totalTime) : 0;
		float avgMsSpent = (m_frames != 0) ? static_cast<double>(zoneInfo.currentValue) / static_cast<double>(m_frames * timeScale) : 0;
		float minMsSpent = (zoneInfo.minValue != ~0ULL) ? static_cast<double>(zoneInfo.minValue) / static_cast<double>(timeScale) : 0;
		float maxMsSpent = static_cast<double>(zoneInfo.maxValue) / static_cast<double>(timeScale);
		result += string_format("%10s %6.2f%% %6.2fms %6.2fms %6.2fms\r\n",
		                        zonePair.first.c_str(), avgRatioSpent * 100.f, avgMsSpent, minMsSpent, maxMsSpent);
	}

	if(!m_profilerZones.empty())
	{
		float totalAvgMsSpent = (m_frames != 0) ? static_cast<double>(totalTime) / static_cast<double>(m_frames * timeScale) : 0;
		result += string_format("                   %6.2fms\r\n\r\n", totalAvgMsSpent);
	}

	{
		float eeUsageRatio = ComputeCpuUsageRatio(m_cpuUtilisation.eeIdleTicks, m_cpuUtilisation.eeTotalTicks);
		float iopUsageRatio = ComputeCpuUsageRatio(m_cpuUtilisation.iopIdleTicks, m_cpuUtilisation.iopTotalTicks);

		result += string_format("EE Usage:  %6.2f%%\r\n", eeUsageRatio);
		result += string_format("IOP Usage: %6.2f%%\r\n", iopUsageRatio);
	}

	return result;
}

#endif

void CStatsManager::ClearStats()
{
	std::lock_guard<std::mutex> statsLock(m_statsMutex);
	m_frames = 0;
	m_drawCalls = 0;
	m_cpuUtilisation = CPS2VM::CPU_UTILISATION_INFO();
#ifdef PROFILE
	for(auto& zonePair : m_profilerZones)
	{
		zonePair.second.currentValue = 0;
	}
#endif
}
