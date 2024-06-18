#pragma once

#include "Types.h"
#include <memory>

class CRegisterState;

namespace Iop
{
	class CUsbDevice
	{
	public:
		virtual ~CUsbDevice() = default;

		virtual uint16 GetId() const = 0;
		virtual const char* GetLldName() const = 0;

		virtual void SaveState(CRegisterState&) const {};
		virtual void LoadState(const CRegisterState&){};

		virtual void CountTicks(uint32) = 0;

		virtual void OnLldRegistered() = 0;
		virtual uint32 ScanStaticDescriptor(uint32, uint32, uint32) = 0;
		virtual int32 OpenPipe(uint32, uint32) = 0;
		virtual int32 TransferPipe(uint32, uint32, uint32, uint32, uint32, uint32) = 0;
	};
	typedef std::unique_ptr<CUsbDevice> UsbDevicePtr;
}
