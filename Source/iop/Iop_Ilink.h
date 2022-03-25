#pragma once

#include "Types.h"
#include "zip/ZipArchiveWriter.h"
#include "zip/ZipArchiveReader.h"

namespace Iop
{
	class CIntc;

	class CIlink
	{
	public:
		CIlink(CIntc&);

		enum
		{
			ADDR_BEGIN = 0x1F808400,
			ADDR_END = 0x1F808554
		};

		enum
		{
			REG_NODEID = 0x1F808400,
			REG_CTRL2 = 0x1F808410,
			REG_PHY_ACCESS = 0x1F808414,
			REG_INTR0 = 0x1F808420,
			REG_INTR0_MASK = 0x1F808424,
			REG_INTR1 = 0x1F808428,
			REG_INTR1_MASK = 0x1F80842C,
			REG_INTR2 = 0x1F808430,
			REG_INTR2_MASK = 0x1F808434,
		};

		enum
		{
			INTR0_PHYRRX = 0x40000000
		};

		enum
		{
			REG_CTRL2_SOK = 0x08,
		};

		void Reset();

		void LoadState(Framework::CZipArchiveReader&);
		void SaveState(Framework::CZipArchiveWriter&);

		uint32 ReadRegister(uint32);
		void WriteRegister(uint32, uint32);

	private:
		void LogRead(uint32);
		void LogWrite(uint32, uint32);

		CIntc& m_intc;

		uint32 m_ctrl2 = 0;
		uint32 m_phyResult;
		uint32 m_intr0;
		uint32 m_intr0Mask;
		uint32 m_intr1;
		uint32 m_intr1Mask;
		uint32 m_intr2;
		uint32 m_intr2Mask;
	};
}
