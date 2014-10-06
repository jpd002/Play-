#pragma once

#include <thread>
#include "win32/Window.h"
#include "win32/GdiObj.h"
#include "../Profiler.h"

class CStatsOverlayWindow : public Framework::Win32::CWindow
{
public:
							CStatsOverlayWindow();
							CStatsOverlayWindow(HWND);
	virtual					~CStatsOverlayWindow();

	CStatsOverlayWindow&	operator =(CStatsOverlayWindow&&);

	void					OnProfileFrameDone(const CProfiler::ZoneArray&);

	void					Update(unsigned int);

private:
	struct RENDERMETRICS
	{
		int marginX = 0;
		int marginY = 0;
		int spaceY = 0;
		int fontSizeX = 0;
		int fontSizeY = 0;
	};

	typedef std::map<std::string, uint64> ZoneMap;

	std::mutex				m_profilerZonesMutex;
	ZoneMap					m_profilerZones;

	RENDERMETRICS			m_renderMetrics;
	Framework::Win32::CFont	m_font;
};
