#include "PH_HidMacOSX.h"
#include <stdexcept>

using namespace std;
using namespace std::tr1;

CPH_HidMacOSX::CPH_HidMacOSX() :
m_currentState(0),
m_previousState(0)
{
	m_hidManager = IOHIDManagerCreate(kCFAllocatorDefault, 0);
	{
		CFDictionaryRef matchingDict = CreateDeviceMatchingDictionary(kHIDPage_GenericDesktop, kHIDUsage_GD_Keyboard);
		IOHIDManagerSetDeviceMatching(m_hidManager, matchingDict);
		CFRelease(matchingDict);
	}
	IOHIDManagerScheduleWithRunLoop(m_hidManager, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
	IOHIDManagerOpen(m_hidManager, kIOHIDOptionsTypeNone);
	IOHIDManagerRegisterInputValueCallback(m_hidManager, InputValueCallbackStub, this);
}

CPH_HidMacOSX::~CPH_HidMacOSX()
{
	IOHIDManagerClose(m_hidManager, 0);
}

void CPH_HidMacOSX::Update(uint8* ram)
{
	for(ListenerList::iterator listenerIterator(m_listeners.begin());
		listenerIterator != m_listeners.end(); listenerIterator++)
	{
		CPadListener* listener(*listenerIterator);
		for(unsigned int i = 0; i < 16; i++)
		{
			uint32 previousBit = m_previousState & (1 << i);
			uint32 currentBit = m_currentState & (1 << i);
			if(currentBit != previousBit)
			{
				listener->SetButtonState(0, static_cast<CPadListener::BUTTON>(1 << i), currentBit != 0, ram);
			}
		}
	}
	m_previousState = m_currentState;
}

CPadHandler::FactoryFunction CPH_HidMacOSX::GetFactoryFunction()
{
	//Needs to be created in the same thread as UI
	return bind(&CPH_HidMacOSX::PadHandlerFactory, new CPH_HidMacOSX());
}

CPadHandler* CPH_HidMacOSX::PadHandlerFactory(CPH_HidMacOSX* handler)
{
	return handler;
}

CFMutableDictionaryRef CPH_HidMacOSX::CreateDeviceMatchingDictionary(uint32 usagePage, uint32 usage)
{
    CFMutableDictionaryRef result = CFDictionaryCreateMutable(kCFAllocatorDefault, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks );
	if(result == NULL) throw runtime_error("CFDictionaryCreateMutable failed.");
	if(usagePage != 0)
	{
		// Add key for device type to refine the matching dictionary.
		CFNumberRef pageCFNumberRef = CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &usagePage);
		if(pageCFNumberRef == NULL) throw runtime_error("CFNumberCreate failed.");
		CFDictionarySetValue(result, CFSTR(kIOHIDDeviceUsagePageKey), pageCFNumberRef);
		CFRelease(pageCFNumberRef);
		// note: the usage is only valid if the usage page is also defined
		if(usage != 0) 
		{
			CFNumberRef usageCFNumberRef = CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &usage);
			if(usageCFNumberRef == NULL) throw runtime_error("CFNumberCreate failed.");
			CFDictionarySetValue(result, CFSTR(kIOHIDDeviceUsageKey), usageCFNumberRef);
			CFRelease(usageCFNumberRef);
		}
	}
    return result;
}

void CPH_HidMacOSX::InputValueCallbackStub(void* context, IOReturn result, void* sender, IOHIDValueRef valueRef)
{
	reinterpret_cast<CPH_HidMacOSX*>(context)->InputValueCallback(valueRef);
}

void CPH_HidMacOSX::InputValueCallback(IOHIDValueRef valueRef)
{
	IOHIDElementRef elementRef = IOHIDValueGetElement(valueRef);
	uint32 usage = IOHIDElementGetUsage(elementRef);
	uint32 usagePage = IOHIDElementGetUsagePage(elementRef);
	CFIndex state = IOHIDValueGetIntegerValue(valueRef);
	if(usagePage != kHIDPage_KeyboardOrKeypad) return;
	uint32 gameKey = 0;
	if(TranslateKey(usage, gameKey))
	{
		if(state)
		{
			m_currentState |= gameKey;
		}
		else
		{
			m_currentState &= ~gameKey;
		}
	}
}

bool CPH_HidMacOSX::TranslateKey(uint32 scanCode, uint32& gameKey)
{
	switch(scanCode)
	{
	case kHIDUsage_KeyboardReturn:
		gameKey = CPadListener::BUTTON_START;
		return true;
		break;
	case kHIDUsage_KeyboardLeftShift:
	case kHIDUsage_KeyboardRightShift:
		gameKey = CPadListener::BUTTON_SELECT;
		return true;
		break;
	case kHIDUsage_KeyboardLeftArrow:
		gameKey = CPadListener::BUTTON_LEFT;
		return true;
		break;
	case kHIDUsage_KeyboardRightArrow:
		gameKey = CPadListener::BUTTON_RIGHT;
		return true;
		break;
	case kHIDUsage_KeyboardUpArrow:
		gameKey = CPadListener::BUTTON_UP;
		return true;
		break;
	case kHIDUsage_KeyboardDownArrow:
		gameKey = CPadListener::BUTTON_DOWN;
		return true;
		break;
	case kHIDUsage_KeyboardA:
		gameKey = CPadListener::BUTTON_SQUARE;
		return true;
		break;
	case kHIDUsage_KeyboardZ:
		gameKey = CPadListener::BUTTON_CROSS;
		return true;
		break;
	case kHIDUsage_KeyboardS:
		gameKey = CPadListener::BUTTON_TRIANGLE;
		return true;
		break;
	case kHIDUsage_KeyboardX:
		gameKey = CPadListener::BUTTON_CIRCLE;
		return true;
		break;
	}
	return false;
}
