#include "PH_HidUnix.h"
#include <stdexcept>
#include "AppConfig.h"

CPH_HidUnix::CPH_HidUnix()
 : m_inputManager(CAppConfig::GetInstance())
{

}

CPH_HidUnix::~CPH_HidUnix()
{
}

void CPH_HidUnix::Update(uint8* ram)
{
    for(auto listenerIterator(std::begin(m_listeners));
        listenerIterator != std::end(m_listeners); listenerIterator++)
    {
        auto* listener(*listenerIterator);

        for(unsigned int i = 0; i < PS2::CControllerInfo::MAX_BUTTONS; i++)
        {
            auto button = static_cast<PS2::CControllerInfo::BUTTON>(i);
            const auto& binding = m_inputManager.GetBinding(button);
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

void CPH_HidUnix::InputValueCallbackPressed(uint32 valueRef, uint32 type)
{
    InputValueCallback({0}, valueRef, 1, type);
}

void CPH_HidUnix::InputValueCallbackReleased(uint32 valueRef, uint32 type)
{
    InputValueCallback({0}, valueRef, 0, type);
}

void CPH_HidUnix::InputValueCallback(std::array<uint32, 6> device, uint32 value, uint32 action, uint32 type)
{
	m_inputManager.OnInputEventReceived(device, value, action);
}
