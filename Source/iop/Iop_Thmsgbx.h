#pragma once

#include "Iop_Module.h"
#include "IopBios.h"

namespace Iop
{
	class CThmsgbx : public CModule
	{
	public:
		CThmsgbx(CIopBios&, uint8*);
		virtual ~CThmsgbx();

		std::string GetId() const override;
		std::string GetFunctionName(unsigned int) const override;
		void Invoke(CMIPS&, unsigned int) override;

	private:
		struct MSGBX
		{
			uint32 attr;
			uint32 options;
		};

		uint32 CreateMbx(const MSGBX*);
		uint32 DeleteMbx(uint32);
		uint32 SendMbx(uint32, uint32);
		uint32 iSendMbx(uint32, uint32);
		uint32 ReceiveMbx(uint32, uint32);
		uint32 PollMbx(uint32, uint32);
		uint32 ReferMbxStatus(uint32, uint32);

		uint8* m_ram = nullptr;
		CIopBios& m_bios;
	};
}
