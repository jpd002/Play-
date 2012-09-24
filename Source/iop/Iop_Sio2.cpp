#include "Iop_Sio2.h"
#include "../Log.h"
#include <assert.h>

#define LOG_NAME ("iop_sio2")

using namespace Iop;

static const uint8 DUALSHOCK2_MODEL[6] = 
{
	0x03, 0x02, 0x00, 0x02, 0x01, 0x00
};

static const uint8 DUALSHOCK2_ID[5][5] = 
{
	{0x00, 0x01, 0x02, 0x00, 0x0A},
	{0x00, 0x01, 0x01, 0x01, 0x14},
	{0x00, 0x02, 0x00, 0x01, 0x00},
	{0x00, 0x00, 0x04, 0x00, 0x00},
	{0x00, 0x00, 0x07, 0x00, 0x00}
};

#define ID_DIGITAL	0x41
#define ID_ANALOG	0x73
#define ID_ANALOGP	0x79
#define ID_CONFIG	0xF3

CSio2::CSio2(Iop::CIntc& intc)
: m_intc(intc)
{
	Reset();
}

CSio2::~CSio2()
{

}

void CSio2::Reset()
{
	m_currentRegIndex = 0;
	m_outputBuffer.clear();
	m_inputBuffer.clear();
	memset(m_regs, 0, sizeof(m_regs));
	memset(&m_padState, 0, sizeof(m_padState));
	for(auto padInfoIterator = std::begin(m_padState);
		padInfoIterator != std::end(m_padState); padInfoIterator++)
	{
		auto& padInfo = (*padInfoIterator);
		padInfo.buttonState = 0xFFFF;
		padInfo.mode = ID_ANALOG;
		memset(padInfo.analogStickState, 0x7F, sizeof(padInfo.analogStickState));
	}
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
		1
	};

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
		case REG_CTRL:
			if(value == 0x0C)
			{
				printf("SIO2: Starting new xfer.\r\n");
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
		unsigned int padId = currentReg & 0x03;
		size_t outputOffset = m_outputBuffer.size();

		for(unsigned int i = 0; i < dstSize; i++)
		{
			m_outputBuffer.push_back(0xFF);
		}

		if(padId < MAX_PADS)
		{
			assert(dstSize >= 3);
			assert(srcSize >= 3);

			auto& padState = m_padState[padId];

			//Write header
			m_outputBuffer[outputOffset + 0x00] = 0xFF;
			m_outputBuffer[outputOffset + 0x01] = padState.configMode ? ID_CONFIG : padState.mode;
			m_outputBuffer[outputOffset + 0x02] = 0x5A;		//?

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
				if(padState.mode == ID_ANALOGP)
				{
					m_outputBuffer[outputOffset + 0x03] = padState.pollMask[0];
					m_outputBuffer[outputOffset + 0x04] = padState.pollMask[1];
					m_outputBuffer[outputOffset + 0x05] = padState.pollMask[2];
					m_outputBuffer[outputOffset + 0x06] = 0x00;
					m_outputBuffer[outputOffset + 0x07] = 0x00;
					m_outputBuffer[outputOffset + 0x08] = 0x5A;
				}
				else
				{
					m_outputBuffer[outputOffset + 0x03] = 0x00;
					m_outputBuffer[outputOffset + 0x04] = 0x00;
					m_outputBuffer[outputOffset + 0x05] = 0x00;
					m_outputBuffer[outputOffset + 0x06] = 0x00;
					m_outputBuffer[outputOffset + 0x07] = 0x00;
					m_outputBuffer[outputOffset + 0x08] = 0x00;
				}
				CLog::GetInstance().Print(LOG_NAME, "Pad %d: Cmd41();\r\n", padId);
				break;
			case 0x42:		//Read Data
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
						m_outputBuffer[outputOffset + 0x09] = 0;
						m_outputBuffer[outputOffset + 0x0A] = 0;
						m_outputBuffer[outputOffset + 0x0B] = 0;
						m_outputBuffer[outputOffset + 0x0C] = 0;
						m_outputBuffer[outputOffset + 0x0D] = 0;
						m_outputBuffer[outputOffset + 0x0E] = 0;
						m_outputBuffer[outputOffset + 0x0F] = 0;
						m_outputBuffer[outputOffset + 0x10] = 0;
						m_outputBuffer[outputOffset + 0x11] = 0;
						m_outputBuffer[outputOffset + 0x12] = 0;
						m_outputBuffer[outputOffset + 0x13] = 0;
						m_outputBuffer[outputOffset + 0x14] = 0;
					}
				}
				CLog::GetInstance().Print(LOG_NAME, "Pad %d: ReadData();\r\n", padId);
				break;
			case 0x43:		//Enter Config Mode
				assert(dstSize == 9);
				padState.configMode = (m_inputBuffer[3] == 0x01);
				CLog::GetInstance().Print(LOG_NAME, "Pad %d: EnterConfigMode(config = %d);\r\n", padId, m_inputBuffer[3]);
				break;
			case 0x44:		//Set Mode & Lock
				{
					assert(dstSize == 5 || dstSize == 9);
					uint8 mode = m_inputBuffer[3];
					uint8 lock = m_inputBuffer[4];
					//cmdBuffer[3] == 0x01 ? (u8)ID_ANALOG : (u8)ID_DIGITAL;
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
					CLog::GetInstance().Print(LOG_NAME, "Pad %d: SetModeAndLock(mode = %d, lock = %d);\r\n", padId, mode, lock);
				}
				break;
			case 0x45:		//Query Model
				assert(dstSize == 9);
				assert(padState.configMode);
				std::copy(std::begin(DUALSHOCK2_MODEL), std::end(DUALSHOCK2_MODEL), m_outputBuffer.begin() + outputOffset + 0x03);
				m_outputBuffer[outputOffset + 5] = 0x01;		//0x01 if analog pad
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
				CLog::GetInstance().Print(LOG_NAME, "Pad %d: GetDeviceId46(mode = %d);\r\n", padId, m_inputBuffer[3]);
				break;
			case 0x47:
				assert(dstSize == 9);
				assert(padState.configMode);
				std::copy(std::begin(DUALSHOCK2_ID[2]), std::end(DUALSHOCK2_ID[2]), m_outputBuffer.begin() + outputOffset + 0x04);
				CLog::GetInstance().Print(LOG_NAME, "Pad %d: GetDeviceId47();\r\n", padId);
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
				CLog::GetInstance().Print(LOG_NAME, "Pad %d: GetDeviceId4C(mode = %d);\r\n", padId, m_inputBuffer[3]);
				break;
			case 0x4D:		//SetVibration
				assert(dstSize == 9);
				CLog::GetInstance().Print(LOG_NAME, "Pad %d: SetVibration();\r\n", padId);
				break;
			case 0x4F:		//SetPollMask
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
				CLog::GetInstance().Print(LOG_NAME, "Pad %d: SetPollMask(mask = { 0x%0.2X, 0x%0.2X, 0x%0.2X });\r\n", 
					padId, padState.pollMask[0], padState.pollMask[1], padState.pollMask[2]);
				break;
			default:
				CLog::GetInstance().Print(LOG_NAME, "Pad %d: Unknown command received (0x%0.2X).\r\n", padId, cmd);
				break;
			}
		}
		else
		{
			CLog::GetInstance().Print(LOG_NAME, "Sending command to unsupported pad (%d).\r\n", padId);
		}

		assert((m_outputBuffer.size() - outputOffset) == dstSize);

		m_inputBuffer.clear();
		m_currentRegIndex++;
	}
}

void CSio2::DisassembleRead(uint32 address, uint32 value)
{
	switch(address)
	{
	case REG_DATA_IN:
		CLog::GetInstance().Print(LOG_NAME, "= DATA_IN = 0x%0.8X\r\n", value);
		break;
	case REG_CTRL:
		CLog::GetInstance().Print(LOG_NAME, "= REG_CTRL = 0x%0.8X\r\n", value);
		break;
	default:
		CLog::GetInstance().Print(LOG_NAME, "Read an unknown register 0x%0.8X.\r\n", address);
		break;
	}
}

void CSio2::DisassembleWrite(uint32 address, uint32 value)
{
	switch(address)
	{
	case REG_PORT0_CTRL1:
		CLog::GetInstance().Print(LOG_NAME, "REG_PORT0_CTRL1 = 0x%0.8X\r\n", value);
		break;
	case REG_PORT0_CTRL2:
		CLog::GetInstance().Print(LOG_NAME, "REG_PORT0_CTRL2 = 0x%0.8X\r\n", value);
		break;
	case REG_PORT1_CTRL1:
		CLog::GetInstance().Print(LOG_NAME, "REG_PORT1_CTRL1 = 0x%0.8X\r\n", value);
		break;
	case REG_PORT1_CTRL2:
		CLog::GetInstance().Print(LOG_NAME, "REG_PORT1_CTRL2 = 0x%0.8X\r\n", value);
		break;
	case REG_PORT2_CTRL1:
		CLog::GetInstance().Print(LOG_NAME, "REG_PORT2_CTRL1 = 0x%0.8X\r\n", value);
		break;
	case REG_PORT2_CTRL2:
		CLog::GetInstance().Print(LOG_NAME, "REG_PORT2_CTRL2 = 0x%0.8X\r\n", value);
		break;
	case REG_PORT3_CTRL1:
		CLog::GetInstance().Print(LOG_NAME, "REG_PORT3_CTRL1 = 0x%0.8X\r\n", value);
		break;
	case REG_PORT3_CTRL2:
		CLog::GetInstance().Print(LOG_NAME, "REG_PORT3_CTRL2 = 0x%0.8X\r\n", value);
		break;
	case REG_DATA_OUT:
		CLog::GetInstance().Print(LOG_NAME, "DATA_OUT = 0x%0.8X\r\n", value);
		break;
	case REG_CTRL:
		CLog::GetInstance().Print(LOG_NAME, "CTRL = 0x%0.8X\r\n", value);
		break;
	default:
		CLog::GetInstance().Print(LOG_NAME, "Write 0x%0.8X to an unknown register 0x%0.8X.\r\n", value, address);
		break;
	}
}
