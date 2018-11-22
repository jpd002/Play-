#include <QKeySequence>
#include <cassert>
#include "InputProviderQtKey.h"
#include "string_format.h"

#define PROVIDER_ID 'QtKy'

uint32 CInputProviderQtKey::GetId() const
{
	return PROVIDER_ID;
}

std::string CInputProviderQtKey::GetTargetDescription(const BINDINGTARGET& target) const
{
	assert(target.providerId == PROVIDER_ID);
	auto description = QKeySequence(target.keyId).toString();
	return string_format("Keyboard: %s", description.toStdString().c_str());
}

BINDINGTARGET CInputProviderQtKey::MakeBindingTarget(int keyCode)
{
	return BINDINGTARGET(PROVIDER_ID, DeviceIdType{{0}}, keyCode, BINDINGTARGET::KEYTYPE::BUTTON);
}

void CInputProviderQtKey::OnKeyPress(int keyCode)
{
	if(!OnInput) return;
	OnInput(MakeBindingTarget(keyCode), 1);
}

void CInputProviderQtKey::OnKeyRelease(int keyCode)
{
	if(!OnInput) return;
	OnInput(MakeBindingTarget(keyCode), 0);
}
