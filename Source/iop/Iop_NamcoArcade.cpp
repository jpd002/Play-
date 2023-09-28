#include "Iop_NamcoArcade.h"
#include <cstring>
#include "StdStreamUtils.h"
#include "zip/ZipArchiveReader.h"
#include "zip/ZipArchiveWriter.h"
#include "../AppConfig.h"
#include "../Log.h"
#include "../states/RegisterStateFile.h"
#include "PathUtils.h"
#include "Iop_SifCmd.h"
#include "namco_arcade/Iop_NamcoAcRam.h"

using namespace Iop;

#define LOG_NAME "iop_namcoarcade"

#define STATE_FILE ("iop_namcoarcade/state.xml")
#define STATE_RECV_ADDR ("recvAddr")
#define STATE_SEND_ADDR ("sendAddr")

//Ref: http://daifukkat.su/files/jvs_wip.pdf
enum
{
	JVS_SYNC = 0xE0,

	JVS_CMD_IOIDENT = 0x10,
	JVS_CMD_CMDREV = 0x11,
	JVS_CMD_JVSREV = 0x12,
	JVS_CMD_COMMVER = 0x13,
	JVS_CMD_FEATCHK = 0x14,
	JVS_CMD_MAINID = 0x15,

	JVS_CMD_SWINP = 0x20,
	JVS_CMD_COININP = 0x21,
	JVS_CMD_ANLINP = 0x22,
	JVS_CMD_SCRPOSINP = 0x25,
	JVS_CMD_COINDEC = 0x30,
	JVS_CMD_COININC = 0x35,

	JVS_CMD_RESET = 0xF0,
	JVS_CMD_SETADDR = 0xF1,
};

// clang-format off
static const std::array<uint16, PS2::CControllerInfo::MAX_BUTTONS> g_defaultJvsButtonBits =
{
	0x0000,
	0x0000,
	0x0000,
	0x0000,
	0x0020, //DPAD_UP,
	0x0010, //DPAD_DOWN,
	0x0008, //DPAD_LEFT,
	0x0004, //DPAD_RIGHT,
	0x0040, //SELECT,
	0x0080, //START,
	0x4000, //SQUARE,
	0x8000, //TRIANGLE,
	0x0001, //CIRCLE,
	0x0002, //CROSS,
	0x0100, //L1,
	0x0200, //L2,
	0x0400, //L3,
	0x0800, //R1,
	0x1000, //R2,
	0x2000, //R3,
};

static const std::array<uint16, PS2::CControllerInfo::MAX_BUTTONS> g_defaultJvsSystemButtonBits =
{
	0x0000,
	0x0000,
	0x0000,
	0x0000,
	0x0000, //DPAD_UP,
	0x0000, //DPAD_DOWN,
	0x0000, //DPAD_LEFT,
	0x0000, //DPAD_RIGHT,
	0x0000, //SELECT,
	0x0000, //START,
	0x0000, //SQUARE,
	0x0000, //TRIANGLE,
	0x0000, //CIRCLE,
	0x0000, //CROSS,
	0x0000, //L1,
	0x0000, //L2,
	0x0002, //L3,
	0x0000, //R1,
	0x0000, //R2,
	0x0001, //R3,
};
// clang-format on

CNamcoArcade::CNamcoArcade(CSifMan& sifMan, CSifCmd& sifCmd, Namco::CAcRam& acRam, const std::string& gameId)
    : m_acRam(acRam)
    , m_gameId(gameId)
{
	m_module001 = CSifModuleAdapter(std::bind(&CNamcoArcade::Invoke001, this,
	                                          std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));
	m_module003 = CSifModuleAdapter(std::bind(&CNamcoArcade::Invoke003, this,
	                                          std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));
	m_module004 = CSifModuleAdapter(std::bind(&CNamcoArcade::Invoke004, this,
	                                          std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));

	sifMan.RegisterModule(MODULE_ID_1, &m_module001);
	sifMan.RegisterModule(MODULE_ID_3, &m_module003);
	sifMan.RegisterModule(MODULE_ID_4, &m_module004);

	sifCmd.AddHleCmdHandler(COMMAND_ID_ACFLASH, std::bind(&CNamcoArcade::ProcessAcFlashCommand, this,
	                                                      std::placeholders::_1, std::placeholders::_2));

	m_jvsButtonBits = g_defaultJvsButtonBits;
}

std::string CNamcoArcade::GetId() const
{
	return "namcoarcade";
}

std::string CNamcoArcade::GetFunctionName(unsigned int) const
{
	return "unknown";
}

void CNamcoArcade::Invoke(CMIPS& context, unsigned int functionId)
{
	throw std::runtime_error("Not implemented.");
}

void CNamcoArcade::ProcessJvsPacket(const uint8* input, uint8* output)
{
	assert(*input == JVS_SYNC);
	input++;
	uint8 inDest = *input++;
	uint8 inSize = *input++;
	FRAMEWORK_MAYBE_UNUSED uint8 outSize = 0;
	FRAMEWORK_MAYBE_UNUSED uint32 inWorkChecksum = inDest + inSize;
	inSize--;

	(*output++) = JVS_SYNC;
	(*output++) = 0x00; //Master ID?
	uint8* dstSize = output++;
	(*dstSize) = 1;
	(*output++) = 0x01; //Packet Success

	while(inSize != 0)
	{
		uint8 cmd = (*input++);
		inSize--;
		inWorkChecksum += cmd;
		switch(cmd)
		{
		case JVS_CMD_RESET:
		{
			assert(inSize != 0);
			uint8 param = (*input++);
			assert(param == 0xD9);
			inSize--;
			inWorkChecksum += param;
		}
		break;
		case JVS_CMD_IOIDENT:
		{
			(*output++) = 0x01; //Command success
			(*dstSize)++;

			//const char* boardName = "namco ltd.;RAYS PCB;";
			const char* boardName = "namco ltd.;TSS-I/O;";
			size_t length = strlen(boardName);

			for(int i = 0; i < length + 1; i++)
			{
				(*output++) = boardName[i];
				(*dstSize)++;
			}
		}
		break;
		case JVS_CMD_SETADDR:
		{
			assert(inSize != 0);
			uint8 param = (*input++);
			inSize--;
			inWorkChecksum += param;
			(*output++) = 0x01; //Command success
			(*dstSize)++;
		}
		break;
		case JVS_CMD_CMDREV:
		{
			(*output++) = 0x01; //Command success
			(*output++) = 0x13; //Revision 1.3
			(*dstSize) += 2;
		}
		break;
		case JVS_CMD_JVSREV:
		{
			(*output++) = 0x01; //Command success
			(*output++) = 0x30; //Revision 3.0
			(*dstSize) += 2;
		}
		break;
		case JVS_CMD_COMMVER:
		{
			(*output++) = 0x01; //Command success
			(*output++) = 0x10; //Version 1.0
			(*dstSize) += 2;
		}
		break;
		case JVS_CMD_FEATCHK:
		{
			(*output++) = 0x01; //Command success

			(*output++) = 0x02;             //Coin input
			(*output++) = JVS_PLAYER_COUNT; //2 Coin slots
			(*output++) = 0x00;
			(*output++) = 0x00;

			(*output++) = 0x01;             //Switch input
			(*output++) = JVS_PLAYER_COUNT; //2 players
			(*output++) = 0x10;             //16 switches
			(*output++) = 0x00;

			if(m_jvsMode == JVS_MODE::DRIVE)
			{
				(*output++) = 0x03;                  //Analog Input
				(*output++) = JVS_WHEEL_CHANNEL_MAX; //Channel Count (3 channels)
				(*output++) = 0x10;                  //Bits (16 bits)
				(*output++) = 0x00;
				(*dstSize) += 4;
			}
			else if(m_jvsMode == JVS_MODE::LIGHTGUN)
			{
				(*output++) = 0x06; //Screen Pos Input
				(*output++) = 0x10; //X pos bits
				(*output++) = 0x10; //Y pos bits
				(*output++) = 0x01; //channels

				//Time Crisis 4 reads from analog input to determine screen position
				(*output++) = 0x03; //Analog Input
				(*output++) = 0x02; //Channel Count (2 channels)
				(*output++) = 0x10; //Bits (16 bits)
				(*output++) = 0x00;

				(*dstSize) += 8;
			}
			else if(m_jvsMode == JVS_MODE::DRUM)
			{
				(*output++) = 0x03;                 //Analog Input
				(*output++) = JVS_DRUM_CHANNEL_MAX; //Channel Count (8 channels)
				(*output++) = 0x0A;                 //Bits (10 bits)
				(*output++) = 0x00;

				(*dstSize) += 4;
			}

			(*output++) = 0x00; //End of features

			(*dstSize) += 10;
		}
		break;
		case JVS_CMD_MAINID:
		{
			while(1)
			{
				uint8 value = (*input++);
				assert(inSize != 0);
				inSize--;
				inWorkChecksum += value;
				if(value == 0) break;
			}
		}
		break;
		case JVS_CMD_SWINP:
		{
			assert(inSize >= 2);
			uint8 playerCount = (*input++);
			uint8 byteCount = (*input++);
			assert(playerCount >= 1);
			assert(playerCount <= JVS_PLAYER_COUNT);
			assert(byteCount == 2);
			inWorkChecksum += playerCount;
			inWorkChecksum += byteCount;
			inSize -= 2;

			(*output++) = 0x01; //Command success

			m_counter++;
			if(m_testButtonState == 0 && m_jvsSystemButtonState == 0x03 && m_counter > 16)
			{
				m_testButtonState = 0x80;
				m_counter = 0;
			}
			else if(m_testButtonState == 0x80 && m_jvsSystemButtonState == 0x03 && m_counter > 16)
			{
				m_testButtonState = 0;
				m_counter = 0;
			}
			(*output++) = m_testButtonState;

			//(*output++) = (m_jvsSystemButtonState == 0x03) ? 0x80 : 0;  //Test
			(*output++) = static_cast<uint8>(m_jvsButtonState[0]);      //Player 1
			(*output++) = static_cast<uint8>(m_jvsButtonState[0] >> 8); //Player 1
			(*dstSize) += 4;

			if(playerCount == 2)
			{
				(*output++) = static_cast<uint8>(m_jvsButtonState[1]);      //Player 2
				(*output++) = static_cast<uint8>(m_jvsButtonState[1] >> 8); //Player 2
				(*dstSize) += 2;
			}
		}
		break;
		case JVS_CMD_COININP:
		{
			assert(inSize != 0);
			uint8 slotCount = (*input++);
			assert(slotCount >= 1);
			assert(slotCount <= 2);
			inWorkChecksum += slotCount;
			inSize--;
			uint8 slot1Condition = 0x00;
			uint8 slot2Condition = 0x00;
			// slot condition : 0x00 = normal
			//					0x01 = coin jam
			//					0x02 = counter disconnected
			//					0x03 = busy

			(*output++) = 0x01; //Command success

			(*output++) = static_cast<uint8>(((m_coin1 >> 8) & 0x3f) | (slot1Condition << 6)); //Coin 1 MSB + slot1condition
			(*output++) = static_cast<uint8>(m_coin1 & 0x00ff);                                //Coin 1 LSB

			(*dstSize) += 3;

			if(slotCount == 2)
			{
				(*output++) = static_cast<uint8>(((m_coin2 >> 8) & 0x3f) | (slot2Condition << 6)); //Coin 2 MSB + slot2condition
				(*output++) = static_cast<uint8>(m_coin2 & 0x00ff);                                //Coin 2 LSB

				(*dstSize) += 2;
			}
		}
		break;
		case JVS_CMD_COININC: // actually never received this jvs cmd
		{
			assert(inSize != 3);
			uint8 slotCount = (*input++);
			uint8 amountMSB = (*input++);
			uint8 amountLSB = (*input++);

			assert(slotCount >= 1);
			assert(slotCount <= 2);
			//inWorkChecksum += slotCount;
			inSize -= 3;

			if(slotCount == 1) m_coin1 += (amountMSB << 8) + amountLSB;
			if(slotCount == 2) m_coin2 += (amountMSB << 8) + amountLSB;

			(*output++) = 0x01; //Command success

			(*dstSize) += 1;
		}
		break;
		case JVS_CMD_COINDEC: // actually never received this jvs cmd
		{
			assert(inSize != 3);
			uint8 slotCount = (*input++);
			uint8 amountMSB = (*input++);
			uint8 amountLSB = (*input++);

			assert(slotCount >= 1);
			assert(slotCount <= 2);
			//inWorkChecksum += slotCount;
			inSize -= 3;

			if(slotCount == 1) m_coin1 -= (amountMSB << 8) + amountLSB;
			if(slotCount == 2) m_coin2 -= (amountMSB << 8) + amountLSB;

			(*output++) = 0x01; //Command success

			(*dstSize) += 1;
		}
		break;
		case JVS_CMD_ANLINP:
		{
			assert(inSize != 0);
			uint8 channel = (*input++);
			inWorkChecksum += channel;
			inSize--;

			(*output++) = 0x01; //Command success

			if(m_jvsMode == JVS_MODE::LIGHTGUN)
			{
				assert(channel == 2);
				(*output++) = static_cast<uint8>(m_jvsGunPosX >> 8); //Pos X MSB
				(*output++) = static_cast<uint8>(m_jvsGunPosX);      //Pos X LSB
				(*output++) = static_cast<uint8>(m_jvsGunPosY >> 8); //Pos Y MSB
				(*output++) = static_cast<uint8>(m_jvsGunPosY);      //Pos Y LSB
			}
			else if(m_jvsMode == JVS_MODE::DRUM)
			{
				assert(channel == JVS_DRUM_CHANNEL_MAX);
				for(int i = 0; i < JVS_DRUM_CHANNEL_MAX; i++)
				{
					(*output++) = static_cast<uint8>(m_jvsDrumChannels[i] >> 8);
					(*output++) = static_cast<uint8>(m_jvsDrumChannels[i]);
				}
			}
			else if(m_jvsMode == JVS_MODE::DRIVE)
			{
				for(int i = 0; i < JVS_WHEEL_CHANNEL_MAX; i++)
				{
					(*output++) = static_cast<uint8>(m_jvsWheelChannels[i] >> 8);
					(*output++) = static_cast<uint8>(m_jvsWheelChannels[i]);
				}
			}
			else
			{
				assert(false);
			}

			(*dstSize) += (2 * channel) + 1;
		}
		break;
		case JVS_CMD_SCRPOSINP:
		{
			assert(inSize != 0);
			uint8 channel = (*input++);
			assert(channel == 1);
			inWorkChecksum += channel;
			inSize--;

			(*output++) = 0x01; //Command success

			(*output++) = static_cast<uint8>(m_jvsGunPosX >> 8); //Pos X MSB
			(*output++) = static_cast<uint8>(m_jvsGunPosX);      //Pos X LSB
			(*output++) = static_cast<uint8>(m_jvsGunPosY >> 8); //Pos Y MSB
			(*output++) = static_cast<uint8>(m_jvsGunPosY);      //Pos Y LSB

			(*dstSize) += 5;
		}
		break;
		default:
			//Unknown command
			assert(false);
			break;
		}
	}
	FRAMEWORK_MAYBE_UNUSED uint8 inChecksum = (*input);
	assert(inChecksum == (inWorkChecksum & 0xFF));
}

void CNamcoArcade::SaveState(Framework::CZipArchiveWriter& archive) const
{
	auto registerFile = std::make_unique<CRegisterStateFile>(STATE_FILE);
	registerFile->SetRegister32(STATE_RECV_ADDR, m_recvAddr);
	registerFile->SetRegister32(STATE_SEND_ADDR, m_sendAddr);
	archive.InsertFile(std::move(registerFile));
}

void CNamcoArcade::LoadState(Framework::CZipArchiveReader& archive)
{
	auto registerFile = CRegisterStateFile(*archive.BeginReadFile(STATE_FILE));
	m_recvAddr = registerFile.GetRegister32(STATE_RECV_ADDR);
	m_sendAddr = registerFile.GetRegister32(STATE_SEND_ADDR);
}

void CNamcoArcade::SetJvsMode(JVS_MODE jvsMode)
{
	m_jvsMode = jvsMode;
}

void CNamcoArcade::SetButton(unsigned int buttonIndex, PS2::CControllerInfo::BUTTON buttonValue)
{
	m_jvsButtonBits[buttonValue] = (1 << buttonIndex);
}

void CNamcoArcade::SetLightGunXform(const std::array<float, 4>& lightGunXform)
{
	m_lightGunXform = lightGunXform;
}

void CNamcoArcade::SetButtonState(unsigned int padNumber, PS2::CControllerInfo::BUTTON button, bool pressed, uint8* ram)
{
	//For Ridge Racer V (coin button)
	//if(pressed && (m_recvAddr != 0))
	//{
	//	*reinterpret_cast<uint16*>(ram + m_recvAddr + 0xC0) += 1;
	//}
	if(padNumber < JVS_PLAYER_COUNT)
	{
		static const uint16 drumPressValue = 0x200;

		m_jvsButtonState[padNumber] &= ~m_jvsButtonBits[button];
		m_jvsButtonState[padNumber] |= (pressed ? m_jvsButtonBits[button] : 0);

		if(padNumber == 0)
		{
			m_jvsSystemButtonState &= ~g_defaultJvsSystemButtonBits[button];
			m_jvsSystemButtonState |= (pressed ? g_defaultJvsSystemButtonBits[button] : 0);

			if(m_jvsMode == JVS_MODE::DRIVE)
			{
				m_jvsWheelChannels[JVS_WHEEL_CHANNEL_WHEEL] = m_jvsWheel << 8;
				m_jvsWheelChannels[JVS_WHEEL_CHANNEL_GAZ] = m_jvsGaz << 8;
				m_jvsWheelChannels[JVS_WHEEL_CHANNEL_BRAKE] = m_jvsBrake << 8;
			}
			else if(m_jvsMode == JVS_MODE::DRUM)
			{
				if(button == PS2::CControllerInfo::L1) m_jvsDrumChannels[JVS_DRUM_CHANNEL_1P_DL] = pressed ? drumPressValue << 6 : 0;
				if(button == PS2::CControllerInfo::L2) m_jvsDrumChannels[JVS_DRUM_CHANNEL_1P_KL] = pressed ? drumPressValue << 6 : 0;
				if(button == PS2::CControllerInfo::R1) m_jvsDrumChannels[JVS_DRUM_CHANNEL_1P_DR] = pressed ? drumPressValue << 6 : 0;
				if(button == PS2::CControllerInfo::R2) m_jvsDrumChannels[JVS_DRUM_CHANNEL_1P_KR] = pressed ? drumPressValue << 6 : 0;
			}
		}
		else if(padNumber == 1)
		{
			if(m_jvsMode == JVS_MODE::DRUM)
			{
				if(button == PS2::CControllerInfo::L1) m_jvsDrumChannels[JVS_DRUM_CHANNEL_2P_DL] = pressed ? drumPressValue << 6 : 0;
				if(button == PS2::CControllerInfo::L2) m_jvsDrumChannels[JVS_DRUM_CHANNEL_2P_KL] = pressed ? drumPressValue << 6 : 0;
				if(button == PS2::CControllerInfo::R1) m_jvsDrumChannels[JVS_DRUM_CHANNEL_2P_DR] = pressed ? drumPressValue << 6 : 0;
				if(button == PS2::CControllerInfo::R2) m_jvsDrumChannels[JVS_DRUM_CHANNEL_2P_KR] = pressed ? drumPressValue << 6 : 0;
			}
		}
	}
	//The following code path is for handling JVSIF which only earlier games use
	if(m_recvAddr && m_sendAddr)
	{
		auto sendData = reinterpret_cast<const uint16*>(ram + m_sendAddr);
		auto recvData = reinterpret_cast<uint16*>(ram + m_recvAddr);
		recvData[0] = sendData[0];
		if(sendData[0] == 0x3E6F)
		{
			//sendData + 0x10 (bytes) -> some command id (used by Vampire Night)
			//sendData + 0x18 (bytes) -> some command id
			//sendData + 0x1A (bytes) -> some command id (used by Vampire Night)
			//sendData + 0x22 (bytes) -> Start of packet

			//recvData + 0x28 (needs to be the same command id) (used by Vampire Night)
			//recvData + 0x2A needs to be >= 0x5210 (used by Vampire Night)
			//recvData + 0x2C needs to be >= 0x5210 (used by Vampire Night)
			//recvData + 0x2E needs to be >= 0x5210 (used by Vampire Night)
			//recvData + 0x40 (needs to be the same command id)
			//recvData + 0x42 (needs to be the same command id) (used by Vampire Night)
			//recvData + 0x5A packet starts here

			recvData[1] = 0x100; //JVS version
			uint16 pktId = sendData[0x0C];
			uint16 pktId2 = sendData[0x0D];
			uint16 pktId3 = sendData[0x08];
			if(pktId != 0)
			{
				CLog::GetInstance().Warn(LOG_NAME, "PktId: 0x%04X\r\n", pktId);
				if(reinterpret_cast<const uint8*>(sendData)[0x122] == JVS_SYNC)
				{
					ProcessJvsPacket(reinterpret_cast<const uint8*>(sendData) + 0x122, reinterpret_cast<uint8*>(recvData) + 0x15A);
				}
				else
				{
					ProcessJvsPacket(reinterpret_cast<const uint8*>(sendData) + 0x22, reinterpret_cast<uint8*>(recvData) + 0x5A);
				}
				recvData[0x20] = pktId;
				recvData[0x21] = pktId2;
				recvData[0x14] = pktId3;
				recvData[0x15] = 0x5210;
				recvData[0x16] = 0x5210;
				recvData[0x17] = 0x5210;
			}
		}
	}
}

void CNamcoArcade::SetAxisState(unsigned int padNumber, PS2::CControllerInfo::BUTTON button, uint8 axisValue, uint8* ram)
{
	switch(button)
	{
	case PS2::CControllerInfo::BUTTON::ANALOG_LEFT_X:
		if(axisValue >= 0 || axisValue < 128) m_jvsWheel = axisValue + 128;
		if(axisValue > 128 || axisValue < 256) m_jvsWheel = axisValue - 128;
		m_jvsWheel = axisValue;
		break;
	case PS2::CControllerInfo::BUTTON::ANALOG_LEFT_Y:
		if(axisValue >= 128) axisValue = 127; // limit Left stick Y axis to Y+
		m_jvsGaz = -axisValue + 127;
		break;
	case PS2::CControllerInfo::BUTTON::ANALOG_RIGHT_X:
		if(axisValue < 128) axisValue = 128; // limit Right stick X axis to X+
		m_jvsBrake = axisValue - 128;
		break;
	case PS2::CControllerInfo::BUTTON::ANALOG_RIGHT_Y:
		break;
	default:
		break;
	}
}

void CNamcoArcade::SetGunPosition(float x, float y)
{
	m_jvsGunPosX = static_cast<int16>((x * m_lightGunXform[0]) + m_lightGunXform[1]);
	m_jvsGunPosY = static_cast<int16>((y * m_lightGunXform[2]) + m_lightGunXform[3]);
}

bool CNamcoArcade::Invoke001(uint32 method, uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	//JVIO stuff?
	switch(method)
	{
	case 0x02:
	{
		//Ridge Racer 5 have argsSize = 0x18 (module name seems different)
		if(argsSize == 0x18)
		{
			uint32 ramAddr = args[2];
			uint32 dstPtr = args[4];
			uint32 size = args[5];
			CLog::GetInstance().Warn(LOG_NAME, "ReadExtRam(ramAddr = 0x%08X, dstPtr = 0x%08X, size = %d);\r\n",
			                         ramAddr, dstPtr, size);
			if(ramAddr >= 0x40000000 && ramAddr < 0x50000000)
			{
				m_acRam.Read(ramAddr - 0x40000000, ram + dstPtr, size);
			}
			else if(ramAddr >= 0x60000000 && ramAddr < 0x70000000)
			{
				ReadBackupRam(ramAddr - 0x60000000, ram + dstPtr, size);
			}
			ret[0] = 0;
			ret[1] = size;
		}
		//Sengoku Basara & Time Crisis have argsSize == 0x10
		else if(argsSize == 0x10)
		{
			assert(argsSize >= 0x10);
			uint32 infoPtr = args[2];
			uint32 infoCount = args[3];
			//CLog::GetInstance().Warn(LOG_NAME, "cmd2[_0 = 0x%08X, _1 = 0x%08X, _2 = 0x%08X, _3 = 0x%08X];\r\n",
			//						 args[0], args[1], args[2], args[3]);
			for(int i = 0; i < infoCount; i++)
			{
				ProcessMemRequest(ram, infoPtr + (i * 0x10));
			}
		}
		else
		{
			CLog::GetInstance().Warn(LOG_NAME, "Unknown args size for method 2: %d.\r\n", argsSize);
		}
	}
	break;
	case 0x03:
		if(argsSize == 0x18)
		{
			uint32 ramAddr = args[2];
			uint32 srcPtr = args[4];
			uint32 size = args[5];
			CLog::GetInstance().Warn(LOG_NAME, "WriteExtRam(ramAddr = 0x%08X, srcPtr = 0x%08X, size = %d);\r\n",
			                         ramAddr, srcPtr, size);
			if(ramAddr >= 0x40000000 && ramAddr < 0x50000000)
			{
				m_acRam.Write(ramAddr - 0x40000000, ram + srcPtr, size);
			}
			else if(ramAddr >= 0x60000000 && ramAddr < 0x70000000)
			{
				WriteBackupRam(ramAddr - 0x60000000, ram + srcPtr, size);
			}
			ret[0] = 0;
			ret[1] = size;
		}
		break;
	default:
		CLog::GetInstance().Warn(LOG_NAME, "Unknown method invoked (0x%08X, 0x%08X).\r\n", 0x001, method);
		break;
	}
	return true;
}

bool CNamcoArcade::Invoke003(uint32 method, uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	//method 0x02 -> jvsif_starts
	//method 0x04 -> jvsif_registers
	//Brake and Gaz pedals control?
	switch(method)
	{
	case 0x04:
	{
		CLog::GetInstance().Warn(LOG_NAME, "jvsif_registers(0x%08X, 0x%08X);\r\n", args[0], args[1]);
		uint32* params = reinterpret_cast<uint32*>(ram + args[0]);
		uint32 recvSize = params[2];
		uint32 recvAddr = params[3];
		uint32 sendSize = params[6];
		uint32 sendAddr = params[7];
		CLog::GetInstance().Warn(LOG_NAME, "Setting JVIO params: recvSize = %d, recvAddr = 0x%08X, sendSize = %d, sendAddr = 0x%08X\r\n", recvSize, recvAddr, sendSize, sendAddr);
		m_recvAddr = recvAddr;
		m_sendAddr = sendAddr;
		//Set break and gaz values?
		//*reinterpret_cast<uint16*>(ram + registerPtr + 0x0E) = 0x0800;
		//*reinterpret_cast<uint16*>(ram + registerPtr + 0xC0) = 0x22;
	}
	break;
	default:
		CLog::GetInstance().Warn(LOG_NAME, "Unknown method invoked (0x%08X, 0x%08X).\r\n", 0x003, method);
		break;
	}
	return true;
}

bool CNamcoArcade::Invoke004(uint32 method, uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	//ac194 - Ridge Racer V & Wangan, force feedback
	switch(method)
	{
	case 0x01:
		CLog::GetInstance().Print(LOG_NAME, "ac194_status();\r\n");
		ret[0x00] = 3;
		reinterpret_cast<uint8*>(ret)[0x0C] = 'C';
		reinterpret_cast<uint8*>(ret)[0x0D] = '0';
		reinterpret_cast<uint8*>(ret)[0x0E] = '1';
		break;
	default:
		CLog::GetInstance().Warn(LOG_NAME, "Unknown method invoked (0x%08X, 0x%08X).\r\n", 0x004, method);
		break;
	}
	return true;
}

void CNamcoArcade::ProcessAcFlashCommand(const SIFCMDHEADER*, CSifMan& sifMan)
{
	//This is needed for Tekken 5: Dark Ressurection and Tekken 5.1
	//Not sure what these games use this for, but they will hang after
	//one stage if we don't send a reply to the EE.

	CLog::GetInstance().Print(LOG_NAME, "ProcessAcFlashCommand();\r\n");

	struct ACFLASH_REPLY
	{
		SIFCMDHEADER header;
		uint32 val[3];
	};
	static_assert(sizeof(ACFLASH_REPLY) == 0x1C, "sizeof(ACFLASH_REPLY) must be 28 bytes.");

	ACFLASH_REPLY reply = {};
	reply.header.commandId = 4;
	reply.header.packetSize = sizeof(ACFLASH_REPLY);
	sifMan.SendPacket(&reply, sizeof(ACFLASH_REPLY));
}

fs::path CNamcoArcade::GetArcadeSavePath()
{
	return CAppConfig::GetInstance().GetBasePath() / fs::path("arcadesaves");
}

void CNamcoArcade::ProcessMemRequest(uint8* ram, uint32 infoPtr)
{
	CLog::GetInstance().Print(LOG_NAME, "ProcessMemRequest(infoPtr = 0x%08X);\r\n", infoPtr);
	uint32* info = reinterpret_cast<uint32*>(ram + infoPtr);
	if(info[2] == 0x60002300)
	{
		//Send?
		m_sendAddr = info[1];
		//uint16* data = reinterpret_cast<uint16*>(ram + dataPtr);
		//valueToHold = data[0];
		//packetId = data[8];
		//CLog::GetInstance().Warn(LOG_NAME, "sending (0x%04X, 0x%04X, 0x%04X, 0x%04X, 0x%04X, 0x%04X, 0x%04X, 0x%04X);\r\n",
		//						 data[0], data[1], data[2], data[3],
		//						 data[4], data[5], data[6], data[7]);
	}
	else if(info[1] == 0x60002000)
	{
		//Recv?
		//Don't set/use m_recvAddr yet. Need to figure out how other JVSIF pulls data in.
		uint32 recvDataPtr = info[2];
		auto sendData = reinterpret_cast<const uint16*>(ram + m_sendAddr);
		auto recvData = reinterpret_cast<uint16*>(ram + recvDataPtr);
		//CLog::GetInstance().Warn(LOG_NAME, "recving (dataPtr = 0x%08X);\r\n", dataPtr);
		recvData[0] = sendData[0];
		uint16 rootPktId = sendData[8];
		if((sendData[0] == 0x3E6F) && (rootPktId != 0))
		{
			recvData[1] = 0x208;        //firmware version?
			recvData[0x14] = rootPktId; //Xored with value at 0x10 in send packet, needs to be the same
			recvData[0x21] = sendData[0x0D];

			//Dipswitches (from https://www.arcade-projects.com/threads/yet-another-256-display-issue.2792/#post-37365):
			//0x80 -> Test Mode
			//0x40 -> Output Level (Voltage) of Video Signal
			//0x20 -> Monitor Sync Frequency (1: 31Khz or 0: 15Khz)
			//0x10 -> Video Sync Signal (1: Separate Sync or 0: Composite Sync)
			//ram[recvDataPtr + 0x30] = 0;

			uint16 pktId = sendData[0x0C];
			if(pktId != 0)
			{
				CLog::GetInstance().Warn(LOG_NAME, "PktId: 0x%04X\r\n", pktId);
				if(reinterpret_cast<const uint8*>(sendData)[0x122] == JVS_SYNC)
				{
					ProcessJvsPacket(reinterpret_cast<const uint8*>(sendData) + 0x122, reinterpret_cast<uint8*>(recvData) + 0x15A);
				}
				else
				{
					ProcessJvsPacket(reinterpret_cast<const uint8*>(sendData) + 0x22, reinterpret_cast<uint8*>(recvData) + 0x5A);
				}
				recvData[0x20] = pktId;
			}
			recvData[0x15] = 0x5210;
			recvData[0x16] = 0x5210;
			recvData[0x17] = 0x5210;
		}
	}
	else if((info[1] >= 0x40000000) && (info[1] < 0x50000000))
	{
		m_acRam.Read(info[1] - 0x40000000, ram + info[2], info[3]);
	}
	else if((info[1] >= 0x50000000) && (info[1] < 0x60000000))
	{
		ReadBackupRam(info[1] - 0x50000000, ram + info[2], info[3]);
	}
	else if((info[2] >= 0x40000000) && (info[2] < 0x50000000))
	{
		m_acRam.Write(info[2] - 0x40000000, ram + info[1], info[3]);
	}
	else if((info[2] >= 0x50000000) && (info[1] < 0x60000000))
	{
		WriteBackupRam(info[2] - 0x50000000, ram + info[1], info[3]);
	}
	else
	{
		CLog::GetInstance().Warn(LOG_NAME, "Unknown Packet\r\n");
		CLog::GetInstance().Warn(LOG_NAME, "info[_0 = 0x%08X, _1 = 0x%08X, _2 = 0x%08X, _3 = 0x%08X];\r\n",
		                         info[0], info[1], info[2], info[3]);
		CLog::GetInstance().Warn(LOG_NAME, "------------------------------------------\r\n");
	}
}

void CNamcoArcade::ReadBackupRam(uint32 backupRamAddr, uint8* buffer, uint32 size)
{
	memset(buffer, 0, size);
	if((backupRamAddr >= BACKUP_RAM_SIZE) || ((backupRamAddr + size) > BACKUP_RAM_SIZE))
	{
		CLog::GetInstance().Warn(LOG_NAME, "Reading outside of backup RAM bounds.\r\n");
		return;
	}
	auto backupRamPath = GetArcadeSavePath() / (m_gameId + ".backupram");
	if(!fs::exists(backupRamPath))
	{
		return;
	}
	auto stream = Framework::CreateInputStdStream(backupRamPath.native());
	stream.Seek(backupRamAddr, Framework::STREAM_SEEK_SET);
	stream.Read(buffer, size);
}

void CNamcoArcade::WriteBackupRam(uint32 backupRamAddr, const uint8* buffer, uint32 size)
{
	if((backupRamAddr >= BACKUP_RAM_SIZE) || ((backupRamAddr + size) > BACKUP_RAM_SIZE))
	{
		CLog::GetInstance().Warn(LOG_NAME, "Writing outside of backup RAM bounds.\r\n");
		return;
	}
	auto arcadeSavePath = GetArcadeSavePath();
	auto backupRamPath = arcadeSavePath / (m_gameId + ".backupram");
	Framework::PathUtils::EnsurePathExists(arcadeSavePath);
	auto stream = fs::exists(backupRamPath) ? Framework::CreateUpdateExistingStdStream(backupRamPath.native()) : Framework::CreateOutputStdStream(backupRamPath.native());
	stream.Seek(backupRamAddr, Framework::STREAM_SEEK_SET);
	stream.Write(buffer, size);
}
