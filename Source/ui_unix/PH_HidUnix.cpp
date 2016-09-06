#include "PH_HidUnix.h"
#include <qnamespace.h>
#include <stdexcept>

CPH_HidUnix::CPH_HidUnix()
{
    m_bindings[PS2::CControllerInfo::ANALOG_LEFT_X]		= std::make_shared<CSimulatedAxisBinding>(Qt::Key_D, Qt::Key_G);
    m_bindings[PS2::CControllerInfo::ANALOG_LEFT_Y]		= std::make_shared<CSimulatedAxisBinding>(Qt::Key_R, Qt::Key_F);

    m_bindings[PS2::CControllerInfo::ANALOG_RIGHT_X]	= std::make_shared<CSimulatedAxisBinding>(Qt::Key_H, Qt::Key_K);
    m_bindings[PS2::CControllerInfo::ANALOG_RIGHT_Y]	= std::make_shared<CSimulatedAxisBinding>(Qt::Key_U, Qt::Key_J);

    m_bindings[PS2::CControllerInfo::START]				= std::make_shared<CSimpleBinding>(Qt::Key_Return);
    m_bindings[PS2::CControllerInfo::SELECT]			= std::make_shared<CSimpleBinding>(Qt::Key_Backspace);
    m_bindings[PS2::CControllerInfo::DPAD_LEFT]			= std::make_shared<CSimpleBinding>(Qt::Key_Left);
    m_bindings[PS2::CControllerInfo::DPAD_RIGHT]		= std::make_shared<CSimpleBinding>(Qt::Key_Right);
    m_bindings[PS2::CControllerInfo::DPAD_UP]			= std::make_shared<CSimpleBinding>(Qt::Key_Up);
    m_bindings[PS2::CControllerInfo::DPAD_DOWN]			= std::make_shared<CSimpleBinding>(Qt::Key_Down);
    m_bindings[PS2::CControllerInfo::SQUARE]			= std::make_shared<CSimpleBinding>(Qt::Key_A);
    m_bindings[PS2::CControllerInfo::CROSS]				= std::make_shared<CSimpleBinding>(Qt::Key_Z);
    m_bindings[PS2::CControllerInfo::TRIANGLE]			= std::make_shared<CSimpleBinding>(Qt::Key_S);
    m_bindings[PS2::CControllerInfo::CIRCLE]			= std::make_shared<CSimpleBinding>(Qt::Key_X);
}

CPH_HidUnix::~CPH_HidUnix()
{
}

void CPH_HidUnix::UpdateBinding(BindingPtr* bindarr)
{
    if (bindarr == nullptr) return;
    for(unsigned int i = 0; i < PS2::CControllerInfo::MAX_BUTTONS; i++)
    {
            if (bindarr[i] != 0)
                m_bindings[i] = bindarr[i];
    }
}


void CPH_HidUnix::Update(uint8* ram)
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

CPadHandler::FactoryFunction CPH_HidUnix::GetFactoryFunction()
{
    //Needs to be created in the same thread as UI
    return std::bind(&CPH_HidUnix::PadHandlerFactory, new CPH_HidUnix());
}

CPadHandler* CPH_HidUnix::PadHandlerFactory(CPH_HidUnix* handler)
{
    return handler;
}

void CPH_HidUnix::InputValueCallbackPressed(uint32 valueRef, int type)
{
    InputValueCallback(valueRef, 1, type);
}

void CPH_HidUnix::InputValueCallbackReleased(uint32 valueRef, int type)
{
    InputValueCallback(valueRef, 0, type);
}

void CPH_HidUnix::InputValueCallback(uint32 value, uint32 action, int type)
{
    for(auto bindingIterator(std::begin(m_bindings));
        bindingIterator != std::end(m_bindings); bindingIterator++)
    {
        const auto& binding = (*bindingIterator);
        if(!binding) continue;
        binding->ProcessEvent(value, action, type);
    }
}

//---------------------------------------------------------------------------------

CPH_HidUnix::CSimpleBinding::CSimpleBinding(uint32 keyCode, int controllerType)
    : m_keyCode(keyCode)
    , m_state(0)
    , m_controllerType(controllerType)
{

}

CPH_HidUnix::CSimpleBinding::~CSimpleBinding()
{

}

void CPH_HidUnix::CSimpleBinding::ProcessEvent(uint32 keyCode, uint32 state, int controllerType)
{
    if(keyCode != m_keyCode || controllerType != m_controllerType) return;
    m_state = state;
}

uint32 CPH_HidUnix::CSimpleBinding::GetValue() const
{
    return m_state;
}

//---------------------------------------------------------------------------------

CPH_HidUnix::CSimulatedAxisBinding::CSimulatedAxisBinding(uint32 negativeKeyCode, uint32 positiveKeyCode, int controllerType)
    : m_negativeKeyCode(negativeKeyCode)
    , m_positiveKeyCode(positiveKeyCode)
    , m_negativeState(0)
    , m_positiveState(0)
    , m_controllerType(controllerType)
{

}

CPH_HidUnix::CSimulatedAxisBinding::~CSimulatedAxisBinding()
{

}

void CPH_HidUnix::CSimulatedAxisBinding::ProcessEvent(uint32 keyCode, uint32 state, int controllerType)
{
    if(controllerType != m_controllerType) return;
    if(keyCode == m_negativeKeyCode)
    {
        m_negativeState = state;
    }

    if(keyCode == m_positiveKeyCode)
    {
        m_positiveState = state;
    }
}

uint32 CPH_HidUnix::CSimulatedAxisBinding::GetValue() const
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
