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
	struct PS3Btn
	{
		uint8_t Select:1;
		uint8_t L3:1;
		uint8_t R3:1;
		uint8_t Start:1;

		uint8_t DPad:4;

		uint8_t R2:1;
		uint8_t L2:1;
		uint8_t R1:1;
		uint8_t L1:1;

		uint8_t PSHome;
		uint8_t unk1;
		uint8_t LX;
		uint8_t LY;
		uint8_t RX;
		uint8_t RY;
		uint64_t unk2;
		uint8_t L2T;
		uint8_t R2T;
		uint8_t L1T;
		uint8_t R1T;
		uint8_t Triangle;
		uint8_t Circle;
		uint8_t Cross;
		uint8_t Square;
	} __attribute__((packed));

	struct PS4Btn
	{
		uint8_t LX;
		uint8_t LY;

		uint8_t RX;
		uint8_t RY;

		uint8_t DPad:4;
		uint8_t Square:1;
		uint8_t Cross:1;
		uint8_t Circle:1;
		uint8_t Triangle:1;

		uint8_t L1:1;
		uint8_t R1:1;
		uint8_t L2:1;
		uint8_t R2:1;
		uint8_t Share:1;
		uint8_t Option:1;
		uint8_t L3:1;
		uint8_t R3:1;

		uint8_t PSHome:1;
		uint8_t TouchPad:1;
		uint8_t Counter:6;

		uint8_t LT:8;
		uint8_t RT:8;

	} __attribute__((packed));

	struct DeviceInfo
	{
		std::array<uint32, 6> device_id;
		OnInputEvent* OnInputEventCallBack;
		bool first_run;
		bool* m_filter;
		uint8_t prev_btn_state[24];
	};
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
	void static InputReportCallbackStub_DS3(void* context, IOReturn result, void* sender, IOHIDReportType type, uint32_t reportID, uint8_t* report, CFIndex reportLength);
	void static InputReportCallbackStub_DS4(void* context, IOReturn result, void* sender, IOHIDReportType type, uint32_t reportID, uint8_t* report, CFIndex reportLength);
	IOHIDReportCallback GetCallback(CGamePadDeviceListener* GPDL, IOHIDDeviceRef device);
	void static SetInitialBindValues(CGamePadDeviceListener* context, IOHIDDeviceRef device);
};
