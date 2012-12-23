#include "Profiler.h"

#ifdef PROFILE

CProfiler::CProfiler()
{

}

CProfiler::~CProfiler()
{

}

void CProfiler::BeginIteration()
{
	Reset();
	m_currentTime = clock();
}

void CProfiler::EndIteration()
{
	std::lock_guard<std::mutex> mutexLock(m_mutex);
}

void CProfiler::BeginZone(const char* name)
{
	assert(std::this_thread::get_id() == m_workThreadId);
	
	clock_t nTime = clock();

	{
		std::lock_guard<std::mutex> mutexLock(m_mutex);
		AddTimeToCurrentZone(nTime - m_currentTime);
		m_zoneStack.push(name);
	}

	m_currentTime = clock();
}

void CProfiler::EndZone()
{
	assert(std::this_thread::get_id() == m_workThreadId);
	
	clock_t nTime = clock();

	{
		std::lock_guard<std::mutex> mutexLock(m_mutex);

		if(m_zoneStack.size() == 0)
		{
			throw std::runtime_error("Currently not inside any zone.");
		}

		AddTimeToCurrentZone(nTime - m_currentTime);
		m_zoneStack.pop();
	}

	m_currentTime = clock();
}

CProfiler::ZoneMap CProfiler::GetStats()
{
	std::lock_guard<std::mutex> mutexLock(m_mutex);
	return m_zones;
}

void CProfiler::Reset()
{
	std::lock_guard<std::mutex> mutexLock(m_mutex);
	std::for_each(std::begin(m_zones), std::end(m_zones), [] (ZoneMap::value_type& zonePair) { zonePair.second = 0; });
}

void CProfiler::SetWorkThread()
{
#ifdef _DEBUG
	m_workThreadId = std::this_thread::get_id();
#endif
}

void CProfiler::AddTimeToCurrentZone(clock_t time)
{
	std::string currentZoneName = (m_zoneStack.size() == 0) ? PROFILE_OTHERZONE : m_zoneStack.top();
	auto zoneIterator = m_zones.find(currentZoneName);
	if(zoneIterator == std::end(m_zones))
	{
		m_zones.insert(ZoneMap::value_type(currentZoneName, 0));
		zoneIterator = m_zones.find(currentZoneName);
	}
	zoneIterator->second += time;
}

//////////////////////////////////////////////////////////////////////////
//CProfilerZone

CProfilerZone::CProfilerZone(const char* name)
: m_name(name)
{
	CProfiler::GetInstance().BeginZone(name);
}

CProfilerZone::~CProfilerZone()
{
	CProfiler::GetInstance().EndZone();
}

#endif
