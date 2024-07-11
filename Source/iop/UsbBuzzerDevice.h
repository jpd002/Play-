#pragma once

#include "UsbDevice.h"
#include "PadInterface.h"

class CIopBios;
class CPadHandler;

namespace Iop
{
	class CBuzzerUsbDevice : public CUsbDevice, public CPadInterface
	{
	public:
		enum
		{
			DEVICE_ID = 0xBEEF,
			CONTROL_PIPE_ID = 0x123,
			PIPE_ID = 0x456,
		};

		CBuzzerUsbDevice(CIopBios&, uint8*);

		void SetPadHandler(CPadHandler*);

		uint16 GetId() const override;
		const char* GetLldName() const override;

		void SaveState(CRegisterState&) const override;
		void LoadState(const CRegisterState&) override;

		void CountTicks(uint32) override;

		void OnLldRegistered() override;
		uint32 ScanStaticDescriptor(uint32, uint32, uint32) override;
		int32 OpenPipe(uint32, uint32) override;
		int32 TransferPipe(uint32, uint32, uint32, uint32, uint32, uint32) override;

		//CPadInterface
		void SetButtonState(unsigned int, PS2::CControllerInfo::BUTTON, bool, uint8*) override;
		void SetAxisState(unsigned int, PS2::CControllerInfo::BUTTON, uint8, uint8*) override{};
		void GetVibration(unsigned int, uint8& largeMotor, uint8& smallMotor) override{};

	private:
		CIopBios& m_bios;
		uint8* m_ram = nullptr;

		CPadHandler* m_padHandler = nullptr;

		uint8 m_buttonState = 0;
		uint32 m_descriptorMemPtr = 0;
		int32 m_nextTransferTicks = 0;
		uint32 m_transferBufferPtr = 0;
		uint32 m_transferSize = 0;
		uint32 m_transferCb = 0;
		uint32 m_transferCbArg = 0;
	};
}
