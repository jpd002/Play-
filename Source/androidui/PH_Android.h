#pragma once

#include "../PadHandler.h"

class CPH_Android : public CPadHandler
{
public:
								CPH_Android();
	virtual						~CPH_Android();
	
	static FactoryFunction		GetFactoryFunction();

	void						Update(uint8*) override;

	void						ReportInput(bool);
	
private:
	bool						m_pressed = false;
};
