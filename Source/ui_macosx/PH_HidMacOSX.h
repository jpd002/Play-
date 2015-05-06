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
	class CBinding
	{
	public:
		virtual			~CBinding() {}
		
		virtual void	ProcessEvent(uint32, uint32) = 0;
		
		virtual uint32	GetValue() const = 0;
	};
	
	typedef std::shared_ptr<CBinding> BindingPtr;
	
	class CSimpleBinding : public CBinding
	{
	public:
						CSimpleBinding(uint32);
		virtual			~CSimpleBinding();
		
		virtual void	ProcessEvent(uint32, uint32);
		
		virtual uint32	GetValue() const;
		
	private:
		uint32			m_keyCode;
		uint32			m_state;
	};
	
	class CSimulatedAxisBinding : public CBinding
	{
	public:
						CSimulatedAxisBinding(uint32, uint32);
		virtual			~CSimulatedAxisBinding();
		
		virtual void	ProcessEvent(uint32, uint32);
		
		virtual uint32	GetValue() const;
		
	private:
		uint32			m_negativeKeyCode;
		uint32			m_positiveKeyCode;
		
		uint32			m_negativeState;
		uint32			m_positiveState;
	};
		
	static CPadHandler*		PadHandlerFactory(CPH_HidMacOSX*);
	
	CFMutableDictionaryRef	CreateDeviceMatchingDictionary(uint32, uint32);
	static void				InputValueCallbackStub(void*, IOReturn, void*, IOHIDValueRef);
	void					InputValueCallback(IOHIDValueRef);
	
	IOHIDManagerRef			m_hidManager;
	BindingPtr				m_bindings[PS2::CControllerInfo::MAX_BUTTONS];
};

#endif
