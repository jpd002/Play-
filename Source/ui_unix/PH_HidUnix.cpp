#include "PH_HidUnix.h"
#include <stdexcept>
#include "AppConfig.h"

CPH_HidUnix::CPH_HidUnix(CInputBindingManager* inputBindingManager)
 : m_inputManager(inputBindingManager)
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
			const auto& binding = m_inputManager->GetBinding(button);
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

CPadHandler::FactoryFunction CPH_HidUnix::GetFactoryFunction(CInputBindingManager* inputBindingManager)
{
	//Needs to be created in the same thread as UI
	return std::bind(&CPH_HidUnix::PadHandlerFactory, new CPH_HidUnix(inputBindingManager));
}

CPadHandler* CPH_HidUnix::PadHandlerFactory(CPH_HidUnix* handler)
{
	return handler;
}
