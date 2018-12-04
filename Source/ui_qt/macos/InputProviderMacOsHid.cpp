#include "InputProviderMacOsHid.h"
#include "string_format.h"

#define PROVIDER_ID 'mHID'

static int GetIntProperty(IOHIDDeviceRef device, CFStringRef key)
{
	int value = 0;
	CFNumberRef ref = static_cast<CFNumberRef>(IOHIDDeviceGetProperty(device, key));
	CFNumberGetValue(ref, kCFNumberSInt32Type, &value);
	return value;
}

static DeviceIdType GetDeviceID(IOHIDDeviceRef dev)
{
	DeviceIdType device{0};
	int vendor = GetIntProperty(dev, CFSTR(kIOHIDVendorIDKey));
	int product = GetIntProperty(dev, CFSTR(kIOHIDProductIDKey));
	int location = GetIntProperty(dev, CFSTR(kIOHIDLocationIDKey));
	device.at(0) = vendor & 0xFF;
	device.at(1) = (vendor >> 8) & 0xFF;
	device.at(2) = product & 0xFF;
	device.at(3) = (product >> 8) & 0xFF;
	device.at(4) = (location >> 16) & 0xFF;
	device.at(5) = (location >> 24) & 0xFF;
	return device;
}

static BINDINGTARGET::KEYTYPE GetKeyType(uint32 usage, IOHIDElementType type)
{
	bool is_axis = (type == kIOHIDElementTypeInput_Axis) || (type == kIOHIDElementTypeInput_Misc);
	auto keyType = BINDINGTARGET::KEYTYPE::BUTTON;
	if(is_axis)
	{
		keyType = BINDINGTARGET::KEYTYPE::AXIS;
		if(usage == kHIDUsage_GD_Hatswitch)
		{
			keyType = BINDINGTARGET::KEYTYPE::POVHAT;
		}
	}
	return keyType;
}

CInputProviderMacOsHid::CInputProviderMacOsHid()
    : m_running(true)
{
	m_thread = std::thread([this]() { InputDeviceListenerThread(); });
}

CInputProviderMacOsHid::~CInputProviderMacOsHid()
{
	m_running = false;
	if(m_thread.joinable())
	{
		m_thread.join();
	}
}

uint32 CInputProviderMacOsHid::GetId() const
{
	return PROVIDER_ID;
}

std::string CInputProviderMacOsHid::GetTargetDescription(const BINDINGTARGET& target) const
{
	return string_format("HID: btn-%d", target.keyId);
}

CFMutableDictionaryRef CInputProviderMacOsHid::CreateDeviceMatchingDictionary(uint32 usagePage, uint32 usage)
{
	CFMutableDictionaryRef result = CFDictionaryCreateMutable(kCFAllocatorDefault, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
	if(result == nullptr) throw std::runtime_error("CFDictionaryCreateMutable failed.");
	if(usagePage != 0)
	{
		// Add key for device type to refine the matching dictionary.
		CFNumberRef pageCFNumberRef = CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &usagePage);
		if(pageCFNumberRef == nullptr) throw std::runtime_error("CFNumberCreate failed.");
		CFDictionarySetValue(result, CFSTR(kIOHIDDeviceUsagePageKey), pageCFNumberRef);
		CFRelease(pageCFNumberRef);
		// note: the usage is only valid if the usage page is also defined
		if(usage != 0)
		{
			CFNumberRef usageCFNumberRef = CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &usage);
			if(usageCFNumberRef == nullptr) throw std::runtime_error("CFNumberCreate failed.");
			CFDictionarySetValue(result, CFSTR(kIOHIDDeviceUsageKey), usageCFNumberRef);
			CFRelease(usageCFNumberRef);
		}
	}
	return result;
}

void CInputProviderMacOsHid::OnDeviceMatchedStub(void* context, IOReturn result, void* sender, IOHIDDeviceRef device)
{
	reinterpret_cast<CInputProviderMacOsHid*>(context)->OnDeviceMatched(result, sender, device);
}

void CInputProviderMacOsHid::InputValueCallbackStub(void* context, IOReturn result, void* sender, IOHIDValueRef value)
{
	auto deviceInfo = reinterpret_cast<DEVICE_INFO*>(context);
	deviceInfo->provider->InputValueCallback(deviceInfo, result, sender, value);
}

void CInputProviderMacOsHid::InputReportCallbackStub_DS3(void* context, IOReturn result, void* sender, IOHIDReportType type, uint32_t reportID, uint8_t* report, CFIndex reportLength)
{
	auto deviceInfo = reinterpret_cast<DEVICE_INFO*>(context);
	deviceInfo->provider->InputReportCallback_DS3(deviceInfo, result, sender, type, reportID, report, reportLength);
}

void CInputProviderMacOsHid::InputReportCallbackStub_DS4(void* context, IOReturn result, void* sender, IOHIDReportType type, uint32_t reportID, uint8_t* report, CFIndex reportLength)
{
	auto deviceInfo = reinterpret_cast<DEVICE_INFO*>(context);
	deviceInfo->provider->InputReportCallback_DS4(deviceInfo, result, sender, type, reportID, report, reportLength);
}

void CInputProviderMacOsHid::OnDeviceMatched(IOReturn result, void* sender, IOHIDDeviceRef device)
{
	m_devices.push_back(DEVICE_INFO());
	auto& deviceInfo = *m_devices.rbegin();
	deviceInfo.provider = this;
	deviceInfo.device = device;
	deviceInfo.deviceId = GetDeviceID(device);

	auto InputReportCallbackStub = GetCallback(device);
	if(InputReportCallbackStub)
	{
		uint32_t max_input_report_size = GetIntProperty(device, CFSTR(kIOHIDMaxInputReportSizeKey));
		uint8_t* report_buffer = static_cast<uint8_t*>(calloc(max_input_report_size, sizeof(uint8_t)));
		IOHIDDeviceRegisterInputReportCallback(device, report_buffer, max_input_report_size, InputReportCallbackStub, &deviceInfo);
	}
	else
	{
		if(OnInput)
		{
			SetInitialBindValues(device);
		}
		IOHIDDeviceRegisterInputValueCallback(device, &InputValueCallbackStub, &deviceInfo);
	}
}

void CInputProviderMacOsHid::InputValueCallback(DEVICE_INFO* deviceInfo, IOReturn result, void* sender, IOHIDValueRef valueRef)
{
	if(!OnInput) return;

	IOHIDElementRef elementRef = IOHIDValueGetElement(valueRef);
	uint32 usagePage = IOHIDElementGetUsagePage(elementRef);
	if(
	    (usagePage != kHIDPage_GenericDesktop) &&
	    (usagePage != kHIDPage_Button))
	{
		return;
	}
	uint32 usage = IOHIDElementGetUsage(elementRef);
	CFIndex value = IOHIDValueGetIntegerValue(valueRef);
	IOHIDElementType type = IOHIDElementGetType(elementRef);
	BINDINGTARGET tgt;
	tgt.providerId = PROVIDER_ID;
	tgt.deviceId = deviceInfo->deviceId;
	tgt.keyId = usage;
	tgt.keyType = GetKeyType(usage, type);
	OnInput(tgt, value);
}

void CInputProviderMacOsHid::InputReportCallback_DS3(DEVICE_INFO* deviceInfo, IOReturn result, void* sender, IOHIDReportType type, uint32_t reportID, uint8_t* report, CFIndex reportLength)
{
	if(report[0] != 0x1 || report[1] == 0xff)
		return;

	struct PS3Btn* new_btn_state = reinterpret_cast<struct PS3Btn*>(&report[2]);
	struct PS3Btn* prev_btn_state = reinterpret_cast<struct PS3Btn*>(deviceInfo->prev_btn_state);
	int is_change = 0;

#define checkbtnstate(prev_btn_state, new_btn_state, btn, btn_id, type)      \
	if(deviceInfo->first_run || (prev_btn_state->btn != new_btn_state->btn)) \
	{                                                                        \
		is_change += 1;                                                      \
		BINDINGTARGET tgt;                                                   \
		tgt.providerId = PROVIDER_ID;                                        \
		tgt.deviceId = deviceInfo->deviceId;                                 \
		tgt.keyId = btn_id;                                                  \
		tgt.keyType = type;                                                  \
		OnInput(tgt, new_btn_state->btn);                                    \
	}

	if(OnInput)
	{
		deviceInfo->first_run = false;

		checkbtnstate(prev_btn_state, new_btn_state, Select, 1, BINDINGTARGET::KEYTYPE::BUTTON);
		checkbtnstate(prev_btn_state, new_btn_state, L3, 2, BINDINGTARGET::KEYTYPE::BUTTON);
		checkbtnstate(prev_btn_state, new_btn_state, R3, 3, BINDINGTARGET::KEYTYPE::BUTTON);
		checkbtnstate(prev_btn_state, new_btn_state, Start, 4, BINDINGTARGET::KEYTYPE::BUTTON);

		checkbtnstate(prev_btn_state, new_btn_state, DPadU, 5, BINDINGTARGET::KEYTYPE::BUTTON);
		checkbtnstate(prev_btn_state, new_btn_state, DPadR, 6, BINDINGTARGET::KEYTYPE::BUTTON);
		checkbtnstate(prev_btn_state, new_btn_state, DPadD, 7, BINDINGTARGET::KEYTYPE::BUTTON);
		checkbtnstate(prev_btn_state, new_btn_state, DPadL, 8, BINDINGTARGET::KEYTYPE::BUTTON);

		checkbtnstate(prev_btn_state, new_btn_state, R2, 9, BINDINGTARGET::KEYTYPE::BUTTON);
		checkbtnstate(prev_btn_state, new_btn_state, L2, 10, BINDINGTARGET::KEYTYPE::BUTTON);
		checkbtnstate(prev_btn_state, new_btn_state, R1, 11, BINDINGTARGET::KEYTYPE::BUTTON);
		checkbtnstate(prev_btn_state, new_btn_state, L1, 12, BINDINGTARGET::KEYTYPE::BUTTON);

		checkbtnstate(prev_btn_state, new_btn_state, PSHome, 13, BINDINGTARGET::KEYTYPE::BUTTON);

		checkbtnstate(prev_btn_state, new_btn_state, LX, 14, BINDINGTARGET::KEYTYPE::AXIS);
		checkbtnstate(prev_btn_state, new_btn_state, LY, 15, BINDINGTARGET::KEYTYPE::AXIS);
		checkbtnstate(prev_btn_state, new_btn_state, RX, 16, BINDINGTARGET::KEYTYPE::AXIS);
		checkbtnstate(prev_btn_state, new_btn_state, RY, 17, BINDINGTARGET::KEYTYPE::AXIS);

		//checkbtnstate(prev_btn_state, new_btn_state, L2T, 18, 3);
		//checkbtnstate(prev_btn_state, new_btn_state, R2T, 19, 3);
		//checkbtnstate(prev_btn_state, new_btn_state, L1T, 20, 3);
		//checkbtnstate(prev_btn_state, new_btn_state, R1T, 21, 3);

		checkbtnstate(prev_btn_state, new_btn_state, Triangle, 22, BINDINGTARGET::KEYTYPE::BUTTON);
		checkbtnstate(prev_btn_state, new_btn_state, Circle, 23, BINDINGTARGET::KEYTYPE::BUTTON);
		checkbtnstate(prev_btn_state, new_btn_state, Cross, 24, BINDINGTARGET::KEYTYPE::BUTTON);
		checkbtnstate(prev_btn_state, new_btn_state, Square, 25, BINDINGTARGET::KEYTYPE::BUTTON);
	}
#undef checkbtnstate

	if(is_change > 0)
	{
		memcpy(deviceInfo->prev_btn_state, new_btn_state, sizeof(struct PS3Btn));
	}
}

void CInputProviderMacOsHid::InputReportCallback_DS4(DEVICE_INFO* deviceInfo, IOReturn result, void* sender, IOHIDReportType type, uint32_t reportID, uint8_t* report, CFIndex reportLength)
{
	int offset = report[0] == 1 ? 1 : 3;

	struct PS4Btn* new_btn_state = reinterpret_cast<struct PS4Btn*>(&report[offset]);
	struct PS4Btn* prev_btn_state = reinterpret_cast<struct PS4Btn*>(deviceInfo->prev_btn_state);
	int is_change = 0;

#define checkbtnstate(prev_btn_state, new_btn_state, btn, btn_id, type)      \
	if(deviceInfo->first_run || (prev_btn_state->btn != new_btn_state->btn)) \
	{                                                                        \
		is_change += 1;                                                      \
		BINDINGTARGET tgt;                                                   \
		tgt.providerId = PROVIDER_ID;                                        \
		tgt.deviceId = deviceInfo->deviceId;                                 \
		tgt.keyId = btn_id;                                                  \
		tgt.keyType = type;                                                  \
		OnInput(tgt, new_btn_state->btn);                                    \
	}

	if(OnInput)
	{
		deviceInfo->first_run = false;

		checkbtnstate(prev_btn_state, new_btn_state, LX, 1, BINDINGTARGET::KEYTYPE::AXIS);
		checkbtnstate(prev_btn_state, new_btn_state, LY, 2, BINDINGTARGET::KEYTYPE::AXIS);
		checkbtnstate(prev_btn_state, new_btn_state, RX, 3, BINDINGTARGET::KEYTYPE::AXIS);
		checkbtnstate(prev_btn_state, new_btn_state, RY, 4, BINDINGTARGET::KEYTYPE::AXIS);

		checkbtnstate(prev_btn_state, new_btn_state, DPad, 5, BINDINGTARGET::KEYTYPE::POVHAT);
		checkbtnstate(prev_btn_state, new_btn_state, Triangle, 6, BINDINGTARGET::KEYTYPE::BUTTON);
		checkbtnstate(prev_btn_state, new_btn_state, Circle, 7, BINDINGTARGET::KEYTYPE::BUTTON);
		checkbtnstate(prev_btn_state, new_btn_state, Cross, 8, BINDINGTARGET::KEYTYPE::BUTTON);
		checkbtnstate(prev_btn_state, new_btn_state, Square, 9, BINDINGTARGET::KEYTYPE::BUTTON);

		checkbtnstate(prev_btn_state, new_btn_state, L1, 10, BINDINGTARGET::KEYTYPE::BUTTON);
		checkbtnstate(prev_btn_state, new_btn_state, R1, 11, BINDINGTARGET::KEYTYPE::BUTTON);
		//checkbtnstate(prev_btn_state, new_btn_state, L2, 12, 1);
		//checkbtnstate(prev_btn_state, new_btn_state, R2, 13, 1);

		checkbtnstate(prev_btn_state, new_btn_state, Share, 14, BINDINGTARGET::KEYTYPE::BUTTON);
		checkbtnstate(prev_btn_state, new_btn_state, Option, 15, BINDINGTARGET::KEYTYPE::BUTTON);
		checkbtnstate(prev_btn_state, new_btn_state, L3, 16, BINDINGTARGET::KEYTYPE::BUTTON);
		checkbtnstate(prev_btn_state, new_btn_state, R3, 17, BINDINGTARGET::KEYTYPE::BUTTON);

		checkbtnstate(prev_btn_state, new_btn_state, PSHome, 18, BINDINGTARGET::KEYTYPE::BUTTON);
		checkbtnstate(prev_btn_state, new_btn_state, TouchPad, 19, BINDINGTARGET::KEYTYPE::BUTTON);
		checkbtnstate(prev_btn_state, new_btn_state, LT, 20, BINDINGTARGET::KEYTYPE::AXIS);
		checkbtnstate(prev_btn_state, new_btn_state, RT, 21, BINDINGTARGET::KEYTYPE::AXIS);
	}
#undef checkbtnstate

	if(is_change > 0)
	{
		memcpy(deviceInfo->prev_btn_state, new_btn_state, sizeof(struct PS4Btn));
	}
}

IOHIDReportCallback CInputProviderMacOsHid::GetCallback(IOHIDDeviceRef device)
{
	auto vid = GetIntProperty(device, CFSTR(kIOHIDVendorIDKey));
	auto pid = GetIntProperty(device, CFSTR(kIOHIDProductIDKey));
	if(vid == 0x54C)
	{
		if(pid == 0x9CC || pid == 0x5C4)
		{
			return &InputReportCallbackStub_DS4;
		}
		else if(pid == 0x268)
		{
			return &InputReportCallbackStub_DS3;
		}
	}
	return nullptr;
}

void CInputProviderMacOsHid::SetInitialBindValues(IOHIDDeviceRef device)
{
	CFArrayRef elements = IOHIDDeviceCopyMatchingElements(device, nullptr, 0);

	for(int i = 0; i < CFArrayGetCount(elements); i++)
	{
		IOHIDElementRef elementRef = (IOHIDElementRef)CFArrayGetValueAtIndex(elements, i);
		uint32 usagePage = IOHIDElementGetUsagePage(elementRef);
		if(
		    (usagePage != kHIDPage_GenericDesktop) &&
		    (usagePage != kHIDPage_Button))
		{
			continue;
		}
		IOHIDValueRef valueRef;
		if(IOHIDDeviceGetValue(device, elementRef, &valueRef) != kIOReturnSuccess)
		{
			continue;
		}

		CFIndex value = IOHIDValueGetIntegerValue(valueRef);
		IOHIDElementType type = IOHIDElementGetType(elementRef);
		uint32 usage = IOHIDElementGetUsage(elementRef);
		BINDINGTARGET tgt;
		tgt.providerId = PROVIDER_ID;
		tgt.deviceId = GetDeviceID(device);
		tgt.keyId = usage;
		tgt.keyType = GetKeyType(usage, type);
		switch(type)
		{
		case kIOHIDElementTypeInput_Misc:
		case kIOHIDElementTypeInput_Button:
		case kIOHIDElementTypeInput_Axis:
			OnInput(tgt, value);
			break;
		default:
			break;
		}
	}
}

void CInputProviderMacOsHid::InputDeviceListenerThread()
{
	m_hidManager = IOHIDManagerCreate(kCFAllocatorDefault, 0);
	{
		CFDictionaryRef matchingDict[3];
		matchingDict[0] = CreateDeviceMatchingDictionary(kHIDPage_GenericDesktop, kHIDUsage_GD_Joystick);
		matchingDict[1] = CreateDeviceMatchingDictionary(kHIDPage_GenericDesktop, kHIDUsage_GD_GamePad);
		matchingDict[2] = CreateDeviceMatchingDictionary(kHIDPage_GenericDesktop, kHIDUsage_GD_MultiAxisController);

		CFArrayRef array = CFArrayCreate(kCFAllocatorDefault, (const void**)matchingDict, 3, &kCFTypeArrayCallBacks);
		CFRelease(matchingDict[0]);
		CFRelease(matchingDict[1]);
		CFRelease(matchingDict[2]);
		IOHIDManagerSetDeviceMatchingMultiple(m_hidManager, array);
	}

	IOHIDManagerRegisterDeviceMatchingCallback(m_hidManager, OnDeviceMatchedStub, this);

	IOHIDManagerOpen(m_hidManager, kIOHIDOptionsTypeNone);
	IOHIDManagerScheduleWithRunLoop(m_hidManager, CFRunLoopGetCurrent(), CFSTR("CustomLoop"));
	while(CFRunLoopRunInMode(CFSTR("CustomLoop"), 1, true) != kCFRunLoopRunStopped && m_running)
	{
	}

	IOHIDManagerClose(m_hidManager, 0);
}
