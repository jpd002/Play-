#pragma once
#include <atomic>
#include <array>
#include <boost/signals2.hpp>
#include <thread>
#include <IOKit/hid/IOHIDManager.h>
#include <CoreFoundation/CoreFoundation.h>
#include <boost/filesystem.hpp>

#include "Types.h"

namespace fs = boost::filesystem;

class CGamePadDeviceListener
{
public:
	enum btn_type
	{
		//keyboard not reported by iohid, but type is used to distinguish qt keyboard vs iohid
		keyboard = 0,
		digital = 1,
		hatswitch = 2,
		axis = 3,
	};
	typedef std::function<void(std::array<uint32, 6>, int, int, int)> OnInputEvent;

	CGamePadDeviceListener(bool f = false);
	CGamePadDeviceListener(OnInputEvent, bool f = false);

	~CGamePadDeviceListener();

	OnInputEvent OnInputEventCallBack;

	void UpdateOnInputEventCallback(OnInputEvent);
	void DisconnectInputEventCallback();
	void SetFilter(bool);

private:
	std::atomic<bool> m_running;
	std::thread m_inputdevicelistenerthread;
	bool m_filter;
	std::thread m_thread;
	IOHIDManagerRef m_hidManager;

	void UpdateDeviceList();
	void AddDevice(const fs::path&);
	void InputDeviceListenerThread();
	CFMutableDictionaryRef CreateDeviceMatchingDictionary(uint32 usagePage, uint32 usage);
	void static InputValueCallbackStub(void* context, IOReturn result, void* sender, IOHIDValueRef valueRef);
	void static onDeviceMatched(void* context, IOReturn result, void* sender, IOHIDDeviceRef device);
};
