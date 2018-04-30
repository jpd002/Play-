#include "Iop_Sio2.h"
#include "../Log.h"
#include "../RegisterStateFile.h"
#include "../MemoryStateFile.h"
#include <assert.h>

#define LOG_NAME ("iop_sio2")

#define STATE_REGS ("sio2/regs")
#define STATE_CTRL1 ("sio2/ctrl1")
#define STATE_CTRL2 ("sio2/ctrl2")
#define STATE_PAD ("sio2/pad")
#define STATE_INPUT ("sio2/input")
#define STATE_OUTPUT ("sio2/output")

#define STATE_REGS_XML ("sio2/regs.xml")
#define STATE_REGS_CURRENTREGINDEX ("CurrentRegIndex")

using namespace Iop;

static const uint8 DUALSHOCK2_MODEL[6] =
    {
        0x03, //Model
        0x02, //Mode Count
        0x00, //Mode Current Offset
        0x02, //Actuator Count
        0x01, //Actuator Comb Count
        0x00};

static const uint8 DUALSHOCK2_ID[5][5] =
    {
        {0x00, 0x01, 0x02, 0x00, 0x0A}, //Actuator 0 info
        {0x00, 0x01, 0x01, 0x01, 0x14}, //Actuator 1 info
        {0x00, 0x02, 0x00, 0x01, 0x00}, //Actuator Comb 0 info
        {0x00, 0x00, 0x04, 0x00, 0x00}, //Mode 0 info
        {0x00, 0x00, 0x07, 0x00, 0x00}  //Mode 1 info
};

#define ID_DIGITAL 0x41
#define ID_ANALOG 0x73
#define ID_ANALOGP 0x79
#define ID_CONFIG 0xF3

CSio2::CSio2(Iop::CIntc& intc)
    : m_intc(intc)
{
	Reset();
}

void CSio2::Reset()
{
	m_currentRegIndex = 0;
	m_outputBuffer.clear();
	m_inputBuffer.clear();
	memset(m_regs, 0, sizeof(m_regs));
	memset(m_ctrl1, 0, sizeof(m_ctrl1));
	memset(m_ctrl2, 0, sizeof(m_ctrl2));
	memset(&m_padState, 0, sizeof(m_padState));
	for(auto& padInfo : m_padState)
	{
		padInfo.buttonState = 0xFFFF;
		padInfo.mode = ID_ANALOG;
		padInfo.pollMask[0] = 0xFF;
		padInfo.pollMask[1] = 0xFF;
		padInfo.pollMask[2] = 0x03;
		memset(padInfo.analogStickState, 0x7F, sizeof(padInfo.analogStickState));
	}
}

void CSio2::LoadState(Framework::CZipArchiveReader& archive)
{
	static const auto readBuffer =
	    [](ByteBufferType& outputBuffer, Framework::CStream& inputStream) {
		    outputBuffer.clear();
		    while(!inputStream.IsEOF())
		    {
			    uint8 buffer[256];
			    uint32 read = inputStream.Read(buffer, 256);
			    outputBuffer.insert(outputBuffer.end(), buffer, buffer + read);
		    }
	    };

	{
		CRegisterStateFile registerFile(*archive.BeginReadFile(STATE_REGS_XML));
		m_currentRegIndex = registerFile.GetRegister32(STATE_REGS_CURRENTREGINDEX);
	}

	archive.BeginReadFile(STATE_REGS)->Read(&m_regs, sizeof(m_regs));
	archive.BeginReadFile(STATE_CTRL1)->Read(&m_ctrl1, sizeof(m_ctrl1));
	archive.BeginReadFile(STATE_CTRL2)->Read(&m_ctrl2, sizeof(m_ctrl2));
	archive.BeginReadFile(STATE_PAD)->Read(&m_padState, sizeof(m_padState));

	readBuffer(m_outputBuffer, *archive.BeginReadFile(STATE_OUTPUT));
	readBuffer(m_inputBuffer, *archive.BeginReadFile(STATE_INPUT));
}

void CSio2::SaveState(Framework::CZipArchiveWriter& archive)
{
	auto inputBuffer = std::vector<uint8>(m_inputBuffer.begin(), m_inputBuffer.end());
	auto outputBuffer = std::vector<uint8>(m_outputBuffer.begin(), m_outputBuffer.end());

	{
		auto registerFile = new CRegisterStateFile(STATE_REGS_XML);
		registerFile->SetRegister32(STATE_REGS_CURRENTREGINDEX, m_currentRegIndex);
		archive.InsertFile(registerFile);
	}

	archive.InsertFile(new CMemoryStateFile(STATE_REGS, &m_regs, sizeof(m_regs)));
	archive.InsertFile(new CMemoryStateFile(STATE_CTRL1, &m_ctrl1, sizeof(m_ctrl1)));
	archive.InsertFile(new CMemoryStateFile(STATE_CTRL2, &m_ctrl2, sizeof(m_ctrl2)));
	archive.InsertFile(new CMemoryStateFile(STATE_PAD, &m_padState, sizeof(m_padState)));
	archive.InsertFile(new CMemoryStateFile(STATE_INPUT, inputBuffer.data(), inputBuffer.size()));
	archive.InsertFile(new CMemoryStateFile(STATE_OUTPUT, outputBuffer.data(), outputBuffer.size()));
}

void CSio2::SetButtonState(unsigned int padNumber, PS2::CControllerInfo::BUTTON button, bool pressed, uint8* ram)
{
	assert(padNumber < MAX_PADS);
	if(padNumber >= MAX_PADS) return;

	auto& padState = m_padState[padNumber];
	auto buttonMask = static_cast<uint16>(GetButtonMask(button));
	padState.buttonState &= ~buttonMask;
	if(!pressed)
	{
		padState.buttonState |= buttonMask;
	}
}

void CSio2::SetAxisState(unsigned int padNumber, PS2::CControllerInfo::BUTTON axis, uint8 axisValue, uint8* ram)
{
	assert(padNumber < MAX_PADS);
	if(padNumber >= MAX_PADS) return;

	assert(axis < 4);
	if(axis >= 4) return;

	static const unsigned int axisIndex[4] =
	    {
	        2,
	        3,
	        0,
	        1};

	auto& padState = m_padState[padNumber];
	padState.analogStickState[axisIndex[axis]] = axisValue;
}

uint32 CSio2::ReadRegister(uint32 address)
{
	uint32 value = 0;
	switch(address)
	{
	case REG_DATA_IN:
		value = m_outputBuffer.front();
		m_outputBuffer.pop_front();
		break;
	}
#ifdef _DEBUG
	DisassembleRead(address, value);
#endif
	return value;
}

void CSio2::WriteRegister(uint32 address, uint32 value)
{
	if(address >= REG_BASE && address <= REG_BASE_END)
	{
		m_regs[(address - REG_BASE) / 4] = value;
	}
	else
	{
		switch(address)
		{
		case REG_PORT0_CTRL1:
		case REG_PORT1_CTRL1:
		case REG_PORT2_CTRL1:
		case REG_PORT3_CTRL1:
		{
			unsigned int portId = (address - REG_PORT0_CTRL1) / 8;
			m_ctrl1[portId] = value;
		}
		break;
		case REG_PORT0_CTRL2:
		case REG_PORT1_CTRL2:
		case REG_PORT2_CTRL2:
		case REG_PORT3_CTRL2:
		{
			unsigned int portId = (address - REG_PORT0_CTRL2) / 8;
			m_ctrl2[portId] = value;
		}
		break;
		case REG_CTRL:
			if(value == 0x0C)
			{
				m_currentRegIndex = 0;
			}
			if(value == 0x01)
			{
				//Ok, done transferring, generate interrupt.
				m_intc.AssertLine(CIntc::LINE_SIO2);
			}
			break;
		case REG_DATA_OUT:
			m_inputBuffer.push_back(static_cast<uint8>(value));
			ProcessCommand();
			break;
		}
	}
#ifdef _DEBUG
	DisassembleWrite(address, value);
#endif
}

void CSio2::ProcessCommand()
{
	uint32 currentReg = m_regs[m_currentRegIndex];
	uint32 srcSize = (currentReg >> 8) & 0x1FF;
	uint32 dstSize = (currentReg >> 18) & 0x1FF;
	if(m_inputBuffer.size() == srcSize)
	{
		unsigned int portId = currentReg & 0x03;
		uint32 deviceId = m_ctrl2[portId];
		size_t outputOffset = m_outputBuffer.size();

		for(unsigned int i = 0; i < dstSize; i++)
		{
			m_outputBuffer.push_back(0xFF);
		}

		if(deviceId == 0x00030064)
		{
			ProcessMultitap(portId, outputOffset, dstSize, srcSize);
		}
		else
		{
			ProcessController(portId, outputOffset, dstSize, srcSize);
		}

		assert((m_outputBuffer.size() - outputOffset) == dstSize);

		m_inputBuffer.clear();
		m_currentRegIndex++;
	}
}

void CSio2::ProcessController(unsigned int portId, size_t outputOffset, uint32 dstSize, uint32 srcSize)
{
	if(portId < MAX_PADS)
	{
		assert(dstSize >= 3);
		assert(srcSize >= 3);

		unsigned int padId = portId & 0x01;
		auto& padState = m_padState[padId];

		//Write header
		m_outputBuffer[outputOffset + 0x00] = 0xFF;
		m_outputBuffer[outputOffset + 0x01] = padState.configMode ? ID_CONFIG : padState.mode;
		m_outputBuffer[outputOffset + 0x02] = 0x5A; //?

		uint8 cmd = m_inputBuffer[1];
		switch(cmd)
		{
		case 0x40:
			assert(dstSize == 9);
			m_outputBuffer[outputOffset + 0x03] = 0x00;
			m_outputBuffer[outputOffset + 0x04] = 0x00;
			m_outputBuffer[outputOffset + 0x05] = 0x02;
			m_outputBuffer[outputOffset + 0x06] = 0x00;
			m_outputBuffer[outputOffset + 0x07] = 0x00;
			m_outputBuffer[outputOffset + 0x08] = 0x5A;
			CLog::GetInstance().Print(LOG_NAME, "Pad %d: SetVrefParam();\r\n", padId);
			break;
		case 0x41:
			assert(dstSize == 9);
			if(padState.mode == ID_DIGITAL)
			{
				m_outputBuffer[outputOffset + 0x03] = 0x00;
				m_outputBuffer[outputOffset + 0x04] = 0x00;
				m_outputBuffer[outputOffset + 0x05] = 0x00;
				m_outputBuffer[outputOffset + 0x06] = 0x00;
				m_outputBuffer[outputOffset + 0x07] = 0x00;
				m_outputBuffer[outputOffset + 0x08] = 0x00;
			}
			else
			{
				m_outputBuffer[outputOffset + 0x03] = padState.pollMask[0];
				m_outputBuffer[outputOffset + 0x04] = padState.pollMask[1];
				m_outputBuffer[outputOffset + 0x05] = padState.pollMask[2];
				m_outputBuffer[outputOffset + 0x06] = 0x00;
				m_outputBuffer[outputOffset + 0x07] = 0x00;
				m_outputBuffer[outputOffset + 0x08] = 0x5A;
			}
			CLog::GetInstance().Print(LOG_NAME, "Pad %d: QueryButtonMask();\r\n", padId);
			break;
		case 0x42: //Read Data
			assert(dstSize == 5 || dstSize == 9 || dstSize == 21);
			//Pad data goes here
			m_outputBuffer[outputOffset + 0x03] = static_cast<uint8>(padState.buttonState >> 8);
			m_outputBuffer[outputOffset + 0x04] = static_cast<uint8>(padState.buttonState & 0xFF);
			if(dstSize >= 9)
			{
				//Analog stuff
				m_outputBuffer[outputOffset + 0x05] = padState.analogStickState[0];
				m_outputBuffer[outputOffset + 0x06] = padState.analogStickState[1];
				m_outputBuffer[outputOffset + 0x07] = padState.analogStickState[2];
				m_outputBuffer[outputOffset + 0x08] = padState.analogStickState[3];

				if(dstSize == 21)
				{
					//Pressure stuff
					m_outputBuffer[outputOffset + 0x09] = ((padState.buttonState & 0x2000) == 0) ? 0xFF : 0x00; //Left
					m_outputBuffer[outputOffset + 0x0A] = ((padState.buttonState & 0x8000) == 0) ? 0xFF : 0x00; //Right
					m_outputBuffer[outputOffset + 0x0B] = ((padState.buttonState & 0x1000) == 0) ? 0xFF : 0x00; //Up
					m_outputBuffer[outputOffset + 0x0C] = ((padState.buttonState & 0x4000) == 0) ? 0xFF : 0x00; //Down

					m_outputBuffer[outputOffset + 0x0D] = ((padState.buttonState & 0x0010) == 0) ? 0xFF : 0x00; //Triangle
					m_outputBuffer[outputOffset + 0x0E] = ((padState.buttonState & 0x0020) == 0) ? 0xFF : 0x00; //Circle
					m_outputBuffer[outputOffset + 0x0F] = ((padState.buttonState & 0x0040) == 0) ? 0xFF : 0x00; //Cross
					m_outputBuffer[outputOffset + 0x10] = ((padState.buttonState & 0x0080) == 0) ? 0xFF : 0x00; //Square

					m_outputBuffer[outputOffset + 0x11] = ((padState.buttonState & 0x0004) == 0) ? 0xFF : 0x00; //L1
					m_outputBuffer[outputOffset + 0x12] = ((padState.buttonState & 0x0008) == 0) ? 0xFF : 0x00; //R1
					m_outputBuffer[outputOffset + 0x13] = ((padState.buttonState & 0x0001) == 0) ? 0xFF : 0x00; //L2
					m_outputBuffer[outputOffset + 0x14] = ((padState.buttonState & 0x0002) == 0) ? 0xFF : 0x00; //R2
				}
			}
			CLog::GetInstance().Print(LOG_NAME, "Pad %d: ReadData();\r\n", padId);
			break;
		case 0x43: //Enter Config Mode
			padState.configMode = (m_inputBuffer[3] == 0x01);
			CLog::GetInstance().Print(LOG_NAME, "Pad %d: EnterConfigMode(config = %d);\r\n", padId, m_inputBuffer[3]);
			break;
		case 0x44: //Set Mode & Lock
		{
			assert(dstSize == 5 || dstSize == 9);
			uint8 mode = m_inputBuffer[3];
			uint8 lock = m_inputBuffer[4];
			//cmdBuffer[4] == 0x03 -> Mode Lock
			m_outputBuffer[outputOffset + 0x03] = 0x00;
			m_outputBuffer[outputOffset + 0x04] = 0x00;
			if(dstSize == 9)
			{
				m_outputBuffer[outputOffset + 0x05] = 0x00;
				m_outputBuffer[outputOffset + 0x06] = 0x00;
				m_outputBuffer[outputOffset + 0x07] = 0x00;
				m_outputBuffer[outputOffset + 0x08] = 0x00;
			}
			padState.mode = (mode == 0x01) ? ID_ANALOG : ID_DIGITAL;
			CLog::GetInstance().Print(LOG_NAME, "Pad %d: SetModeAndLock(mode = %d, lock = %d);\r\n", padId, mode, lock);
		}
		break;
		case 0x45: //Query Model
			assert(dstSize == 9);
			assert(padState.configMode);
			std::copy(std::begin(DUALSHOCK2_MODEL), std::end(DUALSHOCK2_MODEL), m_outputBuffer.begin() + outputOffset + 0x03);
			m_outputBuffer[outputOffset + 5] = (padState.mode == ID_DIGITAL) ? 0x00 : 0x01; //0x01 if analog pad
			CLog::GetInstance().Print(LOG_NAME, "Pad %d: QueryModel();\r\n", padId);
			break;
		case 0x46:
			assert(dstSize == 9);
			assert(padState.configMode);
			if(m_inputBuffer[3] == 0x00)
			{
				std::copy(std::begin(DUALSHOCK2_ID[0]), std::end(DUALSHOCK2_ID[0]), m_outputBuffer.begin() + outputOffset + 0x04);
			}
			else
			{
				std::copy(std::begin(DUALSHOCK2_ID[1]), std::end(DUALSHOCK2_ID[1]), m_outputBuffer.begin() + outputOffset + 0x04);
			}
			CLog::GetInstance().Print(LOG_NAME, "Pad %d: QueryAct(mode = %d);\r\n", padId, m_inputBuffer[3]);
			break;
		case 0x47:
			assert(dstSize == 9);
			assert(padState.configMode);
			std::copy(std::begin(DUALSHOCK2_ID[2]), std::end(DUALSHOCK2_ID[2]), m_outputBuffer.begin() + outputOffset + 0x04);
			CLog::GetInstance().Print(LOG_NAME, "Pad %d: QueryComb();\r\n", padId);
			break;
		case 0x4C:
			assert(dstSize == 9);
			assert(padState.configMode);
			if(m_inputBuffer[3] == 0x00)
			{
				std::copy(std::begin(DUALSHOCK2_ID[3]), std::end(DUALSHOCK2_ID[3]), m_outputBuffer.begin() + outputOffset + 0x04);
			}
			else
			{
				std::copy(std::begin(DUALSHOCK2_ID[4]), std::end(DUALSHOCK2_ID[4]), m_outputBuffer.begin() + outputOffset + 0x04);
			}
			CLog::GetInstance().Print(LOG_NAME, "Pad %d: QueryMode(mode = %d);\r\n", padId, m_inputBuffer[3]);
			break;
		case 0x4D: //SetVibration
			assert(dstSize == 9);
			CLog::GetInstance().Print(LOG_NAME, "Pad %d: SetVibration();\r\n", padId);
			break;
		case 0x4F: //SetPollMask
			assert(dstSize == 9);
			assert(padState.configMode);
			padState.mode = ID_ANALOGP;
			m_outputBuffer[outputOffset + 0x03] = 0x00;
			m_outputBuffer[outputOffset + 0x04] = 0x00;
			m_outputBuffer[outputOffset + 0x05] = 0x00;
			m_outputBuffer[outputOffset + 0x06] = 0x00;
			m_outputBuffer[outputOffset + 0x07] = 0x00;
			m_outputBuffer[outputOffset + 0x08] = 0x5A;
			padState.pollMask[0] = m_inputBuffer[3];
			padState.pollMask[1] = m_inputBuffer[4];
			padState.pollMask[2] = m_inputBuffer[5];
			CLog::GetInstance().Print(LOG_NAME, "Pad %d: SetPollMask(mask = { 0x%02X, 0x%02X, 0x%02X });\r\n",
			                          padId, padState.pollMask[0], padState.pollMask[1], padState.pollMask[2]);
			break;
		default:
			CLog::GetInstance().Print(LOG_NAME, "Pad %d: Unknown command received (0x%02X).\r\n", padId, cmd);
			break;
		}
	}
	else
	{
		CLog::GetInstance().Print(LOG_NAME, "Sending command to unsupported pad (%d).\r\n", portId);
	}
}

void CSio2::ProcessMultitap(unsigned int portId, size_t outputOffset, uint32 dstSize, uint32 srcSize)
{
	uint8 cmd = m_inputBuffer[1];
	switch(cmd)
	{
	case 0x12:
	case 0x13:
		//GetSlotNumber
		m_outputBuffer[outputOffset + 0x03] = 1;
		CLog::GetInstance().Print(LOG_NAME, "Multitap: GetSlotNumber();\r\n");
		break;
	case 0x21:
	case 0x22:
		//ChangeSlot
		m_outputBuffer[outputOffset + 0x05] = 0;
		CLog::GetInstance().Print(LOG_NAME, "Multitap: ChangeSlot();\r\n");
		break;
	}
}

void CSio2::DisassembleRead(uint32 address, uint32 value)
{
	switch(address)
	{
	case REG_DATA_IN:
		CLog::GetInstance().Print(LOG_NAME, "= DATA_IN = 0x%08X\r\n", value);
		break;
	case REG_CTRL:
		CLog::GetInstance().Print(LOG_NAME, "= REG_CTRL = 0x%08X\r\n", value);
		break;
	default:
		CLog::GetInstance().Print(LOG_NAME, "Read an unknown register 0x%08X.\r\n", address);
		break;
	}
}

void CSio2::DisassembleWrite(uint32 address, uint32 value)
{
	switch(address)
	{
	case REG_PORT0_CTRL1:
		CLog::GetInstance().Print(LOG_NAME, "REG_PORT0_CTRL1 = 0x%08X\r\n", value);
		break;
	case REG_PORT0_CTRL2:
		CLog::GetInstance().Print(LOG_NAME, "REG_PORT0_CTRL2 = 0x%08X\r\n", value);
		break;
	case REG_PORT1_CTRL1:
		CLog::GetInstance().Print(LOG_NAME, "REG_PORT1_CTRL1 = 0x%08X\r\n", value);
		break;
	case REG_PORT1_CTRL2:
		CLog::GetInstance().Print(LOG_NAME, "REG_PORT1_CTRL2 = 0x%08X\r\n", value);
		break;
	case REG_PORT2_CTRL1:
		CLog::GetInstance().Print(LOG_NAME, "REG_PORT2_CTRL1 = 0x%08X\r\n", value);
		break;
	case REG_PORT2_CTRL2:
		CLog::GetInstance().Print(LOG_NAME, "REG_PORT2_CTRL2 = 0x%08X\r\n", value);
		break;
	case REG_PORT3_CTRL1:
		CLog::GetInstance().Print(LOG_NAME, "REG_PORT3_CTRL1 = 0x%08X\r\n", value);
		break;
	case REG_PORT3_CTRL2:
		CLog::GetInstance().Print(LOG_NAME, "REG_PORT3_CTRL2 = 0x%08X\r\n", value);
		break;
	case REG_DATA_OUT:
		CLog::GetInstance().Print(LOG_NAME, "DATA_OUT = 0x%08X\r\n", value);
		break;
	case REG_CTRL:
		CLog::GetInstance().Print(LOG_NAME, "CTRL = 0x%08X\r\n", value);
		break;
	default:
		CLog::GetInstance().Print(LOG_NAME, "Write 0x%08X to an unknown register 0x%08X.\r\n", value, address);
		break;
	}
}
