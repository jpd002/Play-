#pragma once

#include <string>
#include "PspModule.h"
#include "PspBios.h"

namespace Psp
{
	class CThreadManForUser : public CModule
	{
	public:
		CThreadManForUser(CBios&, uint8*);

		std::string GetName() const override;
		void Invoke(uint32, CMIPS&) override;

	private:
		uint32 CreateThread(uint32, uint32, uint32, uint32, uint32, uint32);
		uint32 StartThread(uint32, uint32, uint32);
		uint32 ExitThread(uint32);

		uint32 CreateMbx(uint32, uint32, uint32);
		uint32 SendMbx(uint32, uint32);
		uint32 PollMbx(uint32, uint32);

		CBios& m_bios;
		uint8* m_ram = nullptr;
	};
};
