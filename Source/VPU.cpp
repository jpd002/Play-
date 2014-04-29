#include "VPU.h"
#include "Log.h"
#include "RegisterStateFile.h"
#include <boost/lexical_cast.hpp>
#include "Ps2Const.h"

#define LOG_NAME				("vpu")

#define STATE_PREFIX			("vif/vpu_")
#define STATE_SUFFIX			(".xml")
#define STATE_REGS_STAT			("STAT")
#define STATE_REGS_CODE			("CODE")
#define STATE_REGS_CYCLE		("CYCLE")
#define STATE_REGS_NUM			("NUM")
#define STATE_REGS_MASK			("MASK")
#define STATE_REGS_MODE			("MODE")
#define STATE_REGS_ROW0			("ROW0")
#define STATE_REGS_ROW1			("ROW1")
#define STATE_REGS_ROW2			("ROW2")
#define STATE_REGS_ROW3			("ROW3")
#define STATE_REGS_COL0			("COL0")
#define STATE_REGS_COL1			("COL1")
#define STATE_REGS_COL2			("COL2")
#define STATE_REGS_COL3			("COL3")
#define STATE_REGS_MARK			("MARK")
#define STATE_REGS_ITOP			("ITOP")
#define STATE_REGS_ITOPS		("ITOPS")
#define STATE_REGS_READTICK		("readTick")
#define STATE_REGS_WRITETICK	("writeTick")

CVPU::CVPU(CVIF& vif, unsigned int vpuNumber, const CVIF::VPUINIT& vpuInit)
: m_vif(vif)
, m_vpuNumber(vpuNumber)
, m_microMem(vpuInit.microMem)
, m_vuMem(vpuInit.vuMem)
, m_ctx(vpuInit.context)
, m_executor(*vpuInit.context)
#ifdef DEBUGGER_INCLUDED
, m_microMemMiniState(new uint8[(vpuNumber == 0) ? PS2::MICROMEM0SIZE : PS2::MICROMEM1SIZE])
, m_vuMemMiniState(new uint8[(vpuNumber == 0) ? PS2::VUMEM0SIZE : PS2::VUMEM1SIZE])
, m_topMiniState(0)
, m_itopMiniState(0)
#endif
{

}

CVPU::~CVPU()
{
#ifdef DEBUGGER_INCLUDED
	delete [] m_microMemMiniState;
	delete [] m_vuMemMiniState;
#endif
}

void CVPU::Execute(bool singleStep)
{
	unsigned int quota = singleStep ? 1 : 5000;
	assert(IsRunning());
	m_executor.Execute(quota);
	if(m_ctx->m_State.nHasException)
	{
		//E bit encountered
		m_vif.SetStat(m_vif.GetStat() & ~GetVbs());
	}
}

#ifdef DEBUGGER_INCLUDED

bool CVPU::MustBreak() const
{
	return m_executor.MustBreak();
}

void CVPU::DisableBreakpointsOnce()
{
	m_executor.DisableBreakpointsOnce();
}

void CVPU::SaveMiniState()
{
	memcpy(m_microMemMiniState, m_microMem, (m_vpuNumber == 0) ? PS2::MICROMEM0SIZE : PS2::MICROMEM1SIZE);
	memcpy(m_vuMemMiniState, m_vuMem, (m_vpuNumber == 0) ? PS2::VUMEM0SIZE : PS2::VUMEM1SIZE);
	memcpy(&m_vuMiniState, &m_ctx->m_State, sizeof(MIPSSTATE));
	m_topMiniState = (m_vpuNumber == 0) ? 0 : GetTOP();
	m_itopMiniState = GetITOP();
}

const MIPSSTATE& CVPU::GetVuMiniState() const
{
	return m_vuMiniState;
}

uint8* CVPU::GetVuMemoryMiniState() const
{
	return m_vuMemMiniState;
}

uint8* CVPU::GetMicroMemoryMiniState() const
{
	return m_microMemMiniState;
}

uint32 CVPU::GetVuTopMiniState() const
{
	return m_topMiniState;
}

uint32 CVPU::GetVuItopMiniState() const
{
	return m_itopMiniState;
}

#endif

void CVPU::Reset()
{
	m_executor.Reset();
	memset(&m_STAT, 0, sizeof(STAT));
	memset(&m_CODE, 0, sizeof(CODE));
	memset(&m_CYCLE, 0, sizeof(CYCLE));
	memset(&m_R, 0, sizeof(m_R));
	memset(&m_C, 0, sizeof(m_C));
	memset(&m_buffer, 0, sizeof(m_buffer));
	m_MODE = 0;
	m_NUM = 0;
	m_MASK = 0;
	m_MARK = 0;
	m_ITOP = 0;
	m_ITOPS = 0;
	m_readTick = 0;
	m_writeTick = 0;
}

void CVPU::SaveState(Framework::CZipArchiveWriter& archive)
{
	std::string path = STATE_PREFIX + boost::lexical_cast<std::string>(m_vpuNumber) + STATE_SUFFIX;
	CRegisterStateFile* registerFile = new CRegisterStateFile(path.c_str());
	registerFile->SetRegister32(STATE_REGS_STAT,		m_STAT);
	registerFile->SetRegister32(STATE_REGS_CODE,		m_CODE);
	registerFile->SetRegister32(STATE_REGS_CYCLE,		m_CYCLE);
	registerFile->SetRegister32(STATE_REGS_NUM,			m_NUM);
	registerFile->SetRegister32(STATE_REGS_MODE,		m_MODE);
	registerFile->SetRegister32(STATE_REGS_MASK,		m_MASK);
	registerFile->SetRegister32(STATE_REGS_MARK,		m_MARK);
	registerFile->SetRegister32(STATE_REGS_ROW0,		m_R[0]);
	registerFile->SetRegister32(STATE_REGS_ROW1,		m_R[1]);
	registerFile->SetRegister32(STATE_REGS_ROW2,		m_R[2]);
	registerFile->SetRegister32(STATE_REGS_ROW3,		m_R[3]);
	registerFile->SetRegister32(STATE_REGS_COL0,		m_C[0]);
	registerFile->SetRegister32(STATE_REGS_COL1,		m_C[1]);
	registerFile->SetRegister32(STATE_REGS_COL2,		m_C[2]);
	registerFile->SetRegister32(STATE_REGS_COL3,		m_C[3]);
	registerFile->SetRegister32(STATE_REGS_ITOP,		m_ITOP);
	registerFile->SetRegister32(STATE_REGS_ITOPS,		m_ITOPS);
	registerFile->SetRegister32(STATE_REGS_READTICK,	m_readTick);
	registerFile->SetRegister32(STATE_REGS_WRITETICK,	m_writeTick);
	archive.InsertFile(registerFile);
}

void CVPU::LoadState(Framework::CZipArchiveReader& archive)
{
	std::string path = STATE_PREFIX + boost::lexical_cast<std::string>(m_vpuNumber) + STATE_SUFFIX;
	CRegisterStateFile registerFile(*archive.BeginReadFile(path.c_str()));
	m_STAT		<<= registerFile.GetRegister32(STATE_REGS_STAT);
	m_CODE		<<= registerFile.GetRegister32(STATE_REGS_CODE);
	m_CYCLE		<<= registerFile.GetRegister32(STATE_REGS_CYCLE);
	m_NUM		= static_cast<uint8>(registerFile.GetRegister32(STATE_REGS_NUM));
	m_MODE		= registerFile.GetRegister32(STATE_REGS_MODE);
	m_MASK		= registerFile.GetRegister32(STATE_REGS_MASK);
	m_MARK		= registerFile.GetRegister32(STATE_REGS_MARK);
	m_R[0]		= registerFile.GetRegister32(STATE_REGS_ROW0);
	m_R[1]		= registerFile.GetRegister32(STATE_REGS_ROW1);
	m_R[2]		= registerFile.GetRegister32(STATE_REGS_ROW2);
	m_R[3]		= registerFile.GetRegister32(STATE_REGS_ROW3);
	m_C[0]		= registerFile.GetRegister32(STATE_REGS_COL0);
	m_C[1]		= registerFile.GetRegister32(STATE_REGS_COL1);
	m_C[2]		= registerFile.GetRegister32(STATE_REGS_COL2);
	m_C[3]		= registerFile.GetRegister32(STATE_REGS_COL3);
	m_ITOP		= registerFile.GetRegister32(STATE_REGS_ITOP);
	m_ITOPS		= registerFile.GetRegister32(STATE_REGS_ITOPS);
	m_readTick	= registerFile.GetRegister32(STATE_REGS_READTICK);
	m_writeTick	= registerFile.GetRegister32(STATE_REGS_WRITETICK);
}

uint32 CVPU::GetTOP() const
{
	throw std::exception();
}

uint32 CVPU::GetITOP() const
{
	return m_ITOP;
}

uint8* CVPU::GetVuMemory() const
{
	return m_vuMem;
}

CMIPS& CVPU::GetContext() const
{
	return *m_ctx;
}

void CVPU::ProcessPacket(StreamType& stream)
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
			if(IsRunning()) break;
			m_STAT.nVEW = 0;
			//Command is waiting for micro-program to end.
			ExecuteCommand(stream, m_CODE);
			continue;
		}

		stream.Read(&m_CODE, sizeof(CODE));

		assert(m_CODE.nI == 0);

		m_NUM = m_CODE.nNUM;

		ExecuteCommand(stream, m_CODE);
	}
}

void CVPU::ExecuteCommand(StreamType& stream, CODE nCommand)
{
#ifdef _DEBUG
	if(m_vpuNumber == 0)
	{
		DisassembleCommand(nCommand);
	}
#endif
	if(nCommand.nCMD >= 0x60)
	{
		return Cmd_UNPACK(stream, nCommand, (nCommand.nIMM & 0x03FF));
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
		m_ITOPS = nCommand.nIMM & 0x3FF;
		break;
	case 0x05:
		//STMOD
		m_MODE = nCommand.nIMM & 0x03;
		break;
	case 0x07:
		//MARK
		m_MARK = nCommand.nIMM;
		break;
	case 0x10:
		//FLUSHE
		if(IsVuRunning())
		{
			m_STAT.nVEW = 1;
		}
		else
		{
			m_STAT.nVEW = 0;
		}
		break;
	case 0x14:
		//MSCAL
		StartMicroProgram(nCommand.nIMM * 8);
		break;
	case 0x15:
		//MSCALF
		//TODO: Wait for GIF PATH 1 and 2 transfers to be over 
		StartMicroProgram(nCommand.nIMM * 8);
		break;
	case 0x17:
		//MSCNT
		StartMicroProgram(m_ctx->m_State.nPC);
		break;
	case 0x20:
		//STMASK
		return Cmd_STMASK(stream, nCommand);
		break;
	case 0x30:
		//STROW
		return Cmd_STROW(stream, nCommand);
		break;
	case 0x31:
		//STCOL
		return Cmd_STCOL(stream, nCommand);
		break;
	case 0x4A:
		//MPG
		return Cmd_MPG(stream, nCommand);
		break;
	default:
		assert(0);
		break;
	}
}

void CVPU::StartMicroProgram(uint32 address)
{
	if(IsRunning())
	{
		m_STAT.nVEW = 1;
		return;
	}

	assert(!m_STAT.nVEW);
	ExecuteMicro(address);
}

void CVPU::ExecuteMicro(uint32 nAddress)
{
	m_ITOP = m_ITOPS;

	CLog::GetInstance().Print(LOG_NAME, "Starting microprogram execution at 0x%0.8X.\r\n", nAddress);

	m_ctx->m_State.nPC = nAddress;
	m_ctx->m_State.pipeTime = 0;
	m_ctx->m_State.nHasException = 0;

#ifdef DEBUGGER_INCLUDED
	SaveMiniState();
#endif

	m_vif.SetStat(m_vif.GetStat() | GetVbs());

	for(unsigned int i = 0; i < 100; i++)
	{
		Execute(false);
		if(!IsRunning()) break;
	}
}

void CVPU::Cmd_MPG(StreamType& stream, CODE nCommand)
{
	uint32 nSize = stream.GetAvailableReadBytes();

	uint32 nNum			= (m_NUM == 0) ? (256) : (m_NUM);
	uint32 nCodeNum		= (m_CODE.nNUM == 0) ? (256) : (m_CODE.nNUM);
	uint32 nTransfered	= (nCodeNum - nNum) * 8;

	nCodeNum *= 8;
	nNum *= 8;

	nSize = std::min<uint32>(nNum, nSize);

	uint32 nDstAddr = (m_CODE.nIMM * 8) + nTransfered;

	//Check if microprogram is running
	if(IsRunning())
	{
		m_STAT.nVEW = 1;
		return;
	}

	if(nSize != 0)
	{
		uint8* microProgram = reinterpret_cast<uint8*>(alloca(nSize));
		stream.Read(microProgram, nSize);

		//Check if there's a change
		if(memcmp(m_microMem + nDstAddr, microProgram, nSize) != 0)
		{
			m_executor.ClearActiveBlocks();
			memcpy(m_microMem + nDstAddr, microProgram, nSize);
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

void CVPU::Cmd_STROW(StreamType& stream, CODE nCommand)
{
	if(m_NUM == 0)
	{
		m_NUM = 4;
	}

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

void CVPU::Cmd_STCOL(StreamType& stream, CODE nCommand)
{
	if(m_NUM == 0)
	{
		m_NUM = 4;
	}

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

void CVPU::Cmd_STMASK(StreamType& stream, CODE command)
{
	if(m_NUM == 0)
	{
		m_NUM = 1;
	}

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

void CVPU::Cmd_UNPACK(StreamType& stream, CODE nCommand, uint32 nDstAddr)
{
	assert((nCommand.nCMD & 0x60) == 0x60);

	bool usn = (m_CODE.nIMM & 0x4000) != 0;
	bool useMask = (nCommand.nCMD & 0x10) != 0;
	uint32 cl = m_CYCLE.nCL;
	uint32 wl = m_CYCLE.nWL;

	if(m_NUM == nCommand.nNUM)
	{
		m_readTick = 0;
		m_writeTick = 0;
	}

	uint32 currentNum = (m_NUM == 0) ? 256 : m_NUM;
	uint32 codeNum = (m_CODE.nNUM == 0) ? 256 : m_CODE.nNUM;
	uint32 transfered = codeNum - currentNum;
	assert(transfered == 0 || cl == wl);	//The value above is only valid for specific combinations of cl and wl

	nDstAddr += transfered;
	nDstAddr *= 0x10;

	uint128* dst = reinterpret_cast<uint128*>(&m_vuMem[nDstAddr]);

	while((currentNum != 0) && stream.GetAvailableReadBytes())
	{
		bool mustWrite = false;
		uint128 writeValue;

		if(cl >= wl)
		{
			if(m_readTick < wl || wl == 0)
			{
				bool success = Unpack_ReadValue(nCommand, stream, writeValue, usn);
				if(!success) break;
				mustWrite = true;
			}
		}
		else
		{
			if(m_writeTick < cl)
			{
				bool success = Unpack_ReadValue(nCommand, stream, writeValue, usn);
				if(!success) break;
			}

			mustWrite = true;
		}

		if(mustWrite)
		{
			for(unsigned int i = 0; i < 4; i++)
			{
				uint32 maskOp = useMask ? GetMaskOp(i, m_writeTick) : MASK_DATA;

				if(maskOp == MASK_DATA)
				{
					if(m_MODE == MODE_OFFSET)
					{
						writeValue.nV[i] += m_R[i];
					}
					else if(m_MODE == MODE_DIFFERENCE)
					{
						assert(0);
					}

					dst->nV[i] = writeValue.nV[i];
				}
				else if(maskOp == MASK_ROW)
				{
					dst->nV[i] = m_R[i];
				}
				else if(maskOp == MASK_COL)
				{
					int index = (m_writeTick > 3) ? 3 : m_writeTick;
					dst->nV[i] = m_C[index];
				}
				else if(maskOp == MASK_MASK)
				{
					assert(0);
				}
				else
				{
					assert(0);
				}
			}
			
			currentNum--;
		}

		if(cl >= wl)
		{
			m_writeTick = std::min<uint32>(m_writeTick + 1, wl);
			m_readTick = std::min<uint32>(m_readTick + 1, cl);

			if(m_readTick == cl)
			{
				m_writeTick = 0;
				m_readTick = 0;
			}
		}
		else
		{
			m_writeTick = std::min<uint32>(m_writeTick + 1, wl);
			m_readTick = std::min<uint32>(m_readTick + 1, cl);

			if(m_writeTick == wl)
			{
				m_writeTick = 0;
				m_readTick = 0;
			}
		}

		dst++;
	}

	if(currentNum != 0)
	{
		m_STAT.nVPS = 1;
	}
	else
	{
//        assert((m_cmdBuffer.GetReadCount() & 0x03) == 0);
		stream.Align32();
		m_STAT.nVPS = 0;
	}

	m_NUM = static_cast<uint8>(currentNum);
}

bool CVPU::Unpack_ReadValue(const CODE& nCommand, StreamType& stream, uint128& writeValue, bool usn)
{
	bool success = false;
	switch(nCommand.nCMD & 0x0F)
	{
	case 0x00:
		//S-32
		success = Unpack_S32(stream, writeValue);
		break;
	case 0x01:
		//S-16
		success = Unpack_S16(stream, writeValue, usn);
		break;
	case 0x02:
		//S-8
		success = Unpack_S8(stream, writeValue, usn);
		break;
	case 0x04:
		//V2-32
		success = Unpack_V32(stream, writeValue, 2);
		break;
	case 0x05:
		//V2-16
		success = Unpack_V16(stream, writeValue, 2, usn);
		break;
	case 0x08:
		//V3-32
		success = Unpack_V32(stream, writeValue, 3);
		break;
	case 0x09:
		//V3-16
		success = Unpack_V16(stream, writeValue, 3, usn);
		break;
	case 0x0A:
		//V3-8
		success = Unpack_V8(stream, writeValue, 3, usn);
		break;
	case 0x0C:
		//V4-32
		success = Unpack_V32(stream, writeValue, 4);
		break;
	case 0x0D:
		//V4-16
		success = Unpack_V16(stream, writeValue, 4, usn);
		break;
	case 0x0E:
		//V4-8
		success = Unpack_V8(stream, writeValue, 4, usn);
		break;
	case 0x0F:
		//V4-5
		success = Unpack_V45(stream, writeValue);
		break;
	default:
		assert(0);
		break;
	}
	return success;
}

bool CVPU::Unpack_S32(StreamType& stream, uint128& result)
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

bool CVPU::Unpack_S16(StreamType& stream, uint128& result, bool zeroExtend)
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

bool CVPU::Unpack_S8(StreamType& stream, uint128& result, bool zeroExtend)
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

bool CVPU::Unpack_V8(StreamType& stream, uint128& result, unsigned int fields, bool zeroExtend)
{
	if(stream.GetAvailableReadBytes() < (fields)) return false;

	for(unsigned int i = 0; i < fields; i++)
	{
		uint32 temp = 0;
		stream.Read(&temp, 1);
		if(!zeroExtend)
		{
			temp = static_cast<int8>(temp);
		}

		result.nV[i] = temp;
	}

	return true;
}

bool CVPU::Unpack_V16(StreamType& stream, uint128& result, unsigned int fields, bool zeroExtend)
{
	if(stream.GetAvailableReadBytes() < (fields * 2)) return false;

	for(unsigned int i = 0; i < fields; i++)
	{
		uint32 temp = 0;
		stream.Read(&temp, 2);
		if(!zeroExtend)
		{
			temp = static_cast<int16>(temp);
		}

		result.nV[i] = temp;
	}

	return true;
}

bool CVPU::Unpack_V32(StreamType& stream, uint128& result, unsigned int fields)
{
	if(stream.GetAvailableReadBytes() < (fields * 4)) return false;

	stream.Read(&result, (fields * 4));

	return true;
}

bool CVPU::Unpack_V45(StreamType& stream, uint128& result)
{
	if(stream.GetAvailableReadBytes() < 2) return false;

	uint16 nColor = 0;
	stream.Read(&nColor, 2);

	result.nV0 = ((nColor >>  0) & 0x1F) << 3;
	result.nV1 = ((nColor >>  5) & 0x1F) << 3;
	result.nV2 = ((nColor >> 10) & 0x1F) << 3;
	result.nV3 = ((nColor >> 15) & 0x01) << 7;

	return true;
}

uint32 CVPU::GetVbs() const
{
	switch(m_vpuNumber)
	{
	case 0: 
		return CVIF::STAT_VBS0;
	case 1: 
		return CVIF::STAT_VBS1;
	default: 
		throw std::exception();
	}
}

bool CVPU::IsVuRunning() const
{
	if(m_vpuNumber == 0)
	{
		return m_vif.IsVu0Running();
	}
	else
	{
		return m_vif.IsVu1Running();
	}
}

bool CVPU::IsRunning() const
{
	return (m_vif.GetStat() & GetVbs()) != 0;
}

bool CVPU::IsWaitingForProgramEnd() const
{
	return (m_STAT.nVEW != 0);
}

uint32 CVPU::GetMaskOp(unsigned int row, unsigned int col) const
{
	if(col > 3) col = 3;
	assert(row < 4);
	unsigned int index = (col * 4) + row;
	return (m_MASK >> (index * 2)) & 0x03;
}

void CVPU::DisassembleCommand(CODE code)
{
	if(m_STAT.nVPS != 0) return;

	CLog::GetInstance().Print(LOG_NAME, "vpu%i : ", m_vpuNumber);

	if(code.nI)
	{
		CLog::GetInstance().Print(LOG_NAME, "(I) ");
	}

	if(code.nCMD >= 0x60)
	{
		const char* packFormats[16] = 
		{
			"S-32",
			"S-16",
			"S-8",
			"(Unknown)",
			"V2-32",
			"V2-16",
			"(Unknown)",
			"(Unknown)",
			"V3-32",
			"V3-16",
			"V3-8",
			"(Unknown)",
			"V4-32",
			"V4-16",
			"V4-8",
			"V4-5"
		};
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
			CLog::GetInstance().Print(LOG_NAME, "MSKPATH3();\r\n");
			break;
		case 0x07:
			CLog::GetInstance().Print(LOG_NAME, "MARK(imm = 0x%x);\r\n", code.nIMM);
			break;
		case 0x10:
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
		case 0x20:
			CLog::GetInstance().Print(LOG_NAME, "STMASK();\r\n");
			break;
		case 0x30:
			CLog::GetInstance().Print(LOG_NAME, "STROW();\r\n");
			break;
		case 0x31:
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
