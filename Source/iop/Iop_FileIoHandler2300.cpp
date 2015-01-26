#include "Iop_FileIoHandler2300.h"
#include "Iop_Ioman.h"
#include "../Log.h"

#define LOG_NAME ("iop_fileio")

using namespace Iop;

CFileIoHandler2300::CFileIoHandler2300(CIoman* ioman, CSifMan& sifMan)
: CHandler(ioman)
, m_sifMan(sifMan)
{
	memset(m_resultPtr, 0, sizeof(m_resultPtr));
}

void CFileIoHandler2300::Invoke(uint32 method, uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	switch(method)
	{
	case 0:
		{
			assert(retSize == 4);
			auto command = reinterpret_cast<OPENCOMMAND*>(args);
			*ret = m_ioman->Open(command->flags, command->fileName);

			//Send response
			if(m_resultPtr[0] != 0)
			{
				OPENREPLY reply;
				reply.header.commandId = 0;
				CopyHeader(reply.header, command->header);
				reply.result = *ret;
				reply.unknown2 = 0;
				reply.unknown3 = 0;
				reply.unknown4 = 0;
				memcpy(ram + m_resultPtr[0], &reply, sizeof(OPENREPLY));
			}

			{
				size_t packetSize = sizeof(SIFCMDHEADER);
				uint8* callbackPacket = reinterpret_cast<uint8*>(alloca(packetSize));
				auto header = reinterpret_cast<SIFCMDHEADER*>(callbackPacket);
				header->commandId = 0x80000011;
				header->size = packetSize;
				header->dest = 0;
				header->optional = 0;
				m_sifMan.SendPacket(callbackPacket, packetSize);
			}
		}
		break;
	case 1:
		{
			assert(retSize == 4);
			auto command = reinterpret_cast<CLOSECOMMAND*>(args);
			*ret = m_ioman->Close(command->fd);
			//Send response
			{
				CLOSEREPLY reply;
				reply.header.commandId = 1;
				CopyHeader(reply.header, command->header);
				reply.result = *ret;
				reply.unknown2 = 0;
				reply.unknown3 = 0;
				reply.unknown4 = 0;
				memcpy(ram + m_resultPtr[0], &reply, sizeof(reply));
			}

			{
				size_t packetSize = sizeof(SIFCMDHEADER);
				uint8* callbackPacket = reinterpret_cast<uint8*>(alloca(packetSize));
				auto header = reinterpret_cast<SIFCMDHEADER*>(callbackPacket);
				header->commandId = 0x80000011;
				header->size = packetSize;
				header->dest = 0;
				header->optional = 0;
				m_sifMan.SendPacket(callbackPacket, packetSize);
			}
		}
		break;
	case 2:
		{
			assert(retSize == 4);
			auto command = reinterpret_cast<READCOMMAND*>(args);
			*ret = m_ioman->Read(command->fd, command->size, reinterpret_cast<void*>(ram + command->buffer));
			//Send response
			{
				READREPLY reply;
				reply.header.commandId = 2;
				CopyHeader(reply.header, command->header);
				reply.result = *ret;
				reply.unknown2 = 0;
				reply.unknown3 = 0;
				reply.unknown4 = 0;
				memcpy(ram + m_resultPtr[0], &reply, sizeof(reply));
			}

			{
				size_t packetSize = sizeof(SIFCMDHEADER);
				uint8* callbackPacket = reinterpret_cast<uint8*>(alloca(packetSize));
				auto header = reinterpret_cast<SIFCMDHEADER*>(callbackPacket);
				header->commandId = 0x80000011;
				header->size = packetSize;
				header->dest = 0;
				header->optional = 0;
				m_sifMan.SendPacket(callbackPacket, packetSize);
			}
		}
		break;
	case 4:
		{
			assert(retSize == 4);
			auto command = reinterpret_cast<SEEKCOMMAND*>(args);
			*ret = m_ioman->Seek(command->fd, command->offset, command->whence);

			//Send response
			{
				SEEKREPLY reply;
				reply.header.commandId = 4;
				CopyHeader(reply.header, command->header);
				reply.result = *ret;
				reply.unknown2 = 0;
				reply.unknown3 = 0;
				reply.unknown4 = 0;
				memcpy(ram + m_resultPtr[0], &reply, sizeof(reply));
			}

			{
				size_t packetSize = sizeof(SIFCMDHEADER);
				uint8* callbackPacket = reinterpret_cast<uint8*>(alloca(packetSize));
				auto header = reinterpret_cast<SIFCMDHEADER*>(callbackPacket);
				header->commandId = 0x80000011;
				header->size = packetSize;
				header->dest = 0;
				header->optional = 0;
				m_sifMan.SendPacket(callbackPacket, packetSize);
			}
		}
		break;
	case 23:
		{
			//No idea what that does...
			assert(retSize == 4);
			auto command = reinterpret_cast<ACTIVATECOMMAND*>(args);
			*ret = 0;

			{
				//Send response
				ACTIVATEREPLY reply;
				reply.header.commandId = 23;
				CopyHeader(reply.header, command->header);
				reply.result = *ret;
				reply.unknown2 = 0;
				reply.unknown3 = 0;
				reply.unknown4 = 0;
				memcpy(ram + m_resultPtr[0], &reply, sizeof(reply));
			}

			{
				size_t packetSize = sizeof(SIFCMDHEADER);
				uint8* callbackPacket = reinterpret_cast<uint8*>(alloca(packetSize));
				auto header = reinterpret_cast<SIFCMDHEADER*>(callbackPacket);
				header->commandId = 0x80000011;
				header->size = packetSize;
				header->dest = 0;
				header->optional = 0;
				m_sifMan.SendPacket(callbackPacket, packetSize);
			}
		}
		break;
	case 255:
		//Not really sure about that...
		if(retSize == 8)
		{
			memcpy(ret, "....rawr", 8);
		}
		else if(retSize == 4)
		{
			memcpy(ret, "....", 4);
		}
		else
		{
			assert(0);
		}
		m_resultPtr[0] = args[0];
		m_resultPtr[1] = args[1];
		break;
	default:
		CLog::GetInstance().Print(LOG_NAME, "Unknown function (%d) called.\r\n", method);
		break;
	}
}

void CFileIoHandler2300::CopyHeader(REPLYHEADER& reply, const COMMANDHEADER& command)
{
	reply.semaphoreId = command.semaphoreId;
	reply.resultPtr = command.resultPtr;
	reply.resultSize = command.resultSize;
}
