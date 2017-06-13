#pragma once
#include "PadHandler.h"
#include "Config.h"
#include <memory>
#include "InputBindingManager.h"

class CPH_HidUnix : public CPadHandler
{
public:
	CPH_HidUnix(CInputBindingManager*);
	virtual						~CPH_HidUnix();

	void						Update(uint8*) override;

	static FactoryFunction		GetFactoryFunction(CInputBindingManager*);
	void						InputValueCallbackPressed(uint32 valueRef, uint32 type);
	void						InputValueCallbackReleased(uint32 valueRef, uint32 type);

	static CPadHandler*			PadHandlerFactory(CPH_HidUnix*);


	void						InputValueCallback(std::array<uint32, 6>, uint32 value, uint32 action, uint32 type);
	CInputBindingManager		*m_inputManager;
};
