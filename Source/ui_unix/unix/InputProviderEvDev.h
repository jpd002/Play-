#pragma once

#include "input/InputProvider.h"
#include "GamePadDeviceListener.h"

class CInputProviderEvDev : public CInputProvider
{
public:
	CInputProviderEvDev();
	virtual ~CInputProviderEvDev();

	uint32 GetId() const override;
	std::string GetTargetDescription(const BINDINGTARGET&) const override;

private:
	CGamePadDeviceListener m_GPDL;
};
