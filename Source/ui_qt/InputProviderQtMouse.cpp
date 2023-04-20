#include <cassert>
#include "InputProviderQtMouse.h"
#include "string_format.h"

#define PROVIDER_ID 'QtMo'

uint32 CInputProviderQtMouse::GetId() const
{
	return PROVIDER_ID;
}

std::string CInputProviderQtMouse::GetTargetDescription(const BINDINGTARGET& target) const
{
	assert(target.providerId == PROVIDER_ID);
	auto description = [&]() -> std::string
	{
		switch(target.keyId)
		{
			case Qt::LeftButton:
				return "Left Button";
			case Qt::RightButton:
				return "Right Button";
			default:
				return string_format("Unknown button %d", target.keyId);
		}
	}();
	return string_format("Mouse: %s", description.c_str());
}

BINDINGTARGET CInputProviderQtMouse::MakeBindingTarget(Qt::MouseButton mouseButton)
{
	return BINDINGTARGET(PROVIDER_ID, DeviceIdType{{0}}, mouseButton, BINDINGTARGET::KEYTYPE::BUTTON);
}

void CInputProviderQtMouse::OnMousePress(Qt::MouseButton mouseButton)
{
	if(!OnInput) return;
	OnInput(MakeBindingTarget(mouseButton), 1);
}

void CInputProviderQtMouse::OnMouseRelease(Qt::MouseButton mouseButton)
{
	if(!OnInput) return;
	OnInput(MakeBindingTarget(mouseButton), 0);
}
