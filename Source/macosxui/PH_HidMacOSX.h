#ifndef _PH_HIDMACOSX_H_
#define _PH_HIDMACOSX_H_

#include "../PadHandler.h"
#import <IOKit/hid/IOHIDLib.h>

class CPH_HidMacOSX : public CPadHandler
{
public:
							CPH_HidMacOSX();
	virtual					~CPH_HidMacOSX();
	
	void					Update(uint8*);
	
	static FactoryFunction	GetFactoryFunction();
	
private:
	static CPadHandler*		PadHandlerFactory(CPH_HidMacOSX*);
	
	CFMutableDictionaryRef	CreateDeviceMatchingDictionary(uint32, uint32);
	static void				InputValueCallbackStub(void*, IOReturn, void*, IOHIDValueRef);
	void					InputValueCallback(IOHIDValueRef);
	bool					TranslateKey(uint32, uint32&);
	
	IOHIDManagerRef			m_hidManager;
	uint32					m_currentState;
	uint32					m_previousState;
};

#endif
