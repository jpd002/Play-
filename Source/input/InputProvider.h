#pragma once

#include "Types.h"
#include <array>
#include <functional>
#include <string>
#include <cstring>

static const uint32 DeviceIdTypeCount = 6;
typedef std::array<uint32, DeviceIdTypeCount> DeviceIdType;

struct BINDINGTARGET
{
	//AXIS has a value from 0x00 to 0xFF (0x7F is neutral)
	//POVHAT has a value from 0 to 7 (POVHAT_MAX is neutral)
	enum class KEYTYPE
	{
		BUTTON,
		AXIS,
		POVHAT
	};

	enum
	{
		AXIS_MIN = 0,
		AXIS_NEUTRAL = 0x7F,
		AXIS_MAX = 0xFF,
		POVHAT_MAX = 8,
	};

	BINDINGTARGET() = default;
	BINDINGTARGET(uint32 providerId, DeviceIdType deviceId, uint32 keyId, KEYTYPE keyType)
	    : providerId(providerId)
	    , deviceId(deviceId)
	    , keyId(keyId)
	    , keyType(keyType)
	{
	}

	bool IsEqualTo(const BINDINGTARGET& target) const
	{
		return (providerId == target.providerId) &&
		       (keyId == target.keyId) &&
		       (keyType == target.keyType) &&
		       (memcmp(deviceId.data(), target.deviceId.data(), deviceId.size()) == 0);
	}

	bool operator==(const BINDINGTARGET& target) const
	{
		return IsEqualTo(target);
	}

	bool operator!=(const BINDINGTARGET& target) const
	{
		return !IsEqualTo(target);
	}

	uint32 providerId = 0;
	DeviceIdType deviceId = {{0, 0, 0, 0, 0, 0}};
	uint32 keyId = 0;
	KEYTYPE keyType = KEYTYPE::BUTTON;
};

typedef std::function<void(const BINDINGTARGET&, uint32)> InputEventFunction;

class CInputProvider
{
public:
	virtual ~CInputProvider() = default;
	virtual uint32 GetId() const = 0;
	virtual std::string GetTargetDescription(const BINDINGTARGET&) const = 0;

	InputEventFunction OnInput;
};
