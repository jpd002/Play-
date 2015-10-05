#pragma once

#include "../PadHandler.h"

class CPH_iOS : public CPadHandler
{
public:
								CPH_iOS();
	virtual						~CPH_iOS();
	
	static FactoryFunction		GetFactoryFunction();

	void						Update(uint8*) override;

	void						SetButtonState(PS2::CControllerInfo::BUTTON, bool);
	void						SetAxisState(PS2::CControllerInfo::BUTTON, float);
	
private:
	bool						m_buttonStates[PS2::CControllerInfo::MAX_BUTTONS];
	float						m_axisStates[PS2::CControllerInfo::MAX_BUTTONS];
};
