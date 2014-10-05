#include "Profiler.h"

#include <chrono>
#include <cassert>

CProfiler::CProfiler()
{

}

CProfiler::~CProfiler()
{

}

CProfiler::ZoneHandle CProfiler::RegisterZone(const char* name)
{
#ifdef PROFILE
	for(unsigned int i = 0; i < m_zones.size(); i++)
	{
		const auto& zone(m_zones[i]);
		if(zone.name == name) return i;
	}
	auto newZone = ZONE();
	newZone.name = name;
	newZone.totalTime = 0;
	m_zones.push_back(newZone);
	return static_cast<CProfiler::ZoneHandle>(m_zones.size() - 1);
#else
	return 0;
#endif
}

void CProfiler::EnterZone(ZoneHandle zoneHandle)
{
	assert(std::this_thread::get_id() == m_workThreadId);
	
	auto thisTime = std::chrono::high_resolution_clock::now();

	if(!m_zoneStack.empty())
	{
		auto topZoneHandle = m_zoneStack.top();
		auto duration = std::chrono::duration_cast<std::chrono::microseconds>(thisTime - m_currentTime);
		AddTimeToZone(topZoneHandle, duration.count());
	}

	m_zoneStack.push(zoneHandle);

	m_currentTime = thisTime;
}

void CProfiler::ExitZone()
{
	assert(std::this_thread::get_id() == m_workThreadId);
	assert(!m_zoneStack.empty());

	auto thisTime = std::chrono::high_resolution_clock::now();

	{
		auto topZoneHandle = m_zoneStack.top();
		auto duration = std::chrono::duration_cast<std::chrono::microseconds>(thisTime - m_currentTime);
		AddTimeToZone(topZoneHandle, duration.count());
	}

	m_zoneStack.pop();

	m_currentTime = thisTime;
}

CProfiler::ZoneArray CProfiler::GetStats() const
{
	assert(std::this_thread::get_id() == m_workThreadId);
	return m_zones;
}

void CProfiler::Reset()
{
	assert(std::this_thread::get_id() == m_workThreadId);
	for(auto& zone : m_zones)
	{
		zone.totalTime = 0;
	}
}

void CProfiler::SetWorkThread()
{
#ifdef _DEBUG
	m_workThreadId = std::this_thread::get_id();
#endif
}

void CProfiler::AddTimeToZone(ZoneHandle zoneHandle, uint64 timeUs)
{
	assert(m_zones.size() > zoneHandle);
	auto& zone = m_zones[zoneHandle];
	zone.totalTime += timeUs;
}

//////////////////////////////////////////////////////////////////////////
//CProfilerZone

CProfilerZone::CProfilerZone(CProfiler::ZoneHandle handle)
{
#ifdef PROFILE
	CProfiler::GetInstance().EnterZone(handle);
#endif
}

CProfilerZone::~CProfilerZone()
{
#ifdef PROFILE
	CProfiler::GetInstance().ExitZone();
#endif
}
