#pragma once

#include <atomic>
#include <thread>
#include <list>
#include <IOKit/hid/IOHIDManager.h>
#include <CoreFoundation/CoreFoundation.h>
#include "input/InputProvider.h"

class CInputProviderMacOsHid : public CInputProvider
{
public:
	CInputProviderMacOsHid();
	virtual ~CInputProviderMacOsHid();

	uint32 GetId() const override;
	std::string GetTargetDescription(const BINDINGTARGET&) const override;

private:
	enum
	{
		BTN_STATE_SIZE = 24,
	};

	struct DEVICE_INFO
	{
		CInputProviderMacOsHid* provider = nullptr;
		DeviceIdType deviceId;
		IOHIDDeviceRef device;
		bool first_run = true;
		uint8_t prev_btn_state[BTN_STATE_SIZE];
	};

	struct PS3Btn
	{
		uint8_t Select : 1;
		uint8_t L3 : 1;
		uint8_t R3 : 1;
		uint8_t Start : 1;

		uint8_t DPadU : 1;
		uint8_t DPadR : 1;
		uint8_t DPadD : 1;
		uint8_t DPadL : 1;

		uint8_t R2 : 1;
		uint8_t L2 : 1;
		uint8_t R1 : 1;
		uint8_t L1 : 1;

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
	static_assert(sizeof(PS3Btn) <= BTN_STATE_SIZE, "PS4Btn too large for BTN_STATE");

	struct PS4Btn
	{
		uint8_t LX;
		uint8_t LY;

		uint8_t RX;
		uint8_t RY;

		uint8_t DPad : 4;
		uint8_t Square : 1;
		uint8_t Cross : 1;
		uint8_t Circle : 1;
		uint8_t Triangle : 1;

		uint8_t L1 : 1;
		uint8_t R1 : 1;
		uint8_t L2 : 1;
		uint8_t R2 : 1;
		uint8_t Share : 1;
		uint8_t Option : 1;
		uint8_t L3 : 1;
		uint8_t R3 : 1;

		uint8_t PSHome : 1;
		uint8_t TouchPad : 1;
		uint8_t Counter : 6;

		uint8_t LT : 8;
		uint8_t RT : 8;

	} __attribute__((packed));
	static_assert(sizeof(PS4Btn) <= BTN_STATE_SIZE, "PS4Btn too large for BTN_STATE");

	void UpdateDeviceList();
	void InputDeviceListenerThread();
	CFMutableDictionaryRef CreateDeviceMatchingDictionary(uint32 usagePage, uint32 usage);

	static void OnDeviceMatchedStub(void* context, IOReturn result, void* sender, IOHIDDeviceRef device);
	static void InputValueCallbackStub(void* context, IOReturn result, void* sender, IOHIDValueRef valueRef);
	static void InputReportCallbackStub_DS3(void* context, IOReturn result, void* sender, IOHIDReportType type, uint32_t reportID, uint8_t* report, CFIndex reportLength);
	static void InputReportCallbackStub_DS4(void* context, IOReturn result, void* sender, IOHIDReportType type, uint32_t reportID, uint8_t* report, CFIndex reportLength);

	void OnDeviceMatched(IOReturn result, void* sender, IOHIDDeviceRef device);
	void InputValueCallback(DEVICE_INFO*, IOReturn result, void* sender, IOHIDValueRef value);
	void InputReportCallback_DS3(DEVICE_INFO*, IOReturn result, void* sender, IOHIDReportType type, uint32_t reportID, uint8_t* report, CFIndex reportLength);
	void InputReportCallback_DS4(DEVICE_INFO*, IOReturn result, void* sender, IOHIDReportType type, uint32_t reportID, uint8_t* report, CFIndex reportLength);

	IOHIDReportCallback GetCallback(IOHIDDeviceRef device);
	void SetInitialBindValues(IOHIDDeviceRef device);

	std::atomic<bool> m_running;
	std::thread m_inputdevicelistenerthread;
	bool m_filter;
	std::thread m_thread;
	IOHIDManagerRef m_hidManager;
	std::list<DEVICE_INFO> m_devices;
};
