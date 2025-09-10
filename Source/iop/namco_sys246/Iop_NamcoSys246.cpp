#include "Iop_NamcoSys246.h"
#include <cstring>
#include "StdStreamUtils.h"
#include "zip/ZipArchiveReader.h"
#include "zip/ZipArchiveWriter.h"
#include "AppConfig.h"
#include "Log.h"
#include "states/RegisterStateFile.h"
#include "PathUtils.h"
#include "iop/Iop_SifCmd.h"
#include "iop/namco_sys246/Iop_NamcoAcRam.h"
#ifdef _WIN32
#include "windows.h"
#endif
#include "PS2VM_Preferences.h"

using namespace Iop;
using namespace Iop::Namco;

#define LOG_NAME ("iop_namco_sys246")

#define STATE_FILE ("iop_namcoarcade/state.xml")
#define STATE_RECV_ADDR ("recvAddr")
#define STATE_SEND_ADDR ("sendAddr")

#ifdef _WIN32
static void* g_jvs_file_mapping = nullptr;
static void* g_jvs_view_ptr = nullptr;
static bool g_coin_pressed_prev_246;
#endif

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
	0x0008, //DPAD_LEFT (Shutter Sensor Top),
	0x0004, //DPAD_RIGHT (Shutter Sensor Down),
	0x0040, //SELECT,
	0x0080, //START,
	0x4000, //SQUARE,
	0x8000, //TRIANGLE,
	0x0001, //CIRCLE (Motor Sensor),
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

CSys246::CSys246(CSifMan& sifMan, CSifCmd& sifCmd, Namco::CAcRam& acRam, const std::string& gameId)
    : m_acRam(acRam)
    , m_gameId(gameId)
{
	if(gameId == "wanganmd")
		m_jvsMode = JVS_MODE::DRIVE;
	if(gameId == "wanganmr")
		m_jvsMode = JVS_MODE::DRIVE;
	if(gameId == "rrvac")
		m_jvsMode = JVS_MODE::DRIVE;
	m_module001 = CSifModuleAdapter(std::bind(&CSys246::Invoke001, this,
	                                          std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));
	m_module003 = CSifModuleAdapter(std::bind(&CSys246::Invoke003, this,
	                                          std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));
	m_module004 = CSifModuleAdapter(std::bind(&CSys246::Invoke004, this,
	                                          std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));

	sifMan.RegisterModule(MODULE_ID_1, &m_module001);
	sifMan.RegisterModule(MODULE_ID_3, &m_module003);
	sifMan.RegisterModule(MODULE_ID_4, &m_module004);

	sifCmd.AddHleCmdHandler(COMMAND_ID_ACFLASH, std::bind(&CSys246::ProcessAcFlashCommand, this,
	                                                      std::placeholders::_1, std::placeholders::_2));

	m_jvsButtonBits = g_defaultJvsButtonBits;
#ifdef _WIN32
#ifdef TEKNOPARROT
	if(!g_jvs_file_mapping)
	{
		g_jvs_file_mapping = CreateFileMappingA(INVALID_HANDLE_VALUE,  // Use paging file
		                                        nullptr,               // Default security
		                                        PAGE_READWRITE,        // Read/write access
		                                        0,                     // Maximum object size (high-order DWORD)
		                                        64,                    // Maximum object size (low-order DWORD) - 64 bytes
		                                        "TeknoParrot_JvsState" // Name of mapping object
		);

		if(g_jvs_file_mapping)
		{
			g_jvs_view_ptr = MapViewOfFile(g_jvs_file_mapping,  // Handle to map object
			                               FILE_MAP_ALL_ACCESS, // Read/write permission
			                               0,                   // High-order 32 bits of file offset
			                               0,                   // Low-order 32 bits of file offset
			                               64                   // Number of bytes to map
			);
		}
	}
#endif
#endif

	m_cabinetId = CAppConfig::GetInstance().GetPreferenceInteger(PREF_SYS256_CABINET_LINK_ID);
}

std::string CSys246::GetId() const
{
	return "namcoarcade";
}

std::string CSys246::GetFunctionName(unsigned int) const
{
	return "unknown";
}

void CSys246::Invoke(CMIPS& context, unsigned int functionId)
{
	throw std::runtime_error("Not implemented.");
}

void CSys246::ProcessJvsPacket(const uint8* input, uint8* output)
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
#ifndef TEKNOPARROT
				(*output++) = 0x06; //Screen Pos Input
				(*output++) = 0x10; //X pos bits
				(*output++) = 0x10; //Y pos bits
				(*output++) = 0x01; //channels

				//Time Crisis 4 reads from analog input to determine screen position
				(*output++) = 0x03; //Analog Input
				(*output++) = 0x02; //Channel Count (2 channels)
				(*output++) = 0x10; //Bits (16 bits)
				(*output++) = 0x00;
#else
				if(m_gameId == "vnight")
				{
					(*output++) = 0x06; //Screen Pos Input
					(*output++) = 0x10; //X pos bits
					(*output++) = 0x10; //Y pos bits
					(*output++) = 0x02; //channels

					//Time Crisis 4 reads from analog input to determine screen position
					(*output++) = 0x03; //Analog Input
					(*output++) = 0x02; //Channel Count (2 channels)
					(*output++) = 0x10; //Bits (16 bits)
					(*output++) = 0x00;
				}
				else
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
				}
#endif

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
			else if(m_jvsMode == JVS_MODE::TOUCH)
			{
				(*output++) = 0x06; //Screen Pos Input
				(*output++) = 0x10; //X pos bits
				(*output++) = 0x10; //Y pos bits
				(*output++) = 0x01; //channels

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
#ifdef TEKNOPARROT
			BYTE testData = 0;
			BYTE control1 = 0;
			BYTE control1ext = 0;
			BYTE control2 = 0;
			BYTE control2ext = 0;
			OutputDebugStringA("IS TP");
#ifdef _WIN32
			if(g_jvs_view_ptr)
			{
				testData = *reinterpret_cast<BYTE*>(static_cast<BYTE*>(g_jvs_view_ptr) + 8);
				control1 = *reinterpret_cast<BYTE*>(static_cast<BYTE*>(g_jvs_view_ptr) + 9);
				control1ext = *reinterpret_cast<BYTE*>(static_cast<BYTE*>(g_jvs_view_ptr) + 10);
				control2 = *reinterpret_cast<BYTE*>(static_cast<BYTE*>(g_jvs_view_ptr) + 11);
				control2ext = *reinterpret_cast<BYTE*>(static_cast<BYTE*>(g_jvs_view_ptr) + 12);

				if(m_gameId == "timecrs3" && m_cabinetId == 2)
				{
					control1ext |= 0x40;
				}
			}
#endif
#endif
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
#ifndef TEKNOPARROT
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
			OutputDebugStringA("IS NOT TP");
#else
			(*output++) = testData;
			//(*output++) = (m_jvsSystemButtonState == 0x03) ? 0x80 : 0;  //Test
			(*output++) = control1;
			(*output++) = control1ext;
			(*dstSize) += 4;

			if(playerCount == 2)
			{
				(*output++) = control2;    //Player 2
				(*output++) = control2ext; //Player 2
				(*dstSize) += 2;
			}
#endif
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

#ifdef TEKNOPARROT
			// Read coin button state from shared memory (StateView[32] - separate coin offset)
			// Ported from Wii TeknoParrot code
			BYTE coin_state = 0;
#ifdef _WIN32
			if(g_jvs_view_ptr)
			{
				coin_state = *reinterpret_cast<BYTE*>(static_cast<BYTE*>(g_jvs_view_ptr) + 32);
			}
#endif

			// Check coin button state from shared memory (direct value, not bit flag)
			bool coin_pressed_now = (coin_state != 0);

			// Increment coin counter on rising edge (when coin button goes from not pressed to pressed)
			if(coin_pressed_now && !g_coin_pressed_prev_246)
			{
				m_coin1++;
				if(slotCount == 2)
				{
					m_coin2++; // For multiple slots, increment both counters
				}
			}

			// Update previous state for next frame
			g_coin_pressed_prev_246 = coin_pressed_now;
#endif
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
#ifndef TEKNOPARROT
			if(m_jvsMode == JVS_MODE::LIGHTGUN)
			{
				assert(channel == 2);
				(*output++) = static_cast<uint8>(m_jvsScreenPosX >> 8); //Pos X MSB
				(*output++) = static_cast<uint8>(m_jvsScreenPosX);      //Pos X LSB
				(*output++) = static_cast<uint8>(m_jvsScreenPosY >> 8); //Pos Y MSB
				(*output++) = static_cast<uint8>(m_jvsScreenPosY);      //Pos Y LSB
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
#else
#ifdef _WIN32
			BYTE analog0 = 0;
			BYTE analog2 = 0;
			BYTE analog4 = 0;
			BYTE analog6 = 0;
			if(g_jvs_view_ptr)
			{
				analog0 = *reinterpret_cast<BYTE*>(static_cast<BYTE*>(g_jvs_view_ptr) + 13);
				analog2 = *reinterpret_cast<BYTE*>(static_cast<BYTE*>(g_jvs_view_ptr) + 14);
				analog4 = *reinterpret_cast<BYTE*>(static_cast<BYTE*>(g_jvs_view_ptr) + 15);
				analog6 = *reinterpret_cast<BYTE*>(static_cast<BYTE*>(g_jvs_view_ptr) + 16);
			}
#endif

			if(m_jvsMode == JVS_MODE::LIGHTGUN)
			{
				if(m_gameId == "vnight")
				{
					(*output++) = static_cast<uint8>(analog0);
					(*output++) = static_cast<uint8>(0);
					(*output++) = static_cast<uint8>(analog2);
					(*output++) = static_cast<uint8>(0);
					(*output++) = static_cast<uint8>(analog4);
					(*output++) = static_cast<uint8>(0);
					(*output++) = static_cast<uint8>(analog6);
					(*output++) = static_cast<uint8>(0);
				}
				else if(m_gameId == "timecrs4")
				{
					(*output++) = static_cast<uint8>(0);
					(*output++) = static_cast<uint8>(analog0);
					(*output++) = static_cast<uint8>(0);
					(*output++) = static_cast<uint8>(analog2);
				}
				else if(m_gameId == "cobrata" || m_gameId == "cobrataw")
				{
					uint16_t a0 = (analog0 << 8) | analog0;
					uint16_t a2 = (analog2 << 8) | analog2;

					a0 = ~a0;                     // Vertical axis needs to be inverted somehow
					if(analog2 == 0) a2 = 0xFFFF; // 0xFFFF = out of screen. 0 does not trigger it
					if(a0 == 0xFFFF) a0 = 0x0;    // and for vertical its the other way around?

					(*output++) = static_cast<uint8_t>((a2 >> 8) & 0xFF);
					(*output++) = static_cast<uint8_t>(a2 & 0xFF);
					(*output++) = static_cast<uint8_t>((a0 >> 8) & 0xFF);
					(*output++) = static_cast<uint8_t>(a0 & 0xFF);
				}
				else
				{
					assert(channel == 2);
					(*output++) = static_cast<uint8>(m_jvsScreenPosX >> 8); //Pos X MSB
					(*output++) = static_cast<uint8>(m_jvsScreenPosX);      //Pos X LSB
					(*output++) = static_cast<uint8>(m_jvsScreenPosY >> 8); //Pos Y MSB
					(*output++) = static_cast<uint8>(m_jvsScreenPosY);      //Pos Y LSB
				}
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
				(*output++) = static_cast<uint8>(analog0);
				(*output++) = static_cast<uint8>(0);
				(*output++) = static_cast<uint8>(analog2);
				(*output++) = static_cast<uint8>(0);
				(*output++) = static_cast<uint8>(analog4);
				(*output++) = static_cast<uint8>(0);
				//for(int i = 0; i < JVS_WHEEL_CHANNEL_MAX; i++)
				//{
				//	(*output++) = static_cast<uint8>(m_jvsWheelChannels[i] >> 8);
				//	(*output++) = static_cast<uint8>(m_jvsWheelChannels[i]);
				//}
			}
			else
			{
				assert(false);
			}
#endif
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
#ifndef TEKNOPARROT
			(*output++) = 0x01; //Command success

			(*output++) = static_cast<uint8>(m_jvsScreenPosX >> 8); //Pos X MSB
			(*output++) = static_cast<uint8>(m_jvsScreenPosX);      //Pos X LSB
			(*output++) = static_cast<uint8>(m_jvsScreenPosY >> 8); //Pos Y MSB
			(*output++) = static_cast<uint8>(m_jvsScreenPosY);      //Pos Y LSB
#else
#ifdef _WIN32
			BYTE analog0 = 0;
			BYTE analog2 = 0;
			BYTE analog4 = 0;
			BYTE analog6 = 0;
			if(g_jvs_view_ptr)
			{
				analog0 = *reinterpret_cast<BYTE*>(static_cast<BYTE*>(g_jvs_view_ptr) + 13);
				analog2 = *reinterpret_cast<BYTE*>(static_cast<BYTE*>(g_jvs_view_ptr) + 14);
				analog4 = *reinterpret_cast<BYTE*>(static_cast<BYTE*>(g_jvs_view_ptr) + 15);
				analog6 = *reinterpret_cast<BYTE*>(static_cast<BYTE*>(g_jvs_view_ptr) + 16);
			}
#endif
			(*output++) = 0x01; //Command success
			if(channel == 1)
			{
				if(m_gameId == "timecrs3")
				{
					// TPs coordinates need to be inverted
					float gunX = static_cast<float>(255 - analog2) / 255.0f;
					float gunY = static_cast<float>(255 - analog0) / 255.0f;
					m_jvsScreenPosX = static_cast<int16>((gunX * m_screenPosXform[0]) + m_screenPosXform[1]);
					m_jvsScreenPosY = static_cast<int16>((gunY * m_screenPosXform[2]) + m_screenPosXform[3]);
					(*output++) = static_cast<uint8>(m_jvsScreenPosX >> 8); //Pos X MSB
					(*output++) = static_cast<uint8>(m_jvsScreenPosX);      //Pos X LSB
					(*output++) = static_cast<uint8>(m_jvsScreenPosY >> 8); //Pos Y MSB
					(*output++) = static_cast<uint8>(m_jvsScreenPosY);      //Pos Y LSB
				}
				else if(m_gameId == "timecrs4")
				{
					(*output++) = static_cast<uint8>(0);
					(*output++) = static_cast<uint8>(analog0);
					(*output++) = static_cast<uint8>(0);
					(*output++) = static_cast<uint8>(analog2);
				}
				else
				{
					(*output++) = static_cast<uint8>(analog0);
					(*output++) = static_cast<uint8>(0);
					(*output++) = static_cast<uint8>(analog2);
					(*output++) = static_cast<uint8>(0);
				}
			}
			else
			{
				(*output++) = static_cast<uint8>(analog4);
				(*output++) = static_cast<uint8>(0);
				(*output++) = static_cast<uint8>(analog6);
				(*output++) = static_cast<uint8>(0);
			}

#endif
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

void CSys246::SaveState(Framework::CZipArchiveWriter& archive) const
{
	auto registerFile = std::make_unique<CRegisterStateFile>(STATE_FILE);
	registerFile->SetRegister32(STATE_RECV_ADDR, m_recvAddr);
	registerFile->SetRegister32(STATE_SEND_ADDR, m_sendAddr);
	archive.InsertFile(std::move(registerFile));
}

void CSys246::LoadState(Framework::CZipArchiveReader& archive)
{
	auto registerFile = CRegisterStateFile(*archive.BeginReadFile(STATE_FILE));
	m_recvAddr = registerFile.GetRegister32(STATE_RECV_ADDR);
	m_sendAddr = registerFile.GetRegister32(STATE_SEND_ADDR);
}

void CSys246::SetJvsMode(JVS_MODE jvsMode)
{
	m_jvsMode = jvsMode;
}

void CSys246::SetButton(unsigned int buttonIndex, PS2::CControllerInfo::BUTTON buttonValue)
{
	m_jvsButtonBits[buttonValue] = (1 << buttonIndex);
}

void CSys246::SetScreenPosXform(const std::array<float, 4>& screenPosXform)
{
	m_screenPosXform = screenPosXform;
}

void CSys246::SetButtonState(unsigned int padNumber, PS2::CControllerInfo::BUTTON button, bool pressed, uint8* ram)
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

void CSys246::SetAxisState(unsigned int padNumber, PS2::CControllerInfo::BUTTON button, uint8 axisValue, uint8* ram)
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

void CSys246::SetScreenPosition(float x, float y)
{
	m_jvsScreenPosX = static_cast<int16>((x * m_screenPosXform[0]) + m_screenPosXform[1]);
	m_jvsScreenPosY = static_cast<int16>((y * m_screenPosXform[2]) + m_screenPosXform[3]);
}

void CSys246::ReleaseScreenPosition()
{
	m_jvsScreenPosX = 0xFFFF;
	m_jvsScreenPosY = 0xFFFF; //Y position is negligible
}

bool CSys246::Invoke001(uint32 method, uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	//JVIO stuff?
	//method 0x01 -> ??? called once upon idolm@ster's startup. args[2] == 0x001, args[3] == 0x2000.
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
		//Sengoku Basara, Time Crisis & Idolm@ster have argsSize == 0x10
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

bool CSys246::Invoke003(uint32 method, uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	//method 0x02 -> jvsif_starts
	//method 0x04 -> jvsif_registers
	//method 0x05 -> jvsif_request (RRV)
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

bool CSys246::Invoke004(uint32 method, uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
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

void CSys246::ProcessAcFlashCommand(const SIFCMDHEADER*, CSifMan& sifMan)
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

fs::path CSys246::GetArcadeSavePath()
{
	return CAppConfig::GetInstance().GetBasePath() / fs::path("arcadesaves");
}

void CSys246::ProcessMemRequest(uint8* ram, uint32 infoPtr)
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
		//Not sure if we need to check rootPktId, it seems to never be set by Taito games.
		if((sendData[0] == 0x3E6F) /* && (rootPktId != 0)*/)
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
			// Forcing 31khz mode seems to work fine across games, and will set compatible games to progressive mode.
			// Games like TC3 that do not support 31khz will still work in 15khz mode with these dipswitches it seems.
			// Update: Soul Calibur 3 runs progressive even without dipswitches. In fact, having them enabled
			// will make the game run at 20fps?
			if(m_gameId != "soulclb3")
			{
				ram[recvDataPtr + 0x30] = 0x70;
			}
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

void CSys246::ReadBackupRam(uint32 backupRamAddr, uint8* buffer, uint32 size)
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

void CSys246::WriteBackupRam(uint32 backupRamAddr, const uint8* buffer, uint32 size)
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
