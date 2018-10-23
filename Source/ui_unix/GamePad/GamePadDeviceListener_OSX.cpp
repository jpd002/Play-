#include "GamePadDeviceListener_OSX.h"
#include "GamePadUtils.h"

CGamePadDeviceListener::CGamePadDeviceListener(OnInputEvent OnInputEventCallBack, bool filter)
    : OnInputEventCallBack(OnInputEventCallBack)
    , m_running(true)
    , m_filter(filter)
{
	m_thread = std::thread([this]() { InputDeviceListenerThread(); });
}

CGamePadDeviceListener::CGamePadDeviceListener(bool filter)
    : CGamePadDeviceListener(nullptr, filter)
{
}

CGamePadDeviceListener::~CGamePadDeviceListener()
{
	m_running = false;
	CFRunLoopStop(CFRunLoopGetCurrent());
	IOHIDManagerClose(m_hidManager, 0);
	if(m_thread.joinable())
		m_thread.join();
}

void CGamePadDeviceListener::UpdateOnInputEventCallback(CGamePadDeviceListener::OnInputEvent OnInputEventFunction)
{
	OnInputEventCallBack = OnInputEventFunction;
}

void CGamePadDeviceListener::DisconnectInputEventCallback()
{
	OnInputEventCallBack = nullptr;
}

void CGamePadDeviceListener::SetFilter(bool filter)
{
	m_filter = filter;
}

void CGamePadDeviceListener::UpdateDeviceList()
{
}

CFMutableDictionaryRef CGamePadDeviceListener::CreateDeviceMatchingDictionary(uint32 usagePage, uint32 usage)
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

void CGamePadDeviceListener::InputValueCallbackStub(void* context, IOReturn result, void* sender, IOHIDValueRef valueRef)
{
	auto device_info = reinterpret_cast<DeviceInfo*>(context);
	if(!(*device_info->OnInputEventCallBack))
	{
		return;
	}

	IOHIDElementRef elementRef = IOHIDValueGetElement(valueRef);
	uint32 usagePage = IOHIDElementGetUsagePage(elementRef);
	if(usagePage == kHIDPage_VendorDefinedStart)
	{
		return;
	}
	uint32 usage = IOHIDElementGetUsage(elementRef);
	CFIndex value = IOHIDValueGetIntegerValue(valueRef);
	IOHIDDeviceRef device = IOHIDElementGetDevice(elementRef);
	IOHIDElementType type = IOHIDElementGetType(elementRef);
	bool is_axis = type == kIOHIDElementTypeInput_Axis || type == kIOHIDElementTypeInput_Misc;
	if(device_info->m_filter)
	{
		if(is_axis)
		{
			if(usage != kHIDUsage_GD_Hatswitch)
			{
				CFIndex maxRange = IOHIDElementGetLogicalMax(elementRef);
				CFIndex minRange = IOHIDElementGetLogicalMin(elementRef);
				CFIndex neutralRange = (maxRange + minRange) / 2;
				CFIndex triggerRange = (neutralRange * 20) / 100;

				if((value < minRange || value > maxRange) || (neutralRange + triggerRange > value && value > neutralRange - triggerRange))
				{
					return;
				}
			}
		}
	}
	int btn_type = btn_type::digital;
	if(is_axis)
	{
		btn_type = btn_type::axis;
		if(usage == kHIDUsage_GD_Hatswitch)
		{
			btn_type = btn_type::hatswitch;
		}
	}
	(*device_info->OnInputEventCallBack)(device_info->device_id, usage, value, btn_type);
}

void CGamePadDeviceListener::InputReportCallbackStub_DS3(void* context, IOReturn result, void* sender, IOHIDReportType type, uint32_t reportID, uint8_t* report, CFIndex reportLength)
{
	if(report[0] != 0x1 || report[1] == 0xff)
		return;

	auto device_info = reinterpret_cast<DeviceInfo*>(context);

	struct PS3Btn* new_btn_state = reinterpret_cast<struct PS3Btn*>(&report[2]);
	struct PS3Btn* prev_btn_state = reinterpret_cast<struct PS3Btn*>(device_info->prev_btn_state);
	int is_change = 0;

	int triggerRange = (255 * 20) / 100;
	int triggerVal1 = 0x7F - triggerRange;
	int triggerVal2 = 0x7F + triggerRange;
#define deadzone(type, value) (type < 2 || (value < triggerVal1 || triggerVal2 < value))
#define checkbtnstate(prev_btn_state, new_btn_state, btn, btn_id, type)                                                                        \
	if(device_info->first_run || (prev_btn_state->btn != new_btn_state->btn && (device_info->m_filter || deadzone(type, new_btn_state->btn)))) \
	{                                                                                                                                          \
		is_change += 1;                                                                                                                        \
		(*device_info->OnInputEventCallBack)(device_info->device_id, btn_id, new_btn_state->btn, type);                                        \
	}

	if(*device_info->OnInputEventCallBack)
	{
		device_info->first_run = false;
		checkbtnstate(prev_btn_state, new_btn_state, Select, 1, 1);
		checkbtnstate(prev_btn_state, new_btn_state, L3, 2, 1);
		checkbtnstate(prev_btn_state, new_btn_state, R3, 3, 1);
		checkbtnstate(prev_btn_state, new_btn_state, Start, 4, 1);

		checkbtnstate(prev_btn_state, new_btn_state, DPadU, 5, 1);
		checkbtnstate(prev_btn_state, new_btn_state, DPadR, 6, 1);
		checkbtnstate(prev_btn_state, new_btn_state, DPadD, 7, 1);
		checkbtnstate(prev_btn_state, new_btn_state, DPadL, 8, 1);

		checkbtnstate(prev_btn_state, new_btn_state, R2, 9, 1);
		checkbtnstate(prev_btn_state, new_btn_state, L2, 10, 1);
		checkbtnstate(prev_btn_state, new_btn_state, R1, 11, 1);
		checkbtnstate(prev_btn_state, new_btn_state, L1, 12, 1);

		checkbtnstate(prev_btn_state, new_btn_state, PSHome, 13, 1);

		checkbtnstate(prev_btn_state, new_btn_state, LX, 14, 3);
		checkbtnstate(prev_btn_state, new_btn_state, LY, 15, 3);
		checkbtnstate(prev_btn_state, new_btn_state, RX, 16, 3);
		checkbtnstate(prev_btn_state, new_btn_state, RY, 17, 3);

		//checkbtnstate(prev_btn_state, new_btn_state, L2T, 18, 3);
		//checkbtnstate(prev_btn_state, new_btn_state, R2T, 19, 3);
		//checkbtnstate(prev_btn_state, new_btn_state, L1T, 20, 3);
		//checkbtnstate(prev_btn_state, new_btn_state, R1T, 21, 3);

		checkbtnstate(prev_btn_state, new_btn_state, Triangle, 22, 1);
		checkbtnstate(prev_btn_state, new_btn_state, Circle, 23, 1);
		checkbtnstate(prev_btn_state, new_btn_state, Cross, 24, 1);
		checkbtnstate(prev_btn_state, new_btn_state, Square, 25, 1);
	}
#undef checkbtnstate
#undef deadzone

	if(is_change > 0)
	{
		memcpy(device_info->prev_btn_state, new_btn_state, sizeof(struct PS3Btn));
	}
}

void CGamePadDeviceListener::InputReportCallbackStub_DS4(void* context, IOReturn result, void* sender, IOHIDReportType type, uint32_t reportID, uint8_t* report, CFIndex reportLength)
{
	auto device_info = reinterpret_cast<DeviceInfo*>(context);
	int offset = report[0] == 1 ? 1 : 3;

	struct PS4Btn* new_btn_state = reinterpret_cast<struct PS4Btn*>(&report[offset]);
	struct PS4Btn* prev_btn_state = reinterpret_cast<struct PS4Btn*>(device_info->prev_btn_state);
	int is_change = 0;

	int triggerRange = (255 * 20) / 100;
	int triggerVal1 = 0x7F - triggerRange;
	int triggerVal2 = 0x7F + triggerRange;

#define deadzone(type, value) (type < 2 || (value < triggerVal1 || triggerVal2 < value))
#define checkbtnstate(prev_btn_state, new_btn_state, btn, btn_id, type)                                                                        \
	if(device_info->first_run || (prev_btn_state->btn != new_btn_state->btn && (device_info->m_filter || deadzone(type, new_btn_state->btn)))) \
	{                                                                                                                                          \
		is_change += 1;                                                                                                                        \
		(*device_info->OnInputEventCallBack)(device_info->device_id, btn_id, new_btn_state->btn, type);                                        \
	}

	if(*device_info->OnInputEventCallBack)
	{
		checkbtnstate(prev_btn_state, new_btn_state, LX, 1, 3);
		checkbtnstate(prev_btn_state, new_btn_state, LY, 2, 3);
		checkbtnstate(prev_btn_state, new_btn_state, RX, 3, 3);
		checkbtnstate(prev_btn_state, new_btn_state, RY, 4, 3);

		checkbtnstate(prev_btn_state, new_btn_state, DPad, 5, 2);
		checkbtnstate(prev_btn_state, new_btn_state, Triangle, 6, 1);
		checkbtnstate(prev_btn_state, new_btn_state, Circle, 7, 1);
		checkbtnstate(prev_btn_state, new_btn_state, Cross, 8, 1);
		checkbtnstate(prev_btn_state, new_btn_state, Square, 9, 1);

		checkbtnstate(prev_btn_state, new_btn_state, L1, 10, 1);
		checkbtnstate(prev_btn_state, new_btn_state, R1, 11, 1);
		//checkbtnstate(prev_btn_state, new_btn_state, L2, 12, 1);
		//checkbtnstate(prev_btn_state, new_btn_state, R2, 13, 1);

		checkbtnstate(prev_btn_state, new_btn_state, Share, 14, 1);
		checkbtnstate(prev_btn_state, new_btn_state, Option, 15, 1);
		checkbtnstate(prev_btn_state, new_btn_state, L3, 16, 1);
		checkbtnstate(prev_btn_state, new_btn_state, R3, 17, 1);

		checkbtnstate(prev_btn_state, new_btn_state, PSHome, 18, 1);
		checkbtnstate(prev_btn_state, new_btn_state, TouchPad, 19, 1);
		checkbtnstate(prev_btn_state, new_btn_state, LT, 20, 3);
		checkbtnstate(prev_btn_state, new_btn_state, RT, 21, 3);
		device_info->first_run = false;
	}
#undef checkbtnstate
#undef deadzone

	if(is_change > 0)
	{
		memcpy(device_info->prev_btn_state, new_btn_state, sizeof(struct PS4Btn));
	}
}

void CGamePadDeviceListener::onDeviceMatched(void* context, IOReturn result, void* sender, IOHIDDeviceRef device)
{
	auto GPDL = reinterpret_cast<CGamePadDeviceListener*>(context);

	DeviceInfo* device_info = static_cast<DeviceInfo*>(malloc(sizeof(struct DeviceInfo)));
	device_info->device_id = CGamePadUtils::GetDeviceID(device);
	device_info->OnInputEventCallBack = &GPDL->OnInputEventCallBack;
	device_info->m_filter = &GPDL->m_filter;
	device_info->first_run = true;
	auto InputReportCallbackStub = GPDL->GetCallback(GPDL, device);
	if(InputReportCallbackStub)
	{
		uint32_t max_input_report_size = CGamePadUtils::GetIntProperty(device, CFSTR(kIOHIDMaxInputReportSizeKey));
		uint8_t* report_buffer = static_cast<uint8_t*>(calloc(max_input_report_size, sizeof(uint8_t)));
		IOHIDDeviceRegisterInputReportCallback(device, report_buffer, max_input_report_size, InputReportCallbackStub, device_info);
	}
	else
	{
		if(GPDL->OnInputEventCallBack)
			GPDL->SetInitialBindValues(GPDL, device);

		IOHIDDeviceRegisterInputValueCallback(device, GPDL->InputValueCallbackStub, device_info);
	}
}
void CGamePadDeviceListener::SetInitialBindValues(CGamePadDeviceListener* GPDL, IOHIDDeviceRef device)
{
	CFArrayRef elements = IOHIDDeviceCopyMatchingElements(device, nullptr, 0);

	for(int i = 0; i < CFArrayGetCount(elements); i++)
	{
		IOHIDElementRef elementRef = (IOHIDElementRef)CFArrayGetValueAtIndex(elements, i);
		uint32 usagePage = IOHIDElementGetUsagePage(elementRef);
		if(usagePage == kHIDPage_VendorDefinedStart)
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
		int btn_type = btn_type::digital;
		bool is_axis = type == kIOHIDElementTypeInput_Axis || type == kIOHIDElementTypeInput_Misc;
		if(is_axis)
		{
			btn_type = btn_type::axis;
			if(usage == kHIDUsage_GD_Hatswitch)
			{
				btn_type = btn_type::hatswitch;
			}
		}
		switch(type)
		{
		case kIOHIDElementTypeInput_Misc:
		case kIOHIDElementTypeInput_Button:
		case kIOHIDElementTypeInput_Axis:
			GPDL->OnInputEventCallBack(CGamePadUtils::GetDeviceID(device), usage, value, btn_type);
			break;
		default:
			break;
		}
	}
}
IOHIDReportCallback CGamePadDeviceListener::GetCallback(CGamePadDeviceListener* GPDL, IOHIDDeviceRef device)
{
	auto vid = CGamePadUtils::GetIntProperty(device, CFSTR(kIOHIDVendorIDKey));
	auto pid = CGamePadUtils::GetIntProperty(device, CFSTR(kIOHIDProductIDKey));
	if(vid == 0x54C)
	{
		if(pid == 0x9CC || pid == 0x5C4)
		{
			return GPDL->InputReportCallbackStub_DS4;
		}
		else if(pid == 0x268)
		{
			return GPDL->InputReportCallbackStub_DS3;
		}
	}
	return nullptr;
}

void CGamePadDeviceListener::InputDeviceListenerThread()
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

	IOHIDManagerRegisterDeviceMatchingCallback(m_hidManager, onDeviceMatched, this);
	//IOHIDManagerRegisterDeviceRemovalCallback(m_hidManager, onDeviceRemoved, this);

	IOHIDManagerOpen(m_hidManager, kIOHIDOptionsTypeNone);
	IOHIDManagerScheduleWithRunLoop(m_hidManager, CFRunLoopGetCurrent(), CFSTR("CustomLoop"));
	while(CFRunLoopRunInMode(CFSTR("CustomLoop"), 0, true) != kCFRunLoopRunStopped && m_running)
	{
	}
}
