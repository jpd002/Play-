#pragma once

#include "Iop_Module.h"
#include "UsbDevice.h"

class CIopBios;

namespace Iop
{
	class CUsbd : public CModule
	{
	public:
		CUsbd(CIopBios&, uint8*);
		virtual ~CUsbd() = default;

		std::string GetId() const override;
		std::string GetFunctionName(unsigned int) const override;
		void Invoke(CMIPS&, unsigned int) override;

		void SaveState(Framework::CZipArchiveWriter&) const override;
		void LoadState(Framework::CZipArchiveReader&) override;

		void CountTicks(uint32);

		template <typename DeviceType>
		DeviceType* GetDevice()
		{
			auto devicePairIterator = m_devices.find(DeviceType::DEVICE_ID);
			if(devicePairIterator == std::end(m_devices))
			{
				return nullptr;
			}
			else
			{
				return static_cast<DeviceType*>(devicePairIterator->second.get());
			}
		}

	private:
		struct LLDOPS
		{
			uint32 nextPtr;
			uint32 prevPtr;
			uint32 namePtr;
			uint32 probeFctPtr;
			uint32 connectFctPtr;
			uint32 disconnectFctPtr;
			uint32 reserved[5];
			uint32 gp;
		};

		void RegisterDevice(UsbDevicePtr);

		int32 RegisterLld(uint32);
		int32 ScanStaticDescriptor(uint32, uint32, uint32);
		int32 OpenPipe(uint32, uint32);
		int32 TransferPipe(uint32, uint32, uint32, uint32, uint32, uint32);
		int32 GetDeviceLocation(uint32, uint32);

		CIopBios& m_bios;
		uint8* m_ram = nullptr;
		std::unordered_map<uint16, UsbDevicePtr> m_devices;
		std::vector<uint16> m_activeDeviceIds;
	};

	typedef std::shared_ptr<CUsbd> UsbdPtr;
}
