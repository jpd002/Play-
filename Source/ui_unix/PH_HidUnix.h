#pragma once
#include "Config.h"
#include "InputBindingManager.h"
#include "PadHandler.h"
#include <memory>

class CPH_HidUnix : public CPadHandler
{
public:
	CPH_HidUnix(CInputBindingManager*);
	virtual ~CPH_HidUnix();

	void Update(uint8*) override;

	static FactoryFunction GetFactoryFunction(CInputBindingManager*);

private:
	static CPadHandler*   PadHandlerFactory(CPH_HidUnix*);
	CInputBindingManager* m_inputManager;
};
