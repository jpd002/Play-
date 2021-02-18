#include "FrameLimiter.h"

#ifdef _WIN32
#include <Windows.h>
#endif

CFrameLimiter::CFrameLimiter()
{
#ifdef _WIN32
	timeBeginPeriod(1);
#endif
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
	auto currentFrameTime = std::chrono::high_resolution_clock::now();
	auto frameDuration = std::chrono::duration_cast<std::chrono::microseconds>(currentFrameTime - m_lastFrameTime);
	if(frameDuration < m_minFrameDuration)
	{
		auto delay = m_minFrameDuration - frameDuration;
#ifdef _WIN32
		{
			LARGE_INTEGER ft = {};
			ft.QuadPart = -static_cast<int64>(delay.count() * 10);

			HANDLE timer = CreateWaitableTimer(NULL, TRUE, NULL);
			SetWaitableTimer(timer, &ft, 0, NULL, NULL, 0);
			WaitForSingleObject(timer, INFINITE);
			CloseHandle(timer);
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
