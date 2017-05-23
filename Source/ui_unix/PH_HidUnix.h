#ifndef _PH_HIDUnix_H_
#define _PH_HIDUnix_H_

#include "PadHandler.h"
#include "Config.h"
#include <memory>
#include "InputEventManager.h"

class CPH_HidUnix : public CPadHandler
{
public:
    CPH_HidUnix();
    virtual					~CPH_HidUnix();

    void					Update(uint8*) override;

    static FactoryFunction			GetFactoryFunction();
    void				InputValueCallbackPressed(uint32 valueRef, int type);
    void				InputValueCallbackReleased(uint32 valueRef, int type);

    static CPadHandler*		PadHandlerFactory(CPH_HidUnix*);


    void				InputValueCallback(int, uint32 value, uint32 action, int type);
    CInputBindingManager           m_inputManager;
};

#endif
