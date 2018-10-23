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
	typedef std::function<void(std::array<uint32, 6>, int, int, IOHIDElementRef)> OnInputEvent;

	CGamePadDeviceListener(bool f = false);
	CGamePadDeviceListener(OnInputEvent, bool f = false);

	~CGamePadDeviceListener();

	OnInputEvent OnInputEventCallBack;

	void UpdateOnInputEventCallback(OnInputEvent);
	void DisconnectInputEventCallback();

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
