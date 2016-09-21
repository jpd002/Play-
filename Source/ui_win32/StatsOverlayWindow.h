#pragma once

#include <thread>
#include "win32/Window.h"
#include "win32/GdiObj.h"
#include "../Profiler.h"
#include "../PS2VM.h"

class CStatsOverlayWindow : public Framework::Win32::CWindow
{
public:
							CStatsOverlayWindow();
							CStatsOverlayWindow(HWND);
	virtual					~CStatsOverlayWindow();

	CStatsOverlayWindow&	operator =(CStatsOverlayWindow&&);

	void					Update(unsigned int);
	void					ResetStats();

	void					OnProfileFrameDone(CPS2VM&, const CProfiler::ZoneArray&);

private:
	struct RENDERMETRICS
	{
		int marginX = 0;
		int marginY = 0;
		int spaceY = 0;
		int fontSizeX = 0;
		int fontSizeY = 0;
	};

	struct ZONEINFO
	{
		uint64 currentValue = 0;
		uint64 minValue = ~0ULL;
		uint64 maxValue = 0;
	};

	typedef std::map<std::string, ZONEINFO> ZoneMap;

	std::mutex				m_profilerZonesMutex;
	ZoneMap					m_profilerZones;

	CPS2VM::CPU_UTILISATION_INFO m_cpuUtilisation;
	RENDERMETRICS			m_renderMetrics;
	Framework::Win32::CFont	m_font;
};
