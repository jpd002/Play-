#pragma once

#include "PadHandler.h"
#include "InputBindingManager.h"

class CPH_GenericInput : public CPadHandler
{
public:
	CPH_GenericInput() = default;
	virtual ~CPH_GenericInput() = default;

	void Update(uint8*) override;

	CInputBindingManager& GetBindingManager();
	
	static FactoryFunction GetFactoryFunction();

private:
	CInputBindingManager m_bindingManager;
};
