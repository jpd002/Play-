#pragma once

#include <atomic>
#include <boost/signals2.hpp>
#include <thread>
#include <libevdev.h>
#include "Types.h"

class CGamePadInputEventListener
{
public:
	CGamePadInputEventListener(std::string, bool f = false);
	~CGamePadInputEventListener();

	void					ChangeInputDevice(char* device);
	void					RePopulateAbs();

	boost::signals2::signal<void (std::array<uint32, 6>,int, int, int, const input_absinfo*)> OnInputEvent;
private:
	std::string					m_device;
	std::atomic<bool>				m_running;
	bool						m_filter;
	std::thread					m_thread;
	std::array<struct input_absinfo, ABS_MAX>	m_abslist;

	void						InputDeviceListenerThread();
	void						PopulateAbsInfoList(libevdev *dev);
};
