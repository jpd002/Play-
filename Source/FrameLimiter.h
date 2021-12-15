#pragma once

#include <chrono>
#include "Types.h"

class CFrameLimiter
{
public:
	CFrameLimiter();
	~CFrameLimiter();

	void BeginFrame();
	void EndFrame();

	void SetFrameRate(uint32);

private:
	typedef std::chrono::high_resolution_clock::time_point TimePoint;

	enum
	{
		MAX_FRAMETIMES = 4,
	};

	std::chrono::microseconds m_frameTimes[MAX_FRAMETIMES];
	uint32 m_frameTimeIndex = 0;

	std::chrono::microseconds m_minFrameDuration = std::chrono::microseconds(0);
	bool m_frameStarted = false;
	TimePoint m_lastFrameTime;
};
