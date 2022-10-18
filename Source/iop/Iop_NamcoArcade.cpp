#include "Iop_NamcoArcade.h"
#include <cstring>
#include "../Log.h"

using namespace Iop;

#define LOG_NAME "iop_namcoarcade"

CNamcoArcade::CNamcoArcade(CSifMan& sif)
{
	m_module001 = CSifModuleAdapter(std::bind(&CNamcoArcade::Invoke001, this,
											  std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));
	m_module003 = CSifModuleAdapter(std::bind(&CNamcoArcade::Invoke003, this,
											  std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));
	m_module004 = CSifModuleAdapter(std::bind(&CNamcoArcade::Invoke004, this,
											  std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));

	sif.RegisterModule(MODULE_ID_1, &m_module001);
	sif.RegisterModule(MODULE_ID_3, &m_module003);
	sif.RegisterModule(MODULE_ID_4, &m_module004);
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

//Ref: http://daifukkat.su/files/jvs_wip.pdf
enum
{
	JVS_SYNC = 0xE0,
	
	JVS_CMD_IOIDENT = 0x10,
	JVS_CMD_CMDREV = 0x11,
	JVS_CMD_JVSREV = 0x12,
	JVS_CMD_COMMVER = 0x13,
	JVS_CMD_FEATCHK = 0x14,
	JVS_CMD_RESET = 0xF0,
	JVS_CMD_SETADDR = 0xF1,
};

void ProcessJvsPacket(const uint8* input, uint8* output)
{
	assert(*input == JVS_SYNC);
	input++;
	uint8 inDest = *input++;
	uint8 inSize = *input++;
	uint8 outSize = 0;
	uint32 inWorkChecksum = inDest + inSize;
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
				
				(*output++) = 'B';
				(*output++) = 'L';
				(*output++) = 'A';
				(*output++) = 'H';
				(*output++) = 0;

				(*dstSize) += 6;
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
				(*output++) = 0x00; //No features here.
				(*dstSize) += 2;
			}
			break;
		default:
			//Unknown command
			assert(false);
			break;
		}
	}
	uint8 inChecksum = (*input);
	assert(inChecksum == (inWorkChecksum & 0xFF));
}

void CNamcoArcade::SetButtonState(unsigned int padNumber, PS2::CControllerInfo::BUTTON button, bool pressed, uint8* ram)
{
	//For Ridge Racer V (coin button)
	//if(pressed && (g_recvAddr != 0))
	//{
	//	*reinterpret_cast<uint16*>(ram + g_recvAddr + 0xC0) += 1;
	//}
	static int delay = 0;
	if((delay == 0x1) && m_recvAddr && m_sendAddr)
	{
		delay = 0;
		auto sendData = reinterpret_cast<const uint16*>(ram + m_sendAddr);
		auto recvData = reinterpret_cast<uint16*>(ram + m_recvAddr);
		recvData[0] = sendData[0];
		if(sendData[0] == 0x3E6F)
		{
			//sendData + 0x18 (bytes) -> some command id
			//sendData + 0x22 (bytes) -> Start of packet
			//recvData + 0x40 (needs to be the same command id)
			//recvData + 0x5A packet starts here
			recvData[1] = 0x100; //JVS version
			uint16 pktId = sendData[0x0C];
			if(pktId != 0)
			{
				CLog::GetInstance().Warn(LOG_NAME, "PktId: 0x%04X\r\n", pktId);
				ProcessJvsPacket(reinterpret_cast<const uint8*>(sendData) + 0x22, reinterpret_cast<uint8*>(recvData) + 0x5A);
				recvData[0x20] = pktId;
			}
		}
	}
	delay++;
}

void CNamcoArcade::SetAxisState(unsigned int padNumber, PS2::CControllerInfo::BUTTON button, uint8 axisValue, uint8* ram)
{
}

static const uint32_t backupRamSize = 0x4000000;
uint8 backupRam[backupRamSize];

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
				CLog::GetInstance().Warn(LOG_NAME, "ReadBackupRam(ramAddr = 0x%08X, dstPtr = 0x%08X, size = %d);\r\n",
										 ramAddr, dstPtr, size);
				if(ramAddr >= 0x40000000 && ramAddr < 0x50000000)
				{
					uint32 offset = ramAddr - 0x40000000;
					assert(offset < backupRamSize);
					memcpy(ram + dstPtr, backupRam + offset, size);
				}
				ret[0] = 0;
				ret[1] = size;
			}
			//Sengoku Basara & Time Crisis have argsSize == 0x10
			else if(argsSize == 0x10)
			{
				assert(argsSize >= 0x10);
				uint32 infoPtr = args[2];
				//CLog::GetInstance().Warn(LOG_NAME, "cmd2[_0 = 0x%08X, _1 = 0x%08X, _2 = 0x%08X, _3 = 0x%08X];\r\n",
				//						 args[0], args[1], args[2], args[3]);
				uint32* info = reinterpret_cast<uint32*>(ram + infoPtr);
				CLog::GetInstance().Warn(LOG_NAME, "jvioboot?(infoPtr = 0x%08X);\r\n", infoPtr);
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
						recvData[1] = 0x208; //firmware version?
						recvData[0x14] = rootPktId; //Xored with value at 0x10 in send packet, needs to be the same
						recvData[0x21] = sendData[0x0D];
						uint16 pktId = sendData[0x0C];
						if(pktId != 0)
						{
							CLog::GetInstance().Warn(LOG_NAME, "PktId: 0x%04X\r\n", pktId);
							ProcessJvsPacket(reinterpret_cast<const uint8*>(sendData) + 0x22, reinterpret_cast<uint8*>(recvData) + 0x5A);
							recvData[0x20] = pktId;
						}
					}
				}
				else
				{
					CLog::GetInstance().Warn(LOG_NAME, "Unknown Packet\r\n");
					CLog::GetInstance().Warn(LOG_NAME, "info[_0 = 0x%08X, _1 = 0x%08X, _2 = 0x%08X, _3 = 0x%08X];\r\n",
											 info[0], info[1], info[2], info[3]);
					CLog::GetInstance().Warn(LOG_NAME, "------------------------------------------\r\n");
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
			CLog::GetInstance().Warn(LOG_NAME, "WriteBackupRam(ramAddr = 0x%08X, srcPtr = 0x%08X, size = %d);\r\n",
									 ramAddr, srcPtr, size);
			if(ramAddr >= 0x40000000 && ramAddr < 0x50000000)
			{
				uint32 offset = ramAddr - 0x40000000;
				assert(offset < backupRamSize);
				memcpy(backupRam + offset, ram + srcPtr, size);
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
