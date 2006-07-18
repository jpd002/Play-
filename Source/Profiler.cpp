#include "Profiler.h"

using namespace std;
using namespace boost;

#ifdef PROFILE

CProfiler::CProfiler() :
m_OtherZone("Other")
{

}

CProfiler::~CProfiler()
{

}

void CProfiler::BeginIteration()
{
	m_nCurrentTime = clock();
}

void CProfiler::EndIteration()
{
	mutex::scoped_lock Lock(m_Mutex);

	m_Stats.clear();

	for(ZoneMap::const_iterator itZone = m_Zones.begin();
		itZone != m_Zones.end();
		itZone++)
	{
		m_Stats.push_back((*itZone));
	}

	m_Stats.push_back(m_OtherZone);
}

void CProfiler::BeginZone(const char* sName)
{

	clock_t nTime;
	nTime = clock();

	{
		mutex::scoped_lock Lock(m_Mutex);

		CZone& Zone = GetCurrentZone();
		Zone.AddTime(nTime - m_nCurrentTime);

		ZoneMap::iterator itZone;
		
		itZone = m_Zones.find(sName);
		if(itZone == m_Zones.end())
		{
			string sZoneName(sName);
			m_Zones.insert(sZoneName, new CZone(sName));
			itZone = m_Zones.find(sName);
		}

		m_ZoneStack.push_back(&(*itZone));
	}

	m_nCurrentTime = clock();
}

void CProfiler::EndZone()
{
	clock_t nTime;

	nTime = clock();

	{
		mutex::scoped_lock Lock(m_Mutex);

		if(m_ZoneStack.size() == 0)
		{
			throw exception("Currently not inside any zone.");
		}

		CZone& Zone = GetCurrentZone();
		Zone.AddTime(nTime - m_nCurrentTime);

		m_ZoneStack.pop_back();
	}

	m_nCurrentTime = clock();
}

CProfiler::ZoneList CProfiler::GetStats()
{
	mutex::scoped_lock Lock(m_Mutex);

	return m_Stats;
}

void CProfiler::Reset()
{
	mutex::scoped_lock Lock(m_Mutex);

	for(ZoneMap::iterator itZone = m_Zones.begin();
		itZone != m_Zones.end();
		itZone++)
	{
		(*itZone).Reset();
	}
}

CProfiler::CZone& CProfiler::GetCurrentZone()
{
	if(m_ZoneStack.size() == 0)
	{
		return m_OtherZone;
	}
	else
	{
		return **(m_ZoneStack.rbegin());
	}
}

CProfiler::CZone::CZone(const char* sName)
{
	m_sName = sName;
	m_nTime	= 0;
}

void CProfiler::CZone::AddTime(clock_t nTime)
{
	m_nTime += nTime;
}

const char* CProfiler::CZone::GetName() const
{
	return m_sName.c_str();
}

clock_t CProfiler::CZone::GetTime() const
{
	return m_nTime;
}

void CProfiler::CZone::Reset()
{
	m_nTime = 0;
}

#endif
