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
	auto GPDL = reinterpret_cast<CGamePadDeviceListener*>(context);
	if(!GPDL->OnInputEventCallBack)
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
	if(GPDL->m_filter)
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
	GPDL->OnInputEventCallBack(CGamePadUtils::GetDeviceID(device), usage, value, btn_type);
}

void CGamePadDeviceListener::onDeviceMatched(void* context, IOReturn result, void* sender, IOHIDDeviceRef device)
{
	auto GPDL = reinterpret_cast<CGamePadDeviceListener*>(context);
	if(GPDL->OnInputEventCallBack)
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

	IOHIDDeviceRegisterInputValueCallback(device, GPDL->InputValueCallbackStub, context);
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
