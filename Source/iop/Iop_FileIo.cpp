#include "Iop_FileIo.h"
#include "Iop_Ioman.h"
#include "../Log.h"
#include "../SifDefs.h"

#define LOG_NAME ("iop_fileio")

using namespace Iop;
using namespace std;

CFileIo::CFileIo(CSifMan& sifMan, CIoman& ioman) :
m_fileIoHandler(NULL),
m_sifMan(sifMan),
m_ioman(ioman)
{
    m_sifMan.RegisterModule(SIF_MODULE_ID, this);
}

CFileIo::~CFileIo()
{
    if(m_fileIoHandler != NULL)
    {
        delete m_fileIoHandler;
    }
}

string CFileIo::GetId() const
{
    return "fileio";
}

string CFileIo::GetFunctionName(unsigned int) const
{
    return "unknown";
}

//IOP Invoke
void CFileIo::Invoke(CMIPS& context, unsigned int functionId)
{
    throw exception();
}

//SIF Invoke
bool CFileIo::Invoke(uint32 method, uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
    if(m_fileIoHandler == NULL)
    {
        if(method == 0xFF)
        {
            m_fileIoHandler = new CFileIoHandler3100(&m_ioman, m_sifMan);
        }
        else
        {
            m_fileIoHandler = new CFileIoHandlerBasic(&m_ioman);
        }
    }
    m_fileIoHandler->Invoke(method, args, argsSize, ret, retSize, ram);
    return true;
}

//--------------------------------------------------
// CFileIoHandler
//--------------------------------------------------

CFileIo::CFileIoHandler::CFileIoHandler(CIoman* ioman) :
m_ioman(ioman)
{

}

//--------------------------------------------------
// CFileIoHandlerBasic
//--------------------------------------------------

CFileIo::CFileIoHandlerBasic::CFileIoHandlerBasic(CIoman* ioman) :
CFileIoHandler(ioman)
{

}

void CFileIo::CFileIoHandlerBasic::Invoke(uint32 method, uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
    switch(method)
    {
    case 0:
        assert(retSize == 4);
        *ret = m_ioman->Open(*args, reinterpret_cast<char*>(args) + 4);
        break;
	case 1:
        assert(retSize == 4);
        *ret = m_ioman->Close(*args);
		break;
    case 2:
		assert(retSize == 4);
		*ret = m_ioman->Read(args[0], args[2], reinterpret_cast<void*>(ram + args[1]));
        break;
	case 3:
		assert(retSize == 4);
        *ret = m_ioman->Write(args[0], args[2], reinterpret_cast<void*>(ram + args[1]));
		break;
    case 4:
        assert(retSize == 4);
        *ret = m_ioman->Seek(args[0], args[1], args[2]);
        break;
    default:
        CLog::GetInstance().Print(LOG_NAME, "Unknown function (%d) called.\r\n", method);
        break;
    }
}

//--------------------------------------------------
// CFileIoHandler3100
//--------------------------------------------------

CFileIo::CFileIoHandler3100::CFileIoHandler3100(CIoman* ioman, CSifMan& sifMan) :
CFileIoHandler(ioman),
m_sifMan(sifMan)
{

}

void CFileIo::CFileIoHandler3100::Invoke(uint32 method, uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
    static uint32 resultPtr[2];
    switch(method)
    {
    case 0:
        {
            assert(retSize == 4);
            OPENCOMMAND* command = reinterpret_cast<OPENCOMMAND*>(args);
            *ret = m_ioman->Open(command->flags, command->fileName);

            //Send response
            {
                OPENREPLY reply;
                reply.header.commandId = 0;
                CopyHeader(reply.header, command->header);
                reply.result = *ret;
                reply.unknown2 = 0;
                reply.unknown3 = 0;
                reply.unknown4 = 0;
                memcpy(ram + resultPtr[0], &reply, sizeof(OPENREPLY));
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
            CLOSECOMMAND* command = reinterpret_cast<CLOSECOMMAND*>(args);
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
                memcpy(ram + resultPtr[0], &reply, sizeof(reply));
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
            READCOMMAND* command = reinterpret_cast<READCOMMAND*>(args);
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
                memcpy(ram + resultPtr[0], &reply, sizeof(reply));
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
            SEEKCOMMAND* command = reinterpret_cast<SEEKCOMMAND*>(args);
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
                memcpy(ram + resultPtr[0], &reply, sizeof(reply));
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
            ACTIVATECOMMAND* command = reinterpret_cast<ACTIVATECOMMAND*>(args);
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
                memcpy(ram + resultPtr[0], &reply, sizeof(reply));
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
        resultPtr[0] = args[0];
        resultPtr[1] = args[1];
        break;
    default:
        CLog::GetInstance().Print(LOG_NAME, "Unknown function (%d) called.\r\n", method);
        break;
    }
}

void CFileIo::CFileIoHandler3100::CopyHeader(REPLYHEADER& reply, const COMMANDHEADER& command)
{
    reply.semaphoreId = command.semaphoreId;
    reply.resultPtr = command.resultPtr;
    reply.resultSize = command.resultSize;
}
