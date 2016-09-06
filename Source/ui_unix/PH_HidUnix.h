#ifndef _PH_HIDUnix_H_
#define _PH_HIDUnix_H_

#include "PadHandler.h"
#include <memory>

class CPH_HidUnix : public CPadHandler
{
public:
    CPH_HidUnix();
    virtual					~CPH_HidUnix();

    void					Update(uint8*) override;

    static FactoryFunction			GetFactoryFunction();
    void				InputValueCallbackPressed(uint32 valueRef, int type);
    void				InputValueCallbackReleased(uint32 valueRef, int type);

private:
    class CBinding
    {
    public:
        virtual			~CBinding() {}

        virtual void		ProcessEvent(uint32, uint32, int) = 0;

        virtual uint32		GetValue() const = 0;
    };
public:
    typedef std::shared_ptr<CBinding> BindingPtr;

    class CSimpleBinding : public CBinding
    {
    public:
        CSimpleBinding(uint32, int t = 0);
        virtual			~CSimpleBinding();

        virtual void		ProcessEvent(uint32, uint32, int);

        virtual uint32		GetValue() const;

    private:
        uint32			m_keyCode;
        uint32			m_state;
        int				m_controllerType;
    };

    class CSimulatedAxisBinding : public CBinding
    {
    public:
        CSimulatedAxisBinding(uint32, uint32, int = 0);
        virtual			~CSimulatedAxisBinding();

        virtual void		ProcessEvent(uint32, uint32, int);

        virtual uint32		GetValue() const;

    private:
        uint32			m_negativeKeyCode;
        uint32			m_positiveKeyCode;

        uint32			m_negativeState;
        uint32			m_positiveState;

        int				m_controllerType;
    };

    static CPadHandler*		PadHandlerFactory(CPH_HidUnix*);


    void				InputValueCallback(uint32 value, uint32 action, int type);


    BindingPtr			m_bindings[PS2::CControllerInfo::MAX_BUTTONS];
    void UpdateBinding(BindingPtr *bindarr);
};

#endif
