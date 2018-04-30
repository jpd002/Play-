#pragma once

#include "Iop_Module.h"
#include "Iop_Stdio.h"
#include "Iop_Ioman.h"
#include "Iop_BiosStructs.h"
#include "../OsStructManager.h"
#include "../SifModule.h"

namespace Iop
{
	class CSifMan;

	class CSysmem : public CModule, public CSifModule
	{
	public:
		typedef COsStructManager<MEMORYBLOCK> BlockListType;

		CSysmem(uint8*, uint32, uint32, BlockListType&, CStdio&, CIoman&, CSifMan&);
		virtual ~CSysmem();

		std::string GetId() const override;
		std::string GetFunctionName(unsigned int) const override;
		void Invoke(CMIPS&, unsigned int) override;
		bool Invoke(uint32, uint32*, uint32, uint32*, uint32, uint8*) override;

		uint32 AllocateMemory(uint32, uint32, uint32);
		uint32 FreeMemory(uint32);

	private:
		enum MODULE_ID
		{
			MODULE_ID = 0x80000003
		};

		uint32 SifAllocate(uint32);
		uint32 SifAllocateSystemMemory(uint32, uint32, uint32);
		uint32 SifLoadMemory(uint32, const char*);
		uint32 SifFreeMemory(uint32);

		uint32 QueryMaxFreeMemSize();

		uint8* m_iopRam = nullptr;
		BlockListType& m_blocks;
		uint32 m_memoryBegin;
		uint32 m_memoryEnd;
		uint32 m_memorySize;
		uint32 m_headBlockId;
		CStdio& m_stdio;
		CIoman& m_ioman;
	};

	typedef std::shared_ptr<CSysmem> SysmemPtr;
}
