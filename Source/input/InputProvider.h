#pragma once

#include "Types.h"
#include <array>
#include <functional>
#include <string>
#include <cstring>

typedef std::array<uint32, 6> DeviceIdType;

struct BINDINGTARGET
{
	enum class KEYTYPE
	{
		BUTTON,
		AXIS,
		POVHAT
	};

	enum
	{
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
