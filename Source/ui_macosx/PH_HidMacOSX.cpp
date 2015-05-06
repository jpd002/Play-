#include "PH_HidMacOSX.h"
#include <stdexcept>

CPH_HidMacOSX::CPH_HidMacOSX()
{
	m_bindings[PS2::CControllerInfo::ANALOG_LEFT_X]		= std::make_shared<CSimulatedAxisBinding>(kHIDUsage_KeyboardD, kHIDUsage_KeyboardG);
	m_bindings[PS2::CControllerInfo::ANALOG_LEFT_Y]		= std::make_shared<CSimulatedAxisBinding>(kHIDUsage_KeyboardR, kHIDUsage_KeyboardF);

	m_bindings[PS2::CControllerInfo::ANALOG_RIGHT_X]	= std::make_shared<CSimulatedAxisBinding>(kHIDUsage_KeyboardH, kHIDUsage_KeyboardK);
	m_bindings[PS2::CControllerInfo::ANALOG_RIGHT_Y]	= std::make_shared<CSimulatedAxisBinding>(kHIDUsage_KeyboardU, kHIDUsage_KeyboardJ);
	
	m_bindings[PS2::CControllerInfo::START]				= std::make_shared<CSimpleBinding>(kHIDUsage_KeyboardReturnOrEnter);
	m_bindings[PS2::CControllerInfo::SELECT]			= std::make_shared<CSimpleBinding>(kHIDUsage_KeyboardRightShift);
	m_bindings[PS2::CControllerInfo::DPAD_LEFT]			= std::make_shared<CSimpleBinding>(kHIDUsage_KeyboardLeftArrow);
	m_bindings[PS2::CControllerInfo::DPAD_RIGHT]		= std::make_shared<CSimpleBinding>(kHIDUsage_KeyboardRightArrow);
	m_bindings[PS2::CControllerInfo::DPAD_UP]			= std::make_shared<CSimpleBinding>(kHIDUsage_KeyboardUpArrow);
	m_bindings[PS2::CControllerInfo::DPAD_DOWN]			= std::make_shared<CSimpleBinding>(kHIDUsage_KeyboardDownArrow);
	m_bindings[PS2::CControllerInfo::SQUARE]			= std::make_shared<CSimpleBinding>(kHIDUsage_KeyboardA);
	m_bindings[PS2::CControllerInfo::CROSS]				= std::make_shared<CSimpleBinding>(kHIDUsage_KeyboardZ);
	m_bindings[PS2::CControllerInfo::TRIANGLE]			= std::make_shared<CSimpleBinding>(kHIDUsage_KeyboardS);
	m_bindings[PS2::CControllerInfo::CIRCLE]			= std::make_shared<CSimpleBinding>(kHIDUsage_KeyboardX);
		
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
	for(auto listenerIterator(std::begin(m_listeners));
		listenerIterator != std::end(m_listeners); listenerIterator++)
	{
		auto* listener(*listenerIterator);
		
		for(unsigned int i = 0; i < PS2::CControllerInfo::MAX_BUTTONS; i++)
		{
			const auto& binding = m_bindings[i];
			if(!binding) continue;
			uint32 value = binding->GetValue();
			auto currentButtonId = static_cast<PS2::CControllerInfo::BUTTON>(i);
			if(PS2::CControllerInfo::IsAxis(currentButtonId))
			{
				listener->SetAxisState(0, currentButtonId, value & 0xFF, ram);
			}
			else
			{
				listener->SetButtonState(0, currentButtonId, value != 0, ram);
			}
		}
	}
}

CPadHandler::FactoryFunction CPH_HidMacOSX::GetFactoryFunction()
{
	//Needs to be created in the same thread as UI
	return std::bind(&CPH_HidMacOSX::PadHandlerFactory, new CPH_HidMacOSX());
}

CPadHandler* CPH_HidMacOSX::PadHandlerFactory(CPH_HidMacOSX* handler)
{
	return handler;
}

CFMutableDictionaryRef CPH_HidMacOSX::CreateDeviceMatchingDictionary(uint32 usagePage, uint32 usage)
{
	CFMutableDictionaryRef result = CFDictionaryCreateMutable(kCFAllocatorDefault, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks );
	if(result == NULL) throw std::runtime_error("CFDictionaryCreateMutable failed.");
	if(usagePage != 0)
	{
		// Add key for device type to refine the matching dictionary.
		CFNumberRef pageCFNumberRef = CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &usagePage);
		if(pageCFNumberRef == NULL) throw std::runtime_error("CFNumberCreate failed.");
		CFDictionarySetValue(result, CFSTR(kIOHIDDeviceUsagePageKey), pageCFNumberRef);
		CFRelease(pageCFNumberRef);
		// note: the usage is only valid if the usage page is also defined
		if(usage != 0) 
		{
			CFNumberRef usageCFNumberRef = CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &usage);
			if(usageCFNumberRef == NULL) throw std::runtime_error("CFNumberCreate failed.");
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
	for(auto bindingIterator(std::begin(m_bindings));
		bindingIterator != std::end(m_bindings); bindingIterator++)
	{
		const auto& binding = (*bindingIterator);
		if(!binding) continue;
		binding->ProcessEvent(usage, state);
	}
}

//---------------------------------------------------------------------------------

CPH_HidMacOSX::CSimpleBinding::CSimpleBinding(uint32 keyCode)
: m_keyCode(keyCode)
, m_state(0)
{
	
}

CPH_HidMacOSX::CSimpleBinding::~CSimpleBinding()
{
	
}

void CPH_HidMacOSX::CSimpleBinding::ProcessEvent(uint32 keyCode, uint32 state)
{
	if(keyCode != m_keyCode) return;
	m_state = state;
}

uint32 CPH_HidMacOSX::CSimpleBinding::GetValue() const
{
	return m_state;
}

//---------------------------------------------------------------------------------

CPH_HidMacOSX::CSimulatedAxisBinding::CSimulatedAxisBinding(uint32 negativeKeyCode, uint32 positiveKeyCode)
: m_negativeKeyCode(negativeKeyCode)
, m_positiveKeyCode(positiveKeyCode)
, m_negativeState(0)
, m_positiveState(0)
{
	
}

CPH_HidMacOSX::CSimulatedAxisBinding::~CSimulatedAxisBinding()
{
	
}

void CPH_HidMacOSX::CSimulatedAxisBinding::ProcessEvent(uint32 keyCode, uint32 state)
{
	if(keyCode == m_negativeKeyCode)
	{
		m_negativeState = state;
	}
	
	if(keyCode == m_positiveKeyCode)
	{
		m_positiveState = state;
	}	
}

uint32 CPH_HidMacOSX::CSimulatedAxisBinding::GetValue() const
{
	uint32 value = 0x7F;

	if(m_negativeState)
	{
		value -= 0x7F;
	}
	if(m_positiveState)
	{
		value += 0x7F;
	}
	
	return value;
}
