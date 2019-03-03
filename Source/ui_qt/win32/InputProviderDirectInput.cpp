#include "InputProviderDirectInput.h"
#include "string_format.h"
#include "string_cast.h"

#define PROVIDER_ID 'dinp'

static_assert(sizeof(GUID) <= sizeof(DeviceIdType::value_type) * DeviceIdTypeCount, "DeviceIdType cannot hold GUID");

static DeviceIdType GuidToDeviceId(const GUID& guid)
{
	DeviceIdType deviceId;
	for(auto& deviceIdElem : deviceId)
	{
		deviceIdElem = 0;
	}
	memcpy(deviceId.data(), &guid, sizeof(GUID));
	return deviceId;
}

GUID DeviceIdToGuid(const DeviceIdType& deviceId)
{
	GUID guid;
	memcpy(&guid, deviceId.data(), sizeof(GUID));
	return guid;
}

CInputProviderDirectInput::CInputProviderDirectInput()
{
	m_diManager = std::make_unique<Framework::DirectInput::CManager>();
	m_diManager->RegisterInputEventHandler(std::bind(&CInputProviderDirectInput::HandleInputEvent, this,
	                                                 std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
	m_diManager->CreateJoysticks();
}

uint32 CInputProviderDirectInput::GetId() const
{
	return PROVIDER_ID;
}

std::string CInputProviderDirectInput::GetTargetDescription(const BINDINGTARGET& target) const
{
	auto deviceId = DeviceIdToGuid(target.deviceId);
	std::string deviceName = "Unknown Device";
	std::string deviceKeyName = "Unknown Key";

	DIDEVICEINSTANCE deviceInstance = {};
	if(m_diManager->GetDeviceInfo(deviceId, &deviceInstance))
	{
		deviceName = string_cast<std::string>(deviceInstance.tszInstanceName);
	}

	DIDEVICEOBJECTINSTANCE objectInstance = {};
	if(m_diManager->GetDeviceObjectInfo(deviceId, target.keyId, &objectInstance))
	{
		deviceKeyName = string_cast<std::string>(objectInstance.tszName);
	}

	return string_format("%s: %s", deviceName.c_str(), deviceKeyName.c_str());
}

void CInputProviderDirectInput::HandleInputEvent(const GUID& deviceId, uint32 keyId, uint32 value)
{
	DIDEVICEOBJECTINSTANCE objectInstance = {};
	if(!m_diManager->GetDeviceObjectInfo(deviceId, keyId, &objectInstance))
	{
		return;
	}

	BINDINGTARGET tgt;
	tgt.providerId = PROVIDER_ID;
	tgt.deviceId = GuidToDeviceId(deviceId);
	tgt.keyId = keyId;

	uint32 modifiedValue = value;
	if(objectInstance.dwType & DIDFT_AXIS)
	{
		tgt.keyType = BINDINGTARGET::KEYTYPE::AXIS;
		modifiedValue = value >> 8;
	}
	else if(objectInstance.dwType & DIDFT_BUTTON)
	{
		tgt.keyType = BINDINGTARGET::KEYTYPE::BUTTON;
	}
	else if(objectInstance.dwType & DIDFT_POV)
	{
		tgt.keyType = BINDINGTARGET::KEYTYPE::POVHAT;
		if(value == -1)
		{
			modifiedValue = BINDINGTARGET::POVHAT_MAX;
		}
		else
		{
			modifiedValue = value / 4500;
		}
	}
	else
	{
		return;
	}

	OnInput(tgt, modifiedValue);
}
