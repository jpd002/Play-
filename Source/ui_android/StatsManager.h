#pragma once

#include <mutex>
#include "Types.h"
#include "Singleton.h"

class CStatsManager : public CSingleton<CStatsManager>
{
public:
	void			OnNewFrame(uint32);

	uint32			GetFrames();
	uint32			GetDrawCalls();
	
	void			ClearStats();
	
private:
	std::mutex		m_statsMutex;
	
	uint32			m_frames = 0;
	uint32			m_drawCalls = 0;
};
