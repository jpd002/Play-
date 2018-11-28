#pragma once

#include <atomic>
#include <boost/signals2.hpp>
#include <thread>
#include <libevdev.h>
#include "Types.h"

class CGamePadInputEventListener
{
public:
	CGamePadInputEventListener(std::string);
	virtual ~CGamePadInputEventListener();

	boost::signals2::signal<void(std::array<uint32, 6>, int, int, int, const input_absinfo*)> OnInputEvent;

private:
	std::string m_device;
	std::atomic<bool> m_running;
	std::thread m_thread;

	void InputDeviceListenerThread();
};
