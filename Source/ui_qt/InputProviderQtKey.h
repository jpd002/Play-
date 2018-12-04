#pragma once

#include <Qt>
#include "input/InputProvider.h"

class CInputProviderQtKey : public CInputProvider
{
public:
	uint32 GetId() const override;
	std::string GetTargetDescription(const BINDINGTARGET&) const override;

	static BINDINGTARGET MakeBindingTarget(int);

	void OnKeyPress(int);
	void OnKeyRelease(int);
};
