#pragma once

#include "input/InputProvider.h"
#include "GamePadUtils.h"
#include "GamePadDeviceListener.h"

class CInputProviderEvDev : public CInputProvider
{
public:
	CInputProviderEvDev();
	virtual ~CInputProviderEvDev() = default;

	uint32 GetId() const override;
	std::string GetTargetDescription(const BINDINGTARGET&) const override;

private:
	void OnEvDevInputEvent(GamePadDeviceId, int, int, int, const input_absinfo*);
	CGamePadDeviceListener m_GPDL;
};
