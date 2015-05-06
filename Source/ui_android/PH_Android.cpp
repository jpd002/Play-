#include "PH_Android.h"

CPH_Android::CPH_Android()
{
	
}

CPH_Android::~CPH_Android()
{
	
}

CPadHandler::FactoryFunction CPH_Android::GetFactoryFunction()
{
	return [] () { return new CPH_Android(); };
}

void CPH_Android::Update(uint8* ram)
{
	for(auto& listener : m_listeners)
	{
		listener->SetButtonState(0, PS2::CControllerInfo::CROSS, m_pressed ? true : false, ram);
	}
}

void CPH_Android::ReportInput(bool pressed)
{
	m_pressed = pressed;
}
