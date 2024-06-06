#pragma once

#include "Types.h"

namespace Usb
{
	//Details about descriptors here:
	//https://www.beyondlogic.org/usbnutshell/usb5.shtml

	enum DESCRIPTOR_TYPE
	{
		DESCRIPTOR_TYPE_DEVICE = 1,
		DESCRIPTOR_TYPE_CONFIGURATION = 2,
		DESCRIPTOR_TYPE_INTERFACE = 4,
		DESCRIPTOR_TYPE_ENDPOINT = 5,
	};

#pragma pack(push, 1)
	struct DESCRIPTOR_BASE
	{
		uint8 length;
		uint8 descriptorType;
	};

	struct DEVICE_DESCRIPTOR
	{
		DESCRIPTOR_BASE base;
		uint16 usbVersion;
		uint8 deviceClass;
		uint8 deviceSubClass;
		uint8 deviceProtocol;
		uint8 maxPacketSize;
		uint16 vendorId;
		uint16 productId;
		uint16 deviceVersion;
		uint8 manufacturer;
		uint8 product;
		uint8 serialNumber;
		uint8 numConfigurations;
	};

	struct CONFIGURATION_DESCRIPTOR
	{
		DESCRIPTOR_BASE base;
		uint16 totalLength;
		uint8 numInterfaces;
		uint8 configurationValue;
		uint8 configuration;
		uint8 attributes;
		uint8 maxPower;
	};

	struct INTERFACE_DESCRIPTOR
	{
		DESCRIPTOR_BASE base;
		uint8 interfaceNumber;
		uint8 alternateSetting;
		uint8 numEndpoints;
		uint8 interfaceClass;
		uint8 interfaceSubClass;
		uint8 interfaceProtocol;
		uint8 interface;
	};
	static_assert(sizeof(INTERFACE_DESCRIPTOR) == 9);

	struct ENDPOINT_DESCRIPTOR
	{
		DESCRIPTOR_BASE base;
		uint8 endpointAddress;
		uint8 attributes;
		uint16 maxPacketSize;
		uint8 interval;
	};
	static_assert(sizeof(ENDPOINT_DESCRIPTOR) == 7);
#pragma pack(pop)
}
