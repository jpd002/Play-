#include <cstring>
#include "Iop_NamcoSys147.h"
#include "Log.h"
#include "StdStreamUtils.h"
#include "PathUtils.h"
#include "AppConfig.h"
#include "PS2VM_Preferences.h"

using namespace Iop;
using namespace Iop::Namco;

#define LOG_NAME ("iop_namco_sys147")

#ifdef _WIN32
static void* g_jvs_file_mapping = nullptr;
static void* g_jvs_view_ptr = nullptr;
static bool g_coin_pressed_prev_147;
#endif

//Switch IDs for games
//--------------------

//Animal Kaiser
//-------------
// 108 - Test
// 109 - Enter
// 110 - Up
// 111 - Down
// 114 - Service
// 116 - 1P Left
// 117 - 1P Right
// 118 - 2P Right
// 119 - 2P Left

//Pac-Man Arcade Party
//--------------------
// Note: Game seems to be reading from SIO2
// 84 - Test
// 86 - Up
// 87 - Down
// 88 - Enter
// 89 - Stick Down
// 90 - Stick Up
// 91 - Stick Right
// 92 - Stick Left
// 93 - Button 1
// 94 - Button 2
// 95 - P1
// 102 - P2

CSys147::CSys147(CSifMan& sifMan, const std::string& gameId)
    : m_gameId(gameId)
{
	m_module000 = CSifModuleAdapter(std::bind(&CSys147::Invoke000, this,
	                                          std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));
	m_module001 = CSifModuleAdapter(std::bind(&CSys147::Invoke001, this,
	                                          std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));
	m_module002 = CSifModuleAdapter(std::bind(&CSys147::Invoke002, this,
	                                          std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));
	m_module003 = CSifModuleAdapter(std::bind(&CSys147::Invoke003, this,
	                                          std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));
	m_module200 = CSifModuleAdapter(std::bind(&CSys147::Invoke200, this,
	                                          std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));
	m_module201 = CSifModuleAdapter(std::bind(&CSys147::Invoke201, this,
	                                          std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));
	m_module99 = CSifModuleAdapter(std::bind(&CSys147::Invoke99, this,
	                                         std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));

	sifMan.RegisterModule(MODULE_ID_000, &m_module000);
	sifMan.RegisterModule(MODULE_ID_001, &m_module001);
	sifMan.RegisterModule(MODULE_ID_002, &m_module002);
	sifMan.RegisterModule(MODULE_ID_003, &m_module003);
	sifMan.RegisterModule(MODULE_ID_200, &m_module200);
	sifMan.RegisterModule(MODULE_ID_201, &m_module201);
	sifMan.RegisterModule(MODULE_ID_99, &m_module99);

	if(CAppConfig::GetInstance().GetPreferenceBoolean(PREF_PS2_ARCADE_IO_SERVER_ENABLED))
	{
		fs::path logPath = CAppConfig::GetInstance().GetBasePath() / "arcade_io_server.log";
		uint16 port = CAppConfig::GetInstance().GetPreferenceInteger(PREF_PS2_ARCADE_IO_SERVER_PORT);
		m_ioServer = std::make_unique<Framework::CHttpServer>(port, std::bind(&CSys147::HandleIoServerRequest, this, std::placeholders::_1), logPath);
	}

#ifdef _WIN32
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
}

std::string CSys147::GetId() const
{
	return "sys147";
}

std::string CSys147::GetFunctionName(unsigned int functionId) const
{
	return "unknown";
}

void CSys147::SetIoMode(IO_MODE ioMode)
{
	m_ioMode = ioMode;
}

void CSys147::SetButton(unsigned int switchIndex, unsigned int padNumber, PS2::CControllerInfo::BUTTON button)
{
	//m_switchBindings[{padNumber, button}] = switchIndex;
}

void CSys147::SetButtonState(unsigned int padNumber, PS2::CControllerInfo::BUTTON button, bool pressed, uint8* ram)
{
#ifndef TEKNOPARROT
	const auto& binding = m_switchBindings.find({padNumber, button});
	if(binding != std::end(m_switchBindings))
	{
		m_switchStates[binding->second] = pressed ? 0xFF : 0x00;
	}

	if(m_ioMode == IO_MODE::AI)
	{
		//Player Switches
		//4 nibbles, one for each player
		//In one nibble:
		//Bit 0 - Up
		//Bit 1 - Down
		//Bit 2 - Right
		//Bit 3 - Left

		//System Switches
		//Bit 0  - Select Down
		//Bit 1  - Select Up
		//Bit 2  - Enter
		//Bit 3  - Test
		//Bit 5  - Service
		//Bit 8  - P4 Enter
		//Bit 9  - P3 Enter
		//Bit 10 - P2 Enter
		//Bit 11 - P1 Enter
		uint16 systemSwitchMask = 0;
		uint16 playerSwitchMask = 0;
		if(padNumber == 0)
		{
			switch(button)
			{
			case PS2::CControllerInfo::L1:
				systemSwitchMask = 0x0008; //Test Button
				break;
			case PS2::CControllerInfo::DPAD_UP:
				systemSwitchMask = 0x0002; //Select Up
				playerSwitchMask = 0x1000; //P1 Up
				break;
			case PS2::CControllerInfo::DPAD_DOWN:
				systemSwitchMask = 0x0001; //Select Down
				playerSwitchMask = 0x2000; //P1 Down
				break;
			case PS2::CControllerInfo::DPAD_LEFT:
				playerSwitchMask = 0x8000; //P1 Left
				break;
			case PS2::CControllerInfo::DPAD_RIGHT:
				playerSwitchMask = 0x4000; //P1 Right
				break;
			case PS2::CControllerInfo::CROSS:
				systemSwitchMask = 0x0804; //P1 Start + Enter
				break;
			default:
				break;
			}
		}
		else if(padNumber == 1)
		{
			switch(button)
			{
			case PS2::CControllerInfo::DPAD_UP:
				playerSwitchMask = 0x0010; //P2 Up
				break;
			case PS2::CControllerInfo::DPAD_DOWN:
				playerSwitchMask = 0x0020; //P2 Down
				break;
			case PS2::CControllerInfo::DPAD_LEFT:
				playerSwitchMask = 0x0080; //P2 Left
				break;
			case PS2::CControllerInfo::DPAD_RIGHT:
				playerSwitchMask = 0x0040; //P2 Right
				break;
			case PS2::CControllerInfo::CROSS:
				systemSwitchMask = 0x0400; //P2 Start
				break;
			default:
				break;
			}
		}
		else if(padNumber == 2)
		{
			switch(button)
			{
			case PS2::CControllerInfo::DPAD_UP:
				playerSwitchMask = 0x0100; //P3 Up
				break;
			case PS2::CControllerInfo::DPAD_DOWN:
				playerSwitchMask = 0x0200; //P3 Down
				break;
			case PS2::CControllerInfo::DPAD_LEFT:
				playerSwitchMask = 0x0800; //P3 Left
				break;
			case PS2::CControllerInfo::DPAD_RIGHT:
				playerSwitchMask = 0x0400; //P3 Right
				break;
			case PS2::CControllerInfo::CROSS:
				systemSwitchMask = 0x0200; //P3 Start
				break;
			default:
				break;
			}
		}
		else if(padNumber == 3)
		{
			switch(button)
			{
			case PS2::CControllerInfo::DPAD_UP:
				playerSwitchMask = 0x0001; //P4 Up
				break;
			case PS2::CControllerInfo::DPAD_DOWN:
				playerSwitchMask = 0x0002; //P4 Down
				break;
			case PS2::CControllerInfo::DPAD_LEFT:
				playerSwitchMask = 0x0008; //P4 Left
				break;
			case PS2::CControllerInfo::DPAD_RIGHT:
				playerSwitchMask = 0x0004; //P4 Right
				break;
			case PS2::CControllerInfo::CROSS:
				systemSwitchMask = 0x0100; //P4 Start
				break;
			default:
				break;
			}
		}
		m_systemSwitchState &= ~systemSwitchMask;
		m_playerSwitchState &= ~playerSwitchMask;
		if(!pressed)
		{
			m_systemSwitchState |= systemSwitchMask;
			m_playerSwitchState |= playerSwitchMask;
		}
	}
#endif
}

#ifdef TEKNOPARROT
void CSys147::GetTpUiButtons()
{
#ifdef _WIN32
	uint16 systemSwitchMask = 0;
	uint16 playerSwitchMask = 0;
	BYTE p1buttons = 0;
	BYTE p2buttons = 0;
	BYTE p3buttons = 0;
	BYTE p4buttons = 0;
	BYTE coinStatus = 0;
	BYTE p1buttons2 = 0;

	if(g_jvs_view_ptr)
	{
		p1buttons = *reinterpret_cast<BYTE*>(static_cast<BYTE*>(g_jvs_view_ptr) + 8);
		p2buttons = *reinterpret_cast<BYTE*>(static_cast<BYTE*>(g_jvs_view_ptr) + 9);
		p3buttons = *reinterpret_cast<BYTE*>(static_cast<BYTE*>(g_jvs_view_ptr) + 10);
		p4buttons = *reinterpret_cast<BYTE*>(static_cast<BYTE*>(g_jvs_view_ptr) + 11);
		// awkward...
		p1buttons2 = *reinterpret_cast<BYTE*>(static_cast<BYTE*>(g_jvs_view_ptr) + 12);

		coinStatus = *reinterpret_cast<BYTE*>(static_cast<BYTE*>(g_jvs_view_ptr) + 32);
	}

	for(auto& switchState : m_switchStates)
	{
		switchState.second = 0x00;
	}

	if(coinStatus & 0x01)
	{
		if(!g_coin_pressed_prev_147)
		{
			m_currentCredits++;
			g_coin_pressed_prev_147 = true;
		}
	}
	else
	{
		g_coin_pressed_prev_147 = false;
	}

	if(p1buttons & 0x01) //Test Button
	{
		systemSwitchMask = 0x0008;  //Test Button
		m_switchStates[108] = 0xFF; // Animal Kaiser Test
		m_switchStates[PACMAN_AP_BUTTON_TEST] = 0xFF;
	}
	if(p1buttons & 0x02) //Select Up
	{
		systemSwitchMask |= 0x0002; //Select Up
		playerSwitchMask |= 0x1000; //P1 Up
		m_switchStates[110] = 0xFF; //Animal Kaiser Up
		m_switchStates[PACMAN_AP_BUTTON_UP] = 0xFF;
	}
	if(p1buttons & 0x04) //Select Down
	{
		systemSwitchMask |= 0x0001; //Select Down
		playerSwitchMask |= 0x2000; //P1 Down
		m_switchStates[111] = 0xFF; //Animal Kaiser Down
		m_switchStates[PACMAN_AP_BUTTON_DOWN] = 0xFF;
	}
	if(p1buttons & 0x08) //P1 Left
	{
		playerSwitchMask |= 0x8000; //P1 Left
		m_switchStates[116] = 0xFF; //Animal Kaiser Left
	}
	if(p1buttons & 0x10) //P1 Right
	{
		playerSwitchMask |= 0x4000; //P1 Right
		m_switchStates[117] = 0xFF; //Animal Kaiser Right
	}
	if(p1buttons & 0x20) //P1 Start
	{
		systemSwitchMask |= 0x0804;                    //P1 Start + Enter
		m_switchStates[109] = 0xFF;                    //Animal Kaiser Enter
		m_switchStates[PACMAN_AP_BUTTON_ENTER] = 0xFF; //Pacman Arcade Party Enter
	}
	if(p1buttons & 0x40) // Service 1
	{
		m_switchStates[114] = 0xFF; //Animal Kaiser Service
		m_switchStates[PACMAN_AP_BUTTON_SERVICE] = 0xFF;
	}
	if(p2buttons & 0x02) //Select Up
	{
		playerSwitchMask |= 0x0010; //P2 Up
		m_switchStates[PACMAN_AP_BUTTON_STICKUP] = 0xFF;
	}
	if(p2buttons & 0x04) //Select Down
	{
		playerSwitchMask |= 0x0020; //P2 Down
		m_switchStates[PACMAN_AP_BUTTON_STICKDOWN] = 0xFF;
	}
	if(p2buttons & 0x08) //P2 Left
	{
		playerSwitchMask |= 0x0080; //P2 Left
		m_switchStates[119] = 0xFF; //Animal Kaiser 2P Left
		m_switchStates[PACMAN_AP_BUTTON_STICKLEFT] = 0xFF;
	}
	if(p2buttons & 0x10) //P2 Right
	{
		playerSwitchMask |= 0x0040; //P2 Right
		m_switchStates[118] = 0xFF; //Animal Kaiser 2P Right
		m_switchStates[PACMAN_AP_BUTTON_STICKRIGHT] = 0xFF;
	}
	if(p2buttons & 0x20) //P2 Start
	{
		systemSwitchMask |= 0x0400; //P2 Start
	}
	if(p3buttons & 0x02) //Select Up
	{
		playerSwitchMask |= 0x0100; //P3 Up
	}
	if(p3buttons & 0x04) //Select Down
	{
		playerSwitchMask |= 0x0200; //P3 Down
	}
	if(p3buttons & 0x08) //P3 Left
	{
		playerSwitchMask |= 0x0800; //P3 Left
	}
	if(p3buttons & 0x10) //P3 Right
	{
		playerSwitchMask |= 0x0400; //P3 Right
	}
	if(p3buttons & 0x20) //P3 Start
	{
		systemSwitchMask |= 0x0200; //P3 Start
	}
	if(p4buttons & 0x02)
	{
		playerSwitchMask |= 0x0001; //P4 Up
	}
	if(p4buttons & 0x04)
	{
		playerSwitchMask |= 0x0002; //P4 Down
	}
	if(p4buttons & 0x08) //P4 Left
	{
		playerSwitchMask |= 0x0008; //P4 Left
	}
	if(p4buttons & 0x10) //P4 Right
	{
		playerSwitchMask |= 0x0004; //P4 Right
	}
	if(p4buttons & 0x20) //P4 Start
	{
		systemSwitchMask |= 0x0100; //P4 Start
	}
	if(p1buttons2 & 0x01) //Button 1
	{
		m_switchStates[PACMAN_AP_BUTTON_B1] = 0xFF;
	}
	if(p1buttons2 & 0x02) //Button 2
	{
		m_switchStates[PACMAN_AP_BUTTON_B2] = 0xFF;
	}
	if(p1buttons2 & 0x04) //Button 3
	{
		m_switchStates[PACMAN_AP_BUTTON_P1] = 0xFF;
	}
	if(p1buttons2 & 0x08) //Button 4
	{
		m_switchStates[PACMAN_AP_BUTTON_P2] = 0xFF;
	}
	// These aren't needed in any games for now but they're mapped in the pipe
	// so I'll keep them here for future reference
	// if(p1buttons2 & 0x10) //Button 5
	// {
	// 	//m_switchStates[PACMAN_AP_BUTTON_B1] = 0xFF;
	// }
	// if(p1buttons2 & 0x20) //Button 6
	// {
	// 	//m_switchStates[PACMAN_AP_BUTTON_B1] = 0xFF;
	// }

	m_systemSwitchState = ~systemSwitchMask;
	m_playerSwitchState = ~playerSwitchMask;
#endif
}
#endif

void CSys147::SetAxisState(unsigned int padNumber, PS2::CControllerInfo::BUTTON button, uint8 axisValue, uint8* ram)
{
}

void CSys147::Invoke(CMIPS& context, unsigned int functionId)
{
	throw std::runtime_error("Not implemented.");
}

bool CSys147::Invoke000(uint32 method, uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	switch(method)
	{
	default:
		CLog::GetInstance().Warn(LOG_NAME, "Unknown method invoked (0x%08X, 0x%08X).\r\n", 0x000, method);
		break;
	}
	return true;
}

bool CSys147::Invoke001(uint32 method, uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	switch(method)
	{
	default:
		CLog::GetInstance().Warn(LOG_NAME, "Unknown method invoked (0x%08X, 0x%08X).\r\n", 0x001, method);
		break;
	}
	return true;
}

bool CSys147::Invoke002(uint32 method, uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	switch(method)
	{
	default:
		CLog::GetInstance().Warn(LOG_NAME, "Unknown method invoked (0x%08X, 0x%08X).\r\n", 0x002, method);
		break;
	}
	return true;
}

bool CSys147::Invoke003(uint32 method, uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	switch(method)
	{
	case 4:
	{
		//Read RTC
		assert(argsSize == 0x10);
		assert(retSize == 0x20);
		ret[0] = 0x24; //Year
		ret[1] = 0x02; //Month
		ret[2] = 0x07; //Day
		ret[3] = 0x01; //Day of Week (?)
		ret[4] = 0x04; //Hours
		ret[5] = 0x04; //Minutes
		ret[6] = 0x05; //Seconds;
	}
	break;
	default:
		CLog::GetInstance().Warn(LOG_NAME, "Unknown method invoked (0x%08X, 0x%08X).\r\n", 0x003, method);
		break;
	}
	return true;
}

bool CSys147::Invoke200(uint32 method, uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	switch(method)
	{
	case 0:
	{
		//Write
		//0 -> 0x400 -> Data to write
		//0x400 -> Offset
		//0x404 -> Size
		uint32 offset = args[0x100];
		uint32 size = args[0x101];
		assert(size <= 0x400);
		WriteBackupRam(offset, reinterpret_cast<const uint8*>(args), size);
	}
	break;
	default:
		CLog::GetInstance().Warn(LOG_NAME, "Unknown method invoked (0x%08X, 0x%08X).\r\n", 0x200, method);
		break;
	}
	return true;
}

bool CSys147::Invoke201(uint32 method, uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	switch(method)
	{
	case 0:
	{
		//Read
		//0x0 -> Offset
		//0x4 -> Size
		uint32 offset = args[0];
		uint32 size = args[1];
		assert(size <= 0x400);
		ReadBackupRam(offset, reinterpret_cast<uint8*>(ret), size);
	}
	break;
	default:
		CLog::GetInstance().Warn(LOG_NAME, "Unknown method invoked (0x%08X, 0x%08X).\r\n", 0x201, method);
		break;
	}
	return true;
}

bool CSys147::Invoke99(uint32 method, uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	switch(method)
	{
	case 0x02000000:
		//Command 0x02000000
		//Ret 0 -> size (0x40 blocks)
		//Ret 1... -> some data (in blocks of 0x40 bytes)
		CLog::GetInstance().Warn(LOG_NAME, "LINK_02000000();\r\n");
		//Receive Responses?
		{
			uint16 maxPackets = reinterpret_cast<uint16*>(args)[0];
			assert(m_pendingReplies.size() < 0x10000);
			uint16 xferPackets = std::min(maxPackets, static_cast<uint16>(m_pendingReplies.size()));
			auto outputPacket = reinterpret_cast<MODULE_99_PACKET*>(ret + 1);
			for(int i = 0; i < xferPackets; i++)
			{
				memcpy(outputPacket, &m_pendingReplies.front(), sizeof(MODULE_99_PACKET));
				m_pendingReplies.erase(m_pendingReplies.begin());
				outputPacket++;
			}
			reinterpret_cast<uint16*>(ret)[0] = xferPackets;
		}
		break;
	case 0x03004002:
		//Send Command?
		CLog::GetInstance().Warn(LOG_NAME, "LINK_03004002();\r\n");
		{
			assert(argsSize == 0x40);
			auto packet = reinterpret_cast<const MODULE_99_PACKET*>(args);
			CLog::GetInstance().Warn(LOG_NAME, "Recv Command 0x%02X.\r\n", packet->command);

			//0x0D -> Timeout Parameter
			//0x0F -> Get PCB info
			//0x10 -> System Switches (?)
			//0x18 -> Switch
			//0x38 -> SCI
			//0x39 -> ???
			//0x58 -> Mechanical Counter
			//0x48 -> Coin Sensor
			//0x68 -> Hopper

			if(packet->command == 0x0F)
			{
				MODULE_99_PACKET reply = {};

				//Stuff that is displayed
				//00:01 -> ??
				//02:03 -> Version, needs to be 0x104 (MSB first)
				//04

				reply.type = 2;
				reply.command = 0x0F;
				reply.data[2] = 0x01;
				reply.data[3] = 0x04;
				reply.checksum = ComputePacketChecksum(reply);
				m_pendingReplies.emplace_back(reply);
			}
			if(packet->command == 0x48)
			{
				//Coin Sensor
				MODULE_99_PACKET reply = {};
				reply.type = 2;
				reply.command = 0x48;
				reply.data[0] = packet->data[0];
				reply.checksum = ComputePacketChecksum(reply);
				m_pendingReplies.emplace_back(reply);
			}
			else if(packet->command == 0x18)
			{
				//Switch
				MODULE_99_PACKET reply = {};
				reply.type = 2;
				reply.command = 0x18;
				reply.data[0] = packet->data[0];
				reply.checksum = ComputePacketChecksum(reply);
				m_pendingReplies.emplace_back(reply);
			}
			else if(packet->command == 0x58)
			{
				//Mechanical Counter
				MODULE_99_PACKET reply = {};
				reply.type = 2;
				reply.command = 0x58;
				reply.data[0] = packet->data[0];
				reply.checksum = ComputePacketChecksum(reply);
				m_pendingReplies.emplace_back(reply);
			}
			else if(packet->command == 0xD8)
			{
				//???
				MODULE_99_PACKET reply = {};
				reply.type = 2;
				reply.command = 0xD8;
				reply.data[0] = packet->data[0];
				reply.checksum = ComputePacketChecksum(reply);
				m_pendingReplies.emplace_back(reply);
			}
			else if(packet->command == 0x31)
			{
				//Barcode Reader & IC Card
				//data[0] = channel?
				//data[1] = size of data
				//data[4] = start of data
				uint8 channel = packet->data[0];
				uint8 inputSize = packet->data[1];
				CLog::GetInstance().Warn(LOG_NAME, "Received command buffer channel: %d, size: %d\r\n",
				                         channel, inputSize);
				MODULE_99_PACKET reply = {};
				reply.type = 2;
				reply.command = 0x31;
				if(channel == 0)
				{
					assert(inputSize == 4);
					//Barcode Reader
					reply.data[0] = channel;
					reply.data[1] = 0x6; //Data Size
					reply.data[4 + 0] = 0x02;
					reply.data[4 + 1] = 0x02;
					reply.data[4 + 2] = 0x80;
					reply.data[4 + 3] = 0x02; //This is used for result
					reply.data[4 + 4] = 0x03;
					reply.data[4 + 5] = 0x03;
				}
				else if(channel == 2)
				{
					ProcessIcCard(reply, *packet);
				}
				else
				{
					assert(false);
				}
				reply.checksum = ComputePacketChecksum(reply);
				m_pendingReplies.emplace_back(reply);
			}
			else if(packet->command == 0x39)
			{
//Seems to be related to switch values?
#ifdef TEKNOPARROT
				GetTpUiButtons();
#endif
				for(const auto& switchState : m_switchStates)
				{
					MODULE_99_PACKET reply = {};
					reply.type = 2;
					reply.command = 0x39;
					reply.data[0] = switchState.first;
					reply.data[4] = switchState.second;
					reply.checksum = ComputePacketChecksum(reply);
					m_pendingReplies.emplace_back(reply);
				}

				{
					MODULE_99_PACKET reply = {};
					reply.type = 2;
					reply.command = 0x31;
					//Barcode Reader
					reply.data[0] = 0;
					reply.data[1] = 6; //Data Size

					reply.data[4 + 0] = 0x02;
					reply.data[4 + 1] = 0x02;
					reply.data[4 + 2] = 0x80;
					reply.data[4 + 3] = 0x02; //This is used for result
					reply.data[4 + 4] = 0x03;
					reply.data[4 + 5] = 0x03;

					//Barcode Payload
					{
						std::unique_lock barcodeMutexLock(m_barcodeMutex);
						if(m_currentBarcode.size() == 8)
						{
							reply.data[1] += 10; //Add more data to payload

							//0x02 = Header
							//8 bytes of data (0x62,0x6E,0x30,0x63,0x30,0x7D,0x58,0x31)
							//0x03 = Footer

							reply.data[4 + 6] = 0x02;
							for(int i = 0; i < 8; i++)
							{
								reply.data[4 + 7 + i] = m_currentBarcode[i];
							}
							reply.data[4 + 15] = 0x03;
						}
						m_currentBarcode.clear();
					}

					m_pendingReplies.emplace_back(reply);
				}
			}
			else if(packet->command == 0x0D)
			{
				//???
				MODULE_99_PACKET reply = {};
				reply.type = 2;
				reply.command = 0x0D;
				reply.data[0] = packet->data[0];
				reply.checksum = ComputePacketChecksum(reply);
				m_pendingReplies.emplace_back(reply);
			}
			else if(packet->command == 0x38)
			{
				//SCI
				MODULE_99_PACKET reply = {};
				reply.type = 2;
				reply.command = 0x38;
				reply.data[0] = packet->data[0];
				reply.checksum = ComputePacketChecksum(reply);
				m_pendingReplies.emplace_back(reply);
			}
			else if(packet->command == 0x10)
			{
				//Some kind of I/O device related response
				//Animal Kaiser uses this for dispenser
				//Pac Man Battle Royale uses this for switch state
				if(m_ioMode == IO_MODE::AI)
				{
#ifdef TEKNOPARROT
					GetTpUiButtons();
#endif
					{
						MODULE_99_PACKET reply = {};
						reply.type = 2;
						reply.command = 0x10;
						reply.data[0] = static_cast<uint8>(m_systemSwitchState);
						reply.data[1] = static_cast<uint8>(m_systemSwitchState >> 8);
						reply.checksum = ComputePacketChecksum(reply);
						m_pendingReplies.emplace_back(reply);
					}

					{
						MODULE_99_PACKET reply = {};
						reply.type = 3;
						reply.command = 0x10;
						reply.data[0x32] = 0x20;
						reply.data[0x36] = static_cast<uint8>(m_playerSwitchState);
						reply.data[0x37] = static_cast<uint8>(m_playerSwitchState >> 8);
						reply.checksum = ComputePacketChecksum(reply);
						m_pendingReplies.emplace_back(reply);
					}
				}
				else
				{
					MODULE_99_PACKET reply = {};
					reply.type = 2;
					reply.command = 0x10;
					reply.data[0] = packet->data[0];
					reply.checksum = ComputePacketChecksum(reply);
					m_pendingReplies.emplace_back(reply);
				}
			}
			else if(packet->command == 0x41)
			{
#ifdef TEKNOPARROT
				GetTpUiButtons();
#endif
				{
					MODULE_99_PACKET reply = {};
					reply.type = 2;
					reply.command = 0x41;
					reply.data[0] = 0x11;             // flags 1, slot 1 (unsure what the flags do, maybe coin/card/whatever?)
					reply.data[1] = 0x00;             // slot status (0 == no error)
					reply.data[2] = m_currentCredits; // amount of credits inserted, delta
					reply.checksum = ComputePacketChecksum(reply);
					m_pendingReplies.emplace_back(reply);
					m_currentCredits = 0; // reset credits after sending
				}
			}
			reinterpret_cast<uint16*>(ret)[0] = 1;
		}
		break;
	case 0x08000000:
		CLog::GetInstance().Warn(LOG_NAME, "LINK_08000000();\r\n");
		reinterpret_cast<uint16*>(ret)[0] = 1;
		break;
	case 0x09000002:
		CLog::GetInstance().Warn(LOG_NAME, "LINK_09000002();\r\n");
		reinterpret_cast<uint16*>(ret)[0] = 0;
		break;
	case 0x09000003:
		CLog::GetInstance().Warn(LOG_NAME, "LINK_09000003();\r\n");
		reinterpret_cast<uint16*>(ret)[0] = 0;
		break;
	case 0x0C000002:
		CLog::GetInstance().Warn(LOG_NAME, "LINK_0C000002();\r\n");
		reinterpret_cast<uint16*>(ret)[0] = 0;
		break;
	case 0x0C000003:
		CLog::GetInstance().Warn(LOG_NAME, "LINK_0C000003();\r\n");
		reinterpret_cast<uint16*>(ret)[0] = 0;
		break;
	case 0x0D003803:
		CLog::GetInstance().Warn(LOG_NAME, "LINK_0D003803();\r\n");
		reinterpret_cast<uint16*>(ret)[0] = 0;
		break;
	case 0x11000000:
		CLog::GetInstance().Warn(LOG_NAME, "LINK_11000000();\r\n");
		reinterpret_cast<uint16*>(ret)[0] = 0;
		break;
	default:
		reinterpret_cast<uint16*>(ret)[0] = 1;
		CLog::GetInstance().Warn(LOG_NAME, "Unknown method invoked (0x%08X, 0x%08X).\r\n", 0x99, method);
		break;
	}
	return true;
}

void CSys147::ProcessIcCard(MODULE_99_PACKET& output, const MODULE_99_PACKET& input)
{
	uint32 channel = input.data[0];
	assert(channel == 2);
	uint32 dataSize = input.data[1];
	uint32 commandId = input.data[5];
	uint32 replySize = 0;
	static const uint32 replyBase = 4;
	output.data[replyBase + 0] = 0x02;
	output.data[replyBase + 1] = commandId; //Command ID
	switch(commandId)
	{
	case 0x78:
	{
		//??
		assert(dataSize == 0x07);
		replySize = 6;
		output.data[replyBase + 3] = 0x00; //Command result?
		output.data[replyBase + 4] = 0x00;
		output.data[replyBase + 5] = 0xFF;
	}
	break;
	case 0x7A:
	{
		//Ok next?
		assert(dataSize == 0x12);
		replySize = 4;
		output.data[replyBase + 3] = 0x00; //Command result?
	}
	break;
	case 0x7B:
	{
		assert(dataSize == 0x05);
		replySize = 6;
		output.data[replyBase + 3] = 0x00; //Command result? (0x03 -> No IC card)
		output.data[replyBase + 4] = 0x00; //??
		output.data[replyBase + 5] = 0x00; //??
	}
	break;
	case 0x80:
	{
		//??
		assert(dataSize == 0x05);
		replySize = 4;
		output.data[replyBase + 3] = 0x0D; //Command result?
	}
	break;
	case 0x9F:
	{
		//??
		assert(dataSize == 0x0D);
		replySize = 4;
		output.data[replyBase + 3] = 0x00; //Command result?
	}
	break;
	case 0xA7:
	{
		//Init?
		assert(dataSize == 0x0D);
		replySize = 4;
		output.data[replyBase + 3] = 0x00; //Command result?
	}
	break;
	case 0xAC:
	{
		//Some key stuff (parity)? (game expects 8 bytes in payload)
		assert(dataSize == 0x05);
		replySize = 12;
		output.data[replyBase + 3] = 0x00; //Command result?
		output.data[replyBase + 4] = 0xAA;
		output.data[replyBase + 5] = 0xAA;
		output.data[replyBase + 6] = 0xAA;
		output.data[replyBase + 7] = 0xAA;
		output.data[replyBase + 8] = 0x55;
		output.data[replyBase + 9] = 0x55;
		output.data[replyBase + 10] = 0x55;
		output.data[replyBase + 11] = 0x55;
	}
	break;
	case 0xAF:
	{
		//More key stuff (checksum)? (game expects 16 bytes in payload)
		assert(dataSize == 0x05);
		replySize = 20;
		output.data[replyBase + 3] = 0x00; //Command result?
	}
	break;
	default:
		assert(false);
		break;
	}
	uint8 replySizePlusParity = replySize + 1;
	assert(replySizePlusParity >= 5);
	output.data[0] = channel;
	output.data[1] = replySizePlusParity; //This needs to be at least 5
	output.data[replyBase + 2] = replySizePlusParity - 5;
	uint8 parity = 0;
	for(int i = 1; i < replySize; i++)
	{
		parity ^= output.data[replyBase + i];
	}
	output.data[replyBase + replySize] = parity;
}

uint8 CSys147::ComputePacketChecksum(const MODULE_99_PACKET& packet)
{
	uint8 checksum = 0;
	for(const auto& value : packet.data)
	{
		checksum += value;
	}
	return checksum;
}

//TODO: This is copied from Sys246/256. Move this somewhere else

fs::path GetArcadeSavePath()
{
	return CAppConfig::GetInstance().GetBasePath() / fs::path("arcadesaves");
}

void CSys147::ReadBackupRam(uint32 backupRamAddr, uint8* buffer, uint32 size)
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

void CSys147::WriteBackupRam(uint32 backupRamAddr, const uint8* buffer, uint32 size)
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

void CSys147::HandleIoServerRequest(const Framework::CHttpServer::Request& request)
{
	if(request.url.find("/sys147/barcode") == 0)
	{
		std::unique_lock barcodeMutexLock(m_barcodeMutex);
		m_currentBarcode = std::string(reinterpret_cast<const char*>(request.body.data()), request.body.size());
	}
}
