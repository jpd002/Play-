#include "FrameLimiter.h"
#include <cassert>
#include <thread>

#ifdef _WIN32
#include <Windows.h>
#endif

CFrameLimiter::CFrameLimiter()
{
#ifdef _WIN32
	timeBeginPeriod(1);
#endif
	for(uint32 i = 0; i < MAX_FRAMETIMES; i++)
	{
		m_frameTimes[i] = std::chrono::microseconds(0);
	}
}

CFrameLimiter::~CFrameLimiter()
{
#ifdef _WIN32
	timeEndPeriod(1);
#endif
}

void CFrameLimiter::BeginFrame()
{
	assert(!m_frameStarted);
	m_lastFrameTime = std::chrono::high_resolution_clock::now();
	m_frameStarted = true;
}

void CFrameLimiter::EndFrame()
{
	assert(m_frameStarted);

	//Add current frame time to array
	{
		auto currentFrameTime = std::chrono::high_resolution_clock::now();
		auto frameDuration = std::chrono::duration_cast<std::chrono::microseconds>(currentFrameTime - m_lastFrameTime);
		m_frameTimes[m_frameTimeIndex++] = frameDuration;
		m_frameTimeIndex %= MAX_FRAMETIMES;
	}

	//Compute average frame time
	std::chrono::microseconds averageFrameTime = std::chrono::microseconds(0);
	for(uint32 i = 0; i < MAX_FRAMETIMES; i++)
	{
		averageFrameTime += m_frameTimes[i];
	}
	averageFrameTime /= MAX_FRAMETIMES;

	if(averageFrameTime < m_minFrameDuration)
	{
		auto delay = m_minFrameDuration - averageFrameTime;
#ifdef _WIN32
		{
			LARGE_INTEGER ft = {};
			ft.QuadPart = -static_cast<int64>(delay.count() * 10);

			HANDLE timer = CreateWaitableTimer(NULL, TRUE, NULL);
			SetWaitableTimer(timer, &ft, 0, NULL, NULL, 0);
			WaitForSingleObject(timer, INFINITE);
			CloseHandle(timer);
		}
#elif defined(__APPLE__)
		//Sleeping for the whole delay on macOS/iOS doesn't provide a good enough resolution
		auto currentTime = std::chrono::high_resolution_clock::now();
		auto targetTime = currentTime + delay;
		while(currentTime < targetTime)
		{
			std::this_thread::sleep_for(std::chrono::microseconds(250));
			currentTime = std::chrono::high_resolution_clock::now();
		}
#else
		std::this_thread::sleep_for(delay);
#endif
	}
	m_frameStarted = false;
}

void CFrameLimiter::SetFrameRate(uint32 fps)
{
	if(fps == 0)
	{
		m_minFrameDuration = std::chrono::microseconds(0);
	}
	else
	{
		m_minFrameDuration = std::chrono::microseconds(1000000 / fps);
	}
}
