#pragma once

#include "Iop_Module.h"

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
		int32 TransferPipe();
		int32 GetDeviceLocation(uint32, uint32);

		CIopBios& m_bios;
		uint8* m_ram = nullptr;
		uint32 m_descriptorMemPtr = 0;
	};
}
