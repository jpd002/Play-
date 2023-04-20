#pragma once

#include <Qt>
#include "input/InputProvider.h"

class CInputProviderQtMouse : public CInputProvider
{
public:
	uint32 GetId() const override;
	std::string GetTargetDescription(const BINDINGTARGET&) const override;

	static BINDINGTARGET MakeBindingTarget(Qt::MouseButton);

	void OnMousePress(Qt::MouseButton);
	void OnMouseRelease(Qt::MouseButton);
};
