#pragma once

#include "PspBios.h"
#include "PspModule.h"

namespace Psp
{
	class CSysMemUserForUser : public CModule
	{
	public:
		CSysMemUserForUser(CBios&, uint8*);

		void Invoke(uint32, CMIPS&) override;
		std::string GetName() const override;

	private:
		uint32 AllocPartitionMemory(uint32, uint32, uint32, uint32, uint32);
		uint32 GetBlockHeadAddr(uint32);

		uint32 AllocMemoryBlock(uint32, uint32, uint32, uint32);
		uint32 GetMemoryBlockAddr(uint32, uint32);

		void SetCompilerVersion(uint32);

		CBios& m_bios;
		uint8* m_ram;
	};
}
