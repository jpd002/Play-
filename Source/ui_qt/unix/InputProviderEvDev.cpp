#include <cassert>
#include "InputProviderEvDev.h"
#include "string_format.h"

#define PROVIDER_ID 'evdv'

CInputProviderEvDev::CInputProviderEvDev()
    : m_GPDL([this](auto a, auto b, auto c, auto d, auto e) { this->OnEvDevInputEvent(a, b, c, d, e); })
{
}

uint32 CInputProviderEvDev::GetId() const
{
	return PROVIDER_ID;
}

std::string CInputProviderEvDev::GetTargetDescription(const BINDINGTARGET& target) const
{
	return string_format("EvDev: btn-%d", target.keyId);
}

void CInputProviderEvDev::OnEvDevInputEvent(GamePadDeviceId deviceId, int code, int value, int type, const input_absinfo* abs)
{
	if(!OnInput) return;
	BINDINGTARGET tgt;
	tgt.providerId = PROVIDER_ID;
	tgt.deviceId = deviceId;
	tgt.keyId = code;
	if(type == EV_MSC)
	{
		return;
	}
	else if(type == EV_KEY)
	{
		tgt.keyType = BINDINGTARGET::KEYTYPE::BUTTON;
		OnInput(tgt, value);
	}
	else if(type == EV_ABS)
	{
		int fixedValue = value;
		if(abs->flat == 0)
		{
			//Assuming this is a POVhat
			tgt.keyType = BINDINGTARGET::KEYTYPE::POVHAT;
			switch(value)
			{
			case 0:
				fixedValue = BINDINGTARGET::POVHAT_MAX;
				break;
			case 1:
				fixedValue = 0;
				break;
			case -1:
				fixedValue = 4;
				break;
			default:
				assert(false);
				break;
			}
		}
		else
		{
			int range = (abs->maximum - abs->minimum);
			int center = range / 2;
			fixedValue = (((value - abs->minimum) + center) * 255) / range;
			printf("range: %d, center: %d, fixedValue: %d\r\n", range, center, fixedValue);
			tgt.keyType = BINDINGTARGET::KEYTYPE::AXIS;
		}
		OnInput(tgt, fixedValue);
	}
}
