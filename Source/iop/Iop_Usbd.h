#pragma once

#include "Iop_Module.h"
#include "PadInterface.h"

class CIopBios;

namespace Iop
{
	class CUsbd : public CModule, public CPadInterface
	{
	public:
		CUsbd(CIopBios&, uint8*);
		virtual ~CUsbd() = default;

		std::string GetId() const override;
		std::string GetFunctionName(unsigned int) const override;
		void Invoke(CMIPS&, unsigned int) override;

		void CountTicks(uint32);

		//CPadInterface
		void SetButtonState(unsigned int, PS2::CControllerInfo::BUTTON, bool, uint8*);
		void SetAxisState(unsigned int, PS2::CControllerInfo::BUTTON, uint8, uint8*) override{};
		void GetVibration(unsigned int, uint8& largeMotor, uint8& smallMotor) override{};

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

		int32 RegisterLld(uint32);
		int32 ScanStaticDescriptor(uint32, uint32, uint32);
		int32 OpenPipe(uint32, uint32);
		int32 TransferPipe(uint32, uint32, uint32, uint32, uint32, uint32);
		int32 GetDeviceLocation(uint32, uint32);

		CIopBios& m_bios;
		uint8* m_ram = nullptr;
		uint32 m_descriptorMemPtr = 0;
		int32 m_nextTransferTicks = 0;
		uint32 m_transferBufferPtr = 0;
		uint32 m_transferSize = 0;
		uint32 m_transferCb = 0;
		uint32 m_transferCbArg = 0;
		uint8 m_buttonState = 0;
	};

	typedef std::shared_ptr<CUsbd> UsbdPtr;
}
