#include <cassert>
#include <cstring>
#include <stdexcept>
#include "static_loop.h"
#include "string_format.h"
#include "../Log.h"
#include "../Ps2Const.h"
#include "../states/RegisterStateFile.h"
#include "../states/MemoryStateFile.h"
#include "Vpu.h"
#include "Vif.h"
#include "INTC.h"

#define LOG_NAME ("ee_vif")

#define STATE_PATH_REGS_FORMAT ("vpu/vif_%d.xml")
#define STATE_PATH_FIFO_FORMAT ("vpu/vif_%d_fifo")

#define STATE_REGS_STAT ("STAT")
#define STATE_REGS_ERR ("ERR")
#define STATE_REGS_CODE ("CODE")
#define STATE_REGS_CYCLE ("CYCLE")
#define STATE_REGS_NUM ("NUM")
#define STATE_REGS_MASK ("MASK")
#define STATE_REGS_MODE ("MODE")
#define STATE_REGS_ROW0 ("ROW0")
#define STATE_REGS_ROW1 ("ROW1")
#define STATE_REGS_ROW2 ("ROW2")
#define STATE_REGS_ROW3 ("ROW3")
#define STATE_REGS_COL0 ("COL0")
#define STATE_REGS_COL1 ("COL1")
#define STATE_REGS_COL2 ("COL2")
#define STATE_REGS_COL3 ("COL3")
#define STATE_REGS_MARK ("MARK")
#define STATE_REGS_ITOP ("ITOP")
#define STATE_REGS_ITOPS ("ITOPS")
#define STATE_REGS_READTICK ("readTick")
#define STATE_REGS_WRITETICK ("writeTick")
#define STATE_REGS_PENDINGMICROPROGRAM ("pendingMicroProgram")
#define STATE_REGS_FIFOINDEX ("fifoIndex")
#define STATE_REGS_INCOMINGFIFODELAY ("incomingFifoDelay")
#define STATE_REGS_INTERRUPTDELAYTICKS ("interruptDelayTicks")
#define STATE_REGS_STREAM_BUFFER ("streamBuffer")
#define STATE_REGS_STREAM_BUFFERPOSITION ("streamBufferPosition")

CVif::CVif(unsigned int number, CVpu& vpu, CINTC& intc, uint8* ram, uint8* spr)
    : m_number(number)
    , m_ram(ram)
    , m_spr(spr)
    , m_intc(intc)
    , m_stream(ram, spr)
    , m_vpu(vpu)
    , m_vifProfilerZone(CProfiler::GetInstance().RegisterZone(string_format("VIF%d", number).c_str()))
{
	static_loop<int, MAX_UNPACKERS>(
	    [this](auto i) {
		    constexpr uint8 dataType = (i & 0x0F);
		    constexpr bool clGreaterEqualWl = (i & 0x10) ? true : false;
		    constexpr bool useMask = (i & 0x20) ? true : false;
		    constexpr uint8 mode = (i & 0xC0) >> 6;
		    constexpr uint8 usn = (i & 0x100) >> 8;
		    m_unpacker[i] = &CVif::Unpack<dataType, clGreaterEqualWl, useMask, mode, usn>;
	    });
}

void CVif::Reset()
{
	memset(&m_STAT, 0, sizeof(STAT));
	memset(&m_CODE, 0, sizeof(CODE));
	memset(&m_CYCLE, 0, sizeof(CYCLE));
	memset(&m_R, 0, sizeof(m_R));
	memset(&m_C, 0, sizeof(m_C));
	memset(&m_fifoBuffer, 0, sizeof(m_fifoBuffer));
	m_CYCLE.nCL = 1;
	m_CYCLE.nWL = 1;
	m_fifoIndex = 0;
	m_MODE = 0;
	m_NUM = 0;
	m_MASK = 0;
	m_MARK = 0;
	m_ITOP = 0;
	m_ITOPS = 0;
	m_readTick = 0;
	m_writeTick = 0;
	m_stream.Reset();
	m_pendingMicroProgram = -1;
	m_incomingFifoDelay = 0;
	m_interruptDelayTicks = 0;
}

uint32 CVif::GetRegister(uint32 address)
{
	uint32 result = 0;
	switch(address)
	{
	case VIF0_STAT:
	case VIF1_STAT:
		if((m_STAT.nVEW != 0) && (m_CODE.nCMD == CODE_CMD_FLUSHE))
		{
			//We have a pending FLUSHE command that's waiting for the microprogram's end.
			//Check if it's finished and act as if the command was executed properly.
			//Some games will keep reading this register in a loop to verify the state of the VEW bit.
			//- Red Faction
			//- RPG Maker 3
			if(!m_vpu.IsVuRunning())
			{
				m_STAT.nVEW = 0;
			}
		}
		result = m_STAT;
		if(m_STAT.nFDR != 0)
		{
			//When FDR is set, it usually means the game is trying to
			//read data from GS and that FIFO has some data in it
			//Set FQC (amount of quadwords in FIFO) to something to
			//let the game know the transfer is being executed
			//Games sensitive to this behavior:
			//- Serious Sam: Takes screen shots at checkpoints, waits for FQC to become 0
			//- PS2PSXe: Takes a screen shot of what is currently on screen, waits for FQC to become 0
			//- There are games that will check that it's not zero once FDR is set.
			result |= (m_incomingFifoDelay << 24);
			if(m_incomingFifoDelay != 0)
			{
				m_incomingFifoDelay--;
			}
		}
		if(m_STAT.nVIS)
		{
			//If we're not idle here, something might be wrong
			assert(m_STAT.nVPS == STAT_VPS_IDLE);
			//If we're stalled because of an interrupt, report that VIF is decoding
			result &= ~STAT_VPS_MASK;
			result |= STAT_VPS_DECODING;
		}
		break;
	case VIF0_ERR:
	case VIF1_ERR:
		result = m_ERR;
		break;
	case VIF0_MARK:
	case VIF1_MARK:
		result = m_MARK;
		break;
	case VIF0_CYCLE:
	case VIF1_CYCLE:
		result = m_CYCLE;
		break;
	case VIF0_MODE:
	case VIF1_MODE:
		result = m_MODE;
		break;
	case VIF0_NUM:
	case VIF1_NUM:
		result = m_NUM;
		break;
	case VIF0_MASK:
	case VIF1_MASK:
		result = m_MASK;
		break;
	case VIF0_CODE:
	case VIF1_CODE:
		result = m_CODE;
		break;
	case VIF0_R0:
	case VIF1_R0:
		result = m_R[0];
		break;
	case VIF0_R1:
	case VIF1_R1:
		result = m_R[1];
		break;
	case VIF0_R2:
	case VIF1_R2:
		result = m_R[2];
		break;
	case VIF0_R3:
	case VIF1_R3:
		result = m_R[3];
		break;
	default:
		CLog::GetInstance().Warn(LOG_NAME, "Reading unknown register 0x%08X.\r\n", address);
		break;
	}
#ifdef _DEBUG
	DisassembleGet(address);
#endif
	return result;
}

void CVif::SetRegister(uint32 address, uint32 value)
{
	if(
	    (address >= VIF0_FIFO_START && address < VIF0_FIFO_END) ||
	    (address >= VIF1_FIFO_START && address < VIF1_FIFO_END))
	{
		ProcessFifoWrite(address, value);
	}
	else
	{
		switch(address)
		{
		case VIF1_STAT:
			m_STAT.nFDR = ((value & STAT_FDR) != 0) ? 1 : 0;
			if(m_STAT.nFDR)
			{
				m_incomingFifoDelay = 0x1F;
			}
			break;
		case VIF0_FBRST:
		case VIF1_FBRST:
			if(value & FBRST_RST)
			{
				//TODO: Reset FIFO
				m_CODE <<= 0;
				m_STAT <<= 0;
				m_NUM = 0;
			}
			if(value & FBRST_FBK || value & FBRST_STP)
			{
				// TODO: We need to properly handle this!
				// But I lack games which leverage it.
				assert(0);
			}
			if(value & FBRST_STC)
			{
				m_STAT.nVSS = 0;
				m_STAT.nVFS = 0;
				m_STAT.nVIS = 0;
				m_STAT.nINT = 0;
				m_STAT.nER0 = 0;
				m_STAT.nER1 = 0;
			}
			break;
		case VIF0_ERR:
		case VIF1_ERR:
			m_ERR <<= value;
			break;
		case VIF0_MARK:
		case VIF1_MARK:
			m_MARK = value;
			m_STAT.nMRK = 0;
			break;
		default:
			CLog::GetInstance().Warn(LOG_NAME, "Writing unknown register 0x%08X, 0x%08X.\r\n", address, value);
			break;
		}
	}
#ifdef _DEBUG
	DisassembleSet(address, value);
#endif
}

void CVif::CountTicks(uint32 ticks)
{
	if(m_interruptDelayTicks != 0)
	{
		m_interruptDelayTicks -= ticks;
		if(m_interruptDelayTicks <= 0)
		{
			assert(m_STAT.nINT);
			m_intc.AssertLine(CINTC::INTC_LINE_VIF0 + m_number);
			m_interruptDelayTicks = 0;
		}
	}
}

void CVif::SaveState(Framework::CZipArchiveWriter& archive)
{
	{
		auto path = string_format(STATE_PATH_REGS_FORMAT, m_number);
		auto registerFile = new CRegisterStateFile(path.c_str());
		registerFile->SetRegister32(STATE_REGS_STAT, m_STAT);
		registerFile->SetRegister32(STATE_REGS_ERR, m_ERR);
		registerFile->SetRegister32(STATE_REGS_CODE, m_CODE);
		registerFile->SetRegister32(STATE_REGS_CYCLE, m_CYCLE);
		registerFile->SetRegister32(STATE_REGS_NUM, m_NUM);
		registerFile->SetRegister32(STATE_REGS_MODE, m_MODE);
		registerFile->SetRegister32(STATE_REGS_MASK, m_MASK);
		registerFile->SetRegister32(STATE_REGS_MARK, m_MARK);
		registerFile->SetRegister32(STATE_REGS_ROW0, m_R[0]);
		registerFile->SetRegister32(STATE_REGS_ROW1, m_R[1]);
		registerFile->SetRegister32(STATE_REGS_ROW2, m_R[2]);
		registerFile->SetRegister32(STATE_REGS_ROW3, m_R[3]);
		registerFile->SetRegister32(STATE_REGS_COL0, m_C[0]);
		registerFile->SetRegister32(STATE_REGS_COL1, m_C[1]);
		registerFile->SetRegister32(STATE_REGS_COL2, m_C[2]);
		registerFile->SetRegister32(STATE_REGS_COL3, m_C[3]);
		registerFile->SetRegister32(STATE_REGS_ITOP, m_ITOP);
		registerFile->SetRegister32(STATE_REGS_ITOPS, m_ITOPS);
		registerFile->SetRegister32(STATE_REGS_READTICK, m_readTick);
		registerFile->SetRegister32(STATE_REGS_WRITETICK, m_writeTick);
		registerFile->SetRegister32(STATE_REGS_PENDINGMICROPROGRAM, m_pendingMicroProgram);
		registerFile->SetRegister32(STATE_REGS_FIFOINDEX, m_fifoIndex);
		registerFile->SetRegister32(STATE_REGS_INCOMINGFIFODELAY, m_incomingFifoDelay);
		registerFile->SetRegister32(STATE_REGS_INTERRUPTDELAYTICKS, m_interruptDelayTicks);
		registerFile->SetRegister128(STATE_REGS_STREAM_BUFFER, m_stream.GetBuffer());
		registerFile->SetRegister32(STATE_REGS_STREAM_BUFFERPOSITION, m_stream.GetBufferPosition());
		archive.InsertFile(registerFile);
	}
	{
		auto path = string_format(STATE_PATH_FIFO_FORMAT, m_number);
		archive.InsertFile(new CMemoryStateFile(path.c_str(), &m_fifoBuffer, sizeof(m_fifoBuffer)));
	}
}

void CVif::LoadState(Framework::CZipArchiveReader& archive)
{
	{
		auto path = string_format(STATE_PATH_REGS_FORMAT, m_number);
		CRegisterStateFile registerFile(*archive.BeginReadFile(path.c_str()));
		m_STAT <<= registerFile.GetRegister32(STATE_REGS_STAT);
		m_ERR <<= registerFile.GetRegister32(STATE_REGS_ERR);
		m_CODE <<= registerFile.GetRegister32(STATE_REGS_CODE);
		m_CYCLE <<= registerFile.GetRegister32(STATE_REGS_CYCLE);
		m_NUM = static_cast<uint8>(registerFile.GetRegister32(STATE_REGS_NUM));
		m_MODE = registerFile.GetRegister32(STATE_REGS_MODE);
		m_MASK = registerFile.GetRegister32(STATE_REGS_MASK);
		m_MARK = registerFile.GetRegister32(STATE_REGS_MARK);
		m_R[0] = registerFile.GetRegister32(STATE_REGS_ROW0);
		m_R[1] = registerFile.GetRegister32(STATE_REGS_ROW1);
		m_R[2] = registerFile.GetRegister32(STATE_REGS_ROW2);
		m_R[3] = registerFile.GetRegister32(STATE_REGS_ROW3);
		m_C[0] = registerFile.GetRegister32(STATE_REGS_COL0);
		m_C[1] = registerFile.GetRegister32(STATE_REGS_COL1);
		m_C[2] = registerFile.GetRegister32(STATE_REGS_COL2);
		m_C[3] = registerFile.GetRegister32(STATE_REGS_COL3);
		m_ITOP = registerFile.GetRegister32(STATE_REGS_ITOP);
		m_ITOPS = registerFile.GetRegister32(STATE_REGS_ITOPS);
		m_readTick = registerFile.GetRegister32(STATE_REGS_READTICK);
		m_writeTick = registerFile.GetRegister32(STATE_REGS_WRITETICK);
		m_pendingMicroProgram = registerFile.GetRegister32(STATE_REGS_PENDINGMICROPROGRAM);
		m_fifoIndex = registerFile.GetRegister32(STATE_REGS_FIFOINDEX);
		m_incomingFifoDelay = registerFile.GetRegister32(STATE_REGS_INCOMINGFIFODELAY);
		m_interruptDelayTicks = registerFile.GetRegister32(STATE_REGS_INTERRUPTDELAYTICKS);
		m_stream.SetBuffer(registerFile.GetRegister128(STATE_REGS_STREAM_BUFFER));
		m_stream.SetBufferPosition(registerFile.GetRegister32(STATE_REGS_STREAM_BUFFERPOSITION));
	}
	{
		auto path = string_format(STATE_PATH_FIFO_FORMAT, m_number);
		archive.BeginReadFile(path.c_str())->Read(&m_fifoBuffer, sizeof(m_fifoBuffer));
	}
}

uint32 CVif::GetTOP() const
{
	throw std::exception();
}

uint32 CVif::GetITOP() const
{
	return m_ITOP;
}

uint32 CVif::ReceiveDMA(uint32 address, uint32 qwc, uint32 unused, bool tagIncluded)
{
	if(m_STAT.nVEW && m_vpu.IsVuRunning())
	{
		//Is waiting for program end, don't bother
		return 0;
	}

#ifdef PROFILE
	CProfilerZone profilerZone(m_vifProfilerZone);
#endif

#ifdef _DEBUG
	CLog::GetInstance().Print(LOG_NAME, "vif%i : Processing packet @ 0x%08X, qwc = 0x%X, tagIncluded = %i\r\n",
	                          m_number, address, qwc, static_cast<int>(tagIncluded));
#endif

	m_stream.SetDmaParams(address, qwc * 0x10, tagIncluded);

	ProcessPacket(m_stream);

	uint32 remainingSize = m_stream.GetRemainingDmaTransferSize();
	assert((remainingSize & 0x0F) == 0);
	remainingSize /= 0x10;

	return qwc - remainingSize;
}

bool CVif::IsWaitingForProgramEnd() const
{
	return (m_STAT.nVEW != 0);
}

void CVif::ProcessFifoWrite(uint32 address, uint32 value)
{
	assert(m_fifoIndex != FIFO_SIZE);
	if(m_fifoIndex == FIFO_SIZE)
	{
		return;
	}
	uint32 index = (address & 0xF) / 4;
	*reinterpret_cast<uint32*>(m_fifoBuffer + m_fifoIndex + index * 4) = value;
	if(index == 3)
	{
		m_fifoIndex += 0x10;
		m_stream.SetFifoParams(m_fifoBuffer, m_fifoIndex);
		ProcessPacket(m_stream);
		uint32 newIndex = m_stream.GetRemainingDmaTransferSize();
		uint32 discardSize = m_fifoIndex - newIndex;
		memmove(m_fifoBuffer, m_fifoBuffer + discardSize, newIndex);
		m_fifoIndex = newIndex;
	}
}

void CVif::ProcessPacket(StreamType& stream)
{
	while(stream.GetAvailableReadBytes())
	{
		if(m_STAT.nVPS == 1)
		{
			//Command is waiting for more data...
			ExecuteCommand(stream, m_CODE);

			if((m_STAT.nVPS == 1) && (stream.GetAvailableReadBytes() != 0))
			{
				//We have data in our FIFO but we still need more than what's available
				break;
			}
			else
			{
				continue;
			}
		}
		if(m_STAT.nVEW == 1)
		{
			if(m_vpu.IsVuRunning()) break;
			m_STAT.nVEW = 0;
			//Command is waiting for micro-program to end.
			ExecuteCommand(stream, m_CODE);
			continue;
		}

		if(m_STAT.nVIS)
		{
			break;
		}

		stream.Read(&m_CODE, sizeof(CODE));

		if(m_CODE.nI != 0)
		{
			//Next command will be stalled (if not MARK)
			if(m_CODE.nCMD != CODE_CMD_MARK)
			{
				m_STAT.nVIS = 1;
			}
			m_STAT.nINT = 1;
			m_interruptDelayTicks = 0x1000;
		}

		switch(m_CODE.nCMD)
		{
		case CODE_CMD_STMASK:
			m_NUM = 1;
			break;
		case CODE_CMD_STROW:
		case CODE_CMD_STCOL:
			m_NUM = 4;
			break;
		default:
			m_NUM = m_CODE.nNUM;
			break;
		}

		ExecuteCommand(stream, m_CODE);
	}

	if(stream.GetAvailableReadBytes() == 0)
	{
		ResumeDelayedMicroProgram();
	}
}

void CVif::ExecuteCommand(StreamType& stream, CODE nCommand)
{
#ifdef _DEBUG
	if(m_number == 0)
	{
		DisassembleCommand(nCommand);
	}
#endif
	if(nCommand.nCMD >= 0x60)
	{
		Cmd_UNPACK(stream, nCommand, (nCommand.nIMM & 0x03FF));
		return;
	}
	switch(nCommand.nCMD)
	{
	case 0:
		//NOP
		break;
	case 0x01:
		//STCYCL
		m_CYCLE <<= nCommand.nIMM;
		break;
	case 0x04:
		//ITOP
		if(ResumeDelayedMicroProgram())
		{
			m_STAT.nVEW = 1;
			return;
		}
		m_ITOPS = nCommand.nIMM & 0x3FF;
		break;
	case 0x05:
		//STMOD
		m_MODE = nCommand.nIMM & 0x03;
		break;
	case CODE_CMD_MARK:
		m_MARK = nCommand.nIMM;
		m_STAT.nMRK = 1;
		break;
	case CODE_CMD_FLUSHE:
		if(m_vpu.IsVuRunning())
		{
			m_STAT.nVEW = 1;
		}
		else
		{
			m_STAT.nVEW = 0;
		}
		if(ResumeDelayedMicroProgram())
		{
			m_STAT.nVEW = 1;
			return;
		}
		break;
	case 0x14:
		//MSCAL
		if(ResumeDelayedMicroProgram())
		{
			m_STAT.nVEW = 1;
			return;
		}
		StartDelayedMicroProgram(nCommand.nIMM * 8);
		break;
	case 0x15:
		//MSCALF
		//TODO: Wait for GIF PATH 1 and 2 transfers to be over
		if(ResumeDelayedMicroProgram())
		{
			m_STAT.nVEW = 1;
			return;
		}
		StartMicroProgram(nCommand.nIMM * 8);
		break;
	case 0x17:
		//MSCNT
		if(ResumeDelayedMicroProgram())
		{
			m_STAT.nVEW = 1;
			return;
		}
		StartMicroProgram(m_vpu.GetContext().m_State.nPC);
		break;
	case CODE_CMD_STMASK:
		Cmd_STMASK(stream, nCommand);
		break;
	case CODE_CMD_STROW:
		Cmd_STROW(stream, nCommand);
		break;
	case CODE_CMD_STCOL:
		Cmd_STCOL(stream, nCommand);
		break;
	case 0x4A:
		//MPG
		Cmd_MPG(stream, nCommand);
		break;
	default:
		CLog::GetInstance().Warn(LOG_NAME, "Executed invalid command %d.\r\n", nCommand.nCMD);
		m_STAT.nER1 = 1;
		break;
	}
}

void CVif::Cmd_MPG(StreamType& stream, CODE nCommand)
{
	uint32 nSize = stream.GetAvailableReadBytes();

	uint32 nNum = (m_NUM == 0) ? (256) : (m_NUM);
	uint32 nCodeNum = (m_CODE.nNUM == 0) ? (256) : (m_CODE.nNUM);
	uint32 nTransfered = (nCodeNum - nNum) * 8;

	nCodeNum *= 8;
	nNum *= 8;

	nSize = std::min<uint32>(nNum, nSize);

	uint32 nDstAddr = (m_CODE.nIMM * 8) + nTransfered;
	nDstAddr &= (m_vpu.GetMicroMemorySize() - 1);

	//Check if microprogram is running
	if(m_vpu.IsVuRunning())
	{
		m_STAT.nVEW = 1;
		return;
	}

	if(nSize != 0)
	{
		auto microMem = m_vpu.GetMicroMemory();
		auto copyToMicroMem =
		    [&](const uint8* microProgramPtr, uint32 start, uint32 size) {
			    //Check if there's a change
			    if(memcmp(microMem + start, microProgramPtr, size) != 0)
			    {
				    m_vpu.InvalidateMicroProgram(start, start + size);
				    memcpy(microMem + start, microProgramPtr, size);
			    }
		    };

		uint8* microProgram = reinterpret_cast<uint8*>(alloca(nSize));
		stream.Read(microProgram, nSize);

		assert(nSize <= m_vpu.GetMicroMemorySize());

		//Check if the copy's destination address will wrap around
		if((nDstAddr + nSize) > m_vpu.GetMicroMemorySize())
		{
			uint32 start1 = nDstAddr;
			uint32 size1 = m_vpu.GetMicroMemorySize() - nDstAddr;

			uint32 start2 = 0;
			uint32 size2 = nSize - size1;

			copyToMicroMem(microProgram, start1, size1);
			copyToMicroMem(microProgram + size1, start2, size2);
		}
		else
		{
			copyToMicroMem(microProgram, nDstAddr, nSize);
		}
	}

	m_NUM -= static_cast<uint8>(nSize / 8);
	if((m_NUM == 0) && (nSize != 0))
	{
		m_STAT.nVPS = 0;
	}
	else
	{
		m_STAT.nVPS = 1;
	}
}

void CVif::Cmd_STROW(StreamType& stream, CODE nCommand)
{
	while(m_NUM != 0 && stream.GetAvailableReadBytes())
	{
		assert(m_NUM <= 4);
		stream.Read(&m_R[4 - m_NUM], 4);
		m_NUM--;
	}

	if(m_NUM == 0)
	{
		m_STAT.nVPS = 0;
	}
	else
	{
		m_STAT.nVPS = 1;
	}
}

void CVif::Cmd_STCOL(StreamType& stream, CODE nCommand)
{
	while(m_NUM != 0 && stream.GetAvailableReadBytes())
	{
		assert(m_NUM <= 4);
		stream.Read(&m_C[4 - m_NUM], 4);
		m_NUM--;
	}

	if(m_NUM == 0)
	{
		m_STAT.nVPS = 0;
	}
	else
	{
		m_STAT.nVPS = 1;
	}
}

void CVif::Cmd_STMASK(StreamType& stream, CODE command)
{
	while(m_NUM != 0 && stream.GetAvailableReadBytes())
	{
		stream.Read(&m_MASK, 4);
		m_NUM--;
	}

	if(m_NUM == 0)
	{
		m_STAT.nVPS = 0;
	}
	else
	{
		m_STAT.nVPS = 1;
	}
}

void CVif::Cmd_UNPACK(StreamType& stream, CODE nCommand, uint32 nDstAddr)
{
	uint32 cl = m_CYCLE.nCL;
	uint32 wl = m_CYCLE.nWL;
	if(wl == 0)
	{
		wl = UINT_MAX;
		cl = 0;
	}
	bool clGreaterEqualWl = (cl >= wl);
	bool useMask = (nCommand.nCMD & 0x10) != 0;
	bool usn = (m_CODE.nIMM & 0x4000) != 0;
	uint8 mode = m_MODE & 0x3;
	auto unpackFct = m_unpacker[(nCommand.nCMD & 0x0F) | ((clGreaterEqualWl ? 1 : 0) << 4) | ((useMask ? 1 : 0) << 5) | (mode << 6) | (usn << 8)];
	((*this).*(unpackFct))(stream, nCommand, nDstAddr);
}

bool CVif::Unpack_S32(StreamType& stream, uint128& result)
{
	if(stream.GetAvailableReadBytes() < 4) return false;

	uint32 word = 0;
	stream.Read(&word, 4);

	for(unsigned int i = 0; i < 4; i++)
	{
		result.nV[i] = word;
	}

	return true;
}

bool CVif::Unpack_S16(StreamType& stream, uint128& result, bool zeroExtend)
{
	if(stream.GetAvailableReadBytes() < 2) return false;

	uint32 temp = 0;
	stream.Read(&temp, 2);
	if(!zeroExtend)
	{
		temp = static_cast<int16>(temp);
	}

	for(unsigned int i = 0; i < 4; i++)
	{
		result.nV[i] = temp;
	}

	return true;
}

bool CVif::Unpack_S8(StreamType& stream, uint128& result, bool zeroExtend)
{
	if(stream.GetAvailableReadBytes() < 1) return false;

	uint32 temp = 0;
	stream.Read(&temp, 1);
	if(!zeroExtend)
	{
		temp = static_cast<int8>(temp);
	}

	for(unsigned int i = 0; i < 4; i++)
	{
		result.nV[i] = temp;
	}

	return true;
}

bool CVif::Unpack_V45(StreamType& stream, uint128& result)
{
	if(stream.GetAvailableReadBytes() < 2) return false;

	uint16 nColor = 0;
	stream.Read(&nColor, 2);

	result.nV0 = ((nColor >> 0) & 0x1F) << 3;
	result.nV1 = ((nColor >> 5) & 0x1F) << 3;
	result.nV2 = ((nColor >> 10) & 0x1F) << 3;
	result.nV3 = ((nColor >> 15) & 0x01) << 7;

	return true;
}

uint32 CVif::GetMaskOp(unsigned int row, unsigned int col) const
{
	if(col > 3) col = 3;
	assert(row < 4);
	unsigned int index = (col * 4) + row;
	return (m_MASK >> (index * 2)) & 0x03;
}

void CVif::PrepareMicroProgram()
{
	m_ITOP = m_ITOPS;
}

void CVif::StartMicroProgram(uint32 address)
{
	if(m_vpu.IsVuRunning())
	{
		m_STAT.nVEW = 1;
		return;
	}

	assert(!m_STAT.nVEW);
	PrepareMicroProgram();
	m_vpu.ExecuteMicroProgram(address);
}

void CVif::StartDelayedMicroProgram(uint32 address)
{
	//Snowblind Studio games start a VU microprogram and issues an UNPACK command
	//which has data needed by the microprogram. We simulate the microprogram
	//starting a bit later to let the UNPACK command execute
	if(m_vpu.IsVuRunning())
	{
		m_STAT.nVEW = 1;
		return;
	}

	assert(!m_STAT.nVEW);
	PrepareMicroProgram();
	m_pendingMicroProgram = address;
}

bool CVif::ResumeDelayedMicroProgram()
{
	if(m_pendingMicroProgram != -1)
	{
		assert(!IsWaitingForProgramEnd());
		assert(!m_vpu.IsVuRunning());
		m_vpu.ExecuteMicroProgram(m_pendingMicroProgram);
		m_pendingMicroProgram = -1;
		return true;
	}
	else
	{
		return false;
	}
}

void CVif::DisassembleGet(uint32 address)
{
#define LOG_GET(registerId)                                            \
	case registerId:                                                   \
		CLog::GetInstance().Print(LOG_NAME, "= " #registerId ".\r\n"); \
		break;

	switch(address)
	{
		LOG_GET(VIF0_STAT)
		LOG_GET(VIF0_ERR)
		LOG_GET(VIF0_MARK)
		LOG_GET(VIF0_CYCLE)
		LOG_GET(VIF0_MODE)
		LOG_GET(VIF0_NUM)
		LOG_GET(VIF0_MASK)
		LOG_GET(VIF0_CODE)
		LOG_GET(VIF0_R0)
		LOG_GET(VIF0_R1)
		LOG_GET(VIF0_R2)
		LOG_GET(VIF0_R3)

		LOG_GET(VIF1_STAT)
		LOG_GET(VIF1_ERR)
		LOG_GET(VIF1_MARK)
		LOG_GET(VIF1_CYCLE)
		LOG_GET(VIF1_MODE)
		LOG_GET(VIF1_NUM)
		LOG_GET(VIF1_MASK)
		LOG_GET(VIF1_CODE)
		LOG_GET(VIF1_R0)
		LOG_GET(VIF1_R1)
		LOG_GET(VIF1_R2)
		LOG_GET(VIF1_R3)

	default:
		CLog::GetInstance().Print(LOG_NAME, "Reading unknown register 0x%08X.\r\n", address);
		break;
	}

#undef LOG_GET
}

void CVif::DisassembleSet(uint32 address, uint32 value)
{
	if((address >= VIF0_FIFO_START) && (address < VIF0_FIFO_END))
	{
		CLog::GetInstance().Print(LOG_NAME, "VIF0_FIFO(0x%03X) = 0x%08X.\r\n", address & 0xFFF, value);
	}
	else if((address >= VIF1_FIFO_START) && (address < VIF1_FIFO_END))
	{
		CLog::GetInstance().Print(LOG_NAME, "VIF1_FIFO(0x%03X) = 0x%08X.\r\n", address & 0xFFF, value);
	}
	else
	{
#define LOG_SET(registerId)                                                       \
	case registerId:                                                              \
		CLog::GetInstance().Print(LOG_NAME, #registerId " = 0x%08X.\r\n", value); \
		break;

		switch(address)
		{
			LOG_SET(VIF0_FBRST)
			LOG_SET(VIF0_MARK)
			LOG_SET(VIF0_ERR)

			LOG_SET(VIF1_FBRST)
			LOG_SET(VIF1_MARK)
			LOG_SET(VIF1_ERR)

		default:
			CLog::GetInstance().Print(LOG_NAME, "Writing unknown register 0x%08X, 0x%08X.\r\n", address, value);
			break;
		}

#undef LOG_SET
	}
}

void CVif::DisassembleCommand(CODE code)
{
	if(m_STAT.nVPS != 0) return;

	CLog::GetInstance().Print(LOG_NAME, "vif%i : ", m_number);

	if(code.nI)
	{
		CLog::GetInstance().Print(LOG_NAME, "(I) ");
	}

	if(code.nCMD >= 0x60)
	{
		static const char* packFormats[16] =
		    {
		        "S-32",
		        "S-16",
		        "S-8",
		        "(Unknown)",
		        "V2-32",
		        "V2-16",
		        "V2-8",
		        "(Unknown)",
		        "V3-32",
		        "V3-16",
		        "V3-8",
		        "(Unknown)",
		        "V4-32",
		        "V4-16",
		        "V4-8",
		        "V4-5"};
		CLog::GetInstance().Print(LOG_NAME, "UNPACK(format = %s, imm = 0x%x, num = 0x%x);\r\n",
		                          packFormats[code.nCMD & 0x0F], code.nIMM, code.nNUM);
	}
	else
	{
		switch(code.nCMD)
		{
		case 0x00:
			CLog::GetInstance().Print(LOG_NAME, "NOP\r\n");
			break;
		case 0x01:
			CLog::GetInstance().Print(LOG_NAME, "STCYCL(imm = 0x%x);\r\n", code.nIMM);
			break;
		case 0x02:
			CLog::GetInstance().Print(LOG_NAME, "OFFSET(imm = 0x%x);\r\n", code.nIMM);
			break;
		case 0x03:
			CLog::GetInstance().Print(LOG_NAME, "BASE(imm = 0x%x);\r\n", code.nIMM);
			break;
		case 0x04:
			CLog::GetInstance().Print(LOG_NAME, "ITOP(imm = 0x%x);\r\n", code.nIMM);
			break;
		case 0x05:
			CLog::GetInstance().Print(LOG_NAME, "STMOD(imm = 0x%x);\r\n", code.nIMM);
			break;
		case 0x06:
			CLog::GetInstance().Print(LOG_NAME, "MSKPATH3(mask = %d);\r\n", (code.nIMM & 0x8000) ? 1 : 0);
			break;
		case CODE_CMD_MARK:
			CLog::GetInstance().Print(LOG_NAME, "MARK(imm = 0x%x);\r\n", code.nIMM);
			break;
		case CODE_CMD_FLUSHE:
			CLog::GetInstance().Print(LOG_NAME, "FLUSHE();\r\n");
			break;
		case 0x11:
			CLog::GetInstance().Print(LOG_NAME, "FLUSH();\r\n");
			break;
		case 0x13:
			CLog::GetInstance().Print(LOG_NAME, "FLUSHA();\r\n");
			break;
		case 0x14:
			CLog::GetInstance().Print(LOG_NAME, "MSCAL(imm = 0x%x);\r\n", code.nIMM);
			break;
		case 0x15:
			CLog::GetInstance().Print(LOG_NAME, "MSCALF(imm = 0x%x);\r\n", code.nIMM);
			break;
		case 0x17:
			CLog::GetInstance().Print(LOG_NAME, "MSCNT();\r\n");
			break;
		case CODE_CMD_STMASK:
			CLog::GetInstance().Print(LOG_NAME, "STMASK();\r\n");
			break;
		case CODE_CMD_STROW:
			CLog::GetInstance().Print(LOG_NAME, "STROW();\r\n");
			break;
		case CODE_CMD_STCOL:
			CLog::GetInstance().Print(LOG_NAME, "STCOL();\r\n");
			break;
		case 0x4A:
			CLog::GetInstance().Print(LOG_NAME, "MPG(imm = 0x%x, num = 0x%x);\r\n", code.nIMM, code.nNUM);
			break;
		case 0x50:
			CLog::GetInstance().Print(LOG_NAME, "DIRECT(imm = 0x%x);\r\n", code.nIMM);
			break;
		case 0x51:
			CLog::GetInstance().Print(LOG_NAME, "DIRECTHL(imm = 0x%x);\r\n", code.nIMM);
			break;
		default:
			CLog::GetInstance().Print(LOG_NAME, "Unknown command (0x%x).\r\n", code.nCMD);
			break;
		}
	}
}

//CFifoStream
//--------------------------------------------------

CVif::CFifoStream::CFifoStream(uint8* ram, uint8* spr)
    : m_ram(ram)
    , m_spr(spr)
{
}

void CVif::CFifoStream::Reset()
{
	m_buffer = {};
	m_bufferPosition = BUFFERSIZE;
	m_startAddress = 0;
	m_nextAddress = 0;
	m_endAddress = 0;
	m_tagIncluded = false;
	m_source = nullptr;
}

void CVif::CFifoStream::Flush()
{
	m_bufferPosition = BUFFERSIZE;
}

void CVif::CFifoStream::SetDmaParams(uint32 address, uint32 size, bool tagIncluded)
{
	if(address & 0x80000000)
	{
		m_source = m_spr;
		address &= (PS2::EE_SPR_SIZE - 1);
		assert((address + size) <= PS2::EE_SPR_SIZE);
	}
	else
	{
		m_source = m_ram;
		address &= (PS2::EE_RAM_SIZE - 1);
		assert((address + size) <= PS2::EE_RAM_SIZE);
	}
	m_startAddress = address;
	m_nextAddress = address;
	m_endAddress = address + size;
	m_tagIncluded = tagIncluded;
	SyncBuffer();
}

void CVif::CFifoStream::SetFifoParams(uint8* source, uint32 size)
{
	m_source = source;
	m_startAddress = 0;
	m_nextAddress = 0;
	m_endAddress = size;
	m_tagIncluded = false;
	SyncBuffer();
}

void CVif::CFifoStream::Align32()
{
	unsigned int remainBytes = m_bufferPosition & 0x03;
	if(remainBytes == 0) return;
	uint32 dummy = 0;
	Read(&dummy, 4 - remainBytes);
	assert((m_bufferPosition & 0x03) == 0);
}

uint8* CVif::CFifoStream::GetDirectPointer() const
{
	assert(!m_tagIncluded);
	if(m_bufferPosition == BUFFERSIZE)
	{
		return m_source + m_nextAddress;
	}
	else
	{
		assert((m_nextAddress - m_startAddress) >= 0x10);
		return m_source + m_nextAddress + m_bufferPosition - 0x10;
	}
}

void CVif::CFifoStream::Advance(uint32 size)
{
	assert((size & 0x0F) == 0);
	assert(!m_tagIncluded);
	//If buffer was untouched, we can do as if we read from it directly
	if(m_bufferPosition == 0)
	{
		assert(size >= 0x10);
		size -= 0x10;
		m_bufferPosition = BUFFERSIZE;
	}
	assert((m_nextAddress + size) <= m_endAddress);
	m_nextAddress += size;
	if(m_bufferPosition != BUFFERSIZE)
	{
		//Update buffer
		assert((m_nextAddress - m_startAddress) >= 0x10);
		m_buffer = *reinterpret_cast<uint128*>(&m_source[m_nextAddress - 0x10]);
	}
}

uint128 CVif::CFifoStream::GetBuffer() const
{
	return m_buffer;
}

void CVif::CFifoStream::SetBuffer(uint128 buffer)
{
	m_buffer = buffer;
}

uint32 CVif::CFifoStream::GetBufferPosition() const
{
	return m_bufferPosition;
}

void CVif::CFifoStream::SetBufferPosition(uint32 bufferPosition)
{
	m_bufferPosition = bufferPosition;
}
