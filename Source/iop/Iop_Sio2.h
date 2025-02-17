#pragma once

#include "Types.h"
#include "Iop_Intc.h"
#include "../PadInterface.h"
#include <deque>
#include <array>

namespace Iop
{
	class CSio2 : public CPadInterface
	{
	public:
		enum
		{
			ADDR_BEGIN = 0x1F808200,
			ADDR_END = 0x1F8082FF
		};

		CSio2(Iop::CIntc&);
		virtual ~CSio2() = default;

		void Reset();

		void LoadState(Framework::CZipArchiveReader&);
		void SaveState(Framework::CZipArchiveWriter&);

		uint32 ReadRegister(uint32);
		void WriteRegister(uint32, uint32);

		uint32 ReceiveDmaIn(uint8*, uint32, uint32, uint32);
		uint32 ReceiveDmaOut(uint8*, uint32, uint32, uint32);

		void SetButtonState(unsigned int, PS2::CControllerInfo::BUTTON, bool, uint8*) override;
		void SetAxisState(unsigned int, PS2::CControllerInfo::BUTTON, uint8, uint8*) override;
		void GetVibration(unsigned int padId, uint8& largeMotor, uint8& smallMotor) override;

	private:
		enum REGISTERS
		{
			REG_BASE = 0x1F808200,
			REG_BASE_END = 0x1F80823F,

			REG_PORT0_CTRL1 = 0x1F808240,
			REG_PORT0_CTRL2 = 0x1F808244,

			REG_PORT1_CTRL1 = 0x1F808248,
			REG_PORT1_CTRL2 = 0x1F80824C,

			REG_PORT2_CTRL1 = 0x1F808250,
			REG_PORT2_CTRL2 = 0x1F808254,

			REG_PORT3_CTRL1 = 0x1F808258,
			REG_PORT3_CTRL2 = 0x1F80825C,

			REG_DATA_OUT = 0x1F808260,
			REG_DATA_IN = 0x1F808264,

			REG_CTRL = 0x1F808268,
			REG_STAT6C = 0x1F80826C,
		};

		enum
		{
			MAX_REGS = 16,
			MAX_PADS = 2,
			MAX_PORTS = 4
		};

		struct PADSTATE
		{
			bool configMode;
			uint8 mode;
			uint8 pollMask[3];
			uint16 buttonState;
			uint8 analogStickState[4];
			uint8 smallMotor;
			uint8 largeMotor;
		};

		typedef std::deque<uint8> ByteBufferType;

		void ProcessCommand();
		void ProcessController(unsigned int, size_t, uint32, uint32);
		void ProcessMultitap(unsigned int, size_t, uint32, uint32);
		void ProcessMemoryCard(unsigned int, size_t, uint32, uint32);

		void DisassembleRead(uint32, uint32);
		void DisassembleWrite(uint32, uint32);

		Iop::CIntc& m_intc;

		unsigned int m_currentRegIndex;
		uint32 m_regs[MAX_REGS];
		uint32 m_ctrl1[MAX_PORTS];
		uint32 m_ctrl2[MAX_PORTS];
		uint32 m_stat6C;
		ByteBufferType m_inputBuffer;
		ByteBufferType m_outputBuffer;

		std::array<PADSTATE, MAX_PADS> m_padState;
	};
}
