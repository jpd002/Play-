#include "InputProviderEvDev.h"
#include "string_format.h"

#define PROVIDER_ID 'evdv'

CInputProviderEvDev::CInputProviderEvDev()
	: m_GPDL([this] (auto a, auto b, auto c, auto d, auto e){ this->OnEvDevInputEvent(a, b, c, d, e); })
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
		if(code == ABS_HAT0X || code == ABS_HAT0Y)
		{
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
			tgt.keyType = BINDINGTARGET::KEYTYPE::AXIS;
		}
		OnInput(tgt, fixedValue);
	}
}
