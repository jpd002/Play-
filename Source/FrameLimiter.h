#pragma once

#include <chrono>

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

	std::chrono::microseconds m_minFrameDuration = std::chrono::microseconds(0);
	bool m_frameStarted = false;
	TimePoint m_lastFrameTime;
};
