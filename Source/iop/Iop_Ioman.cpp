#include "../AppConfig.h"
#include "../SIF.h"
#include "Iop_Ioman.h"
#include "StdStream.h"
#include <stdexcept>

using namespace Iop;
using namespace std;
using namespace Framework;

#define PREF_IOP_FILEIO_STDLOGGING ("iop.fileio.stdlogging")

CIoman::CIoman(uint8* ram, CSifMan& sifMan) :
m_ram(ram),
m_sifMan(sifMan),
m_fileIoHandler(NULL),
m_nextFileHandle(3)
{
	CAppConfig::GetInstance().RegisterPreferenceBoolean(PREF_IOP_FILEIO_STDLOGGING, false);
    m_sifMan.RegisterModule(SIF_MODULE_ID, this);

	//Insert standard files if requested.
	if(CAppConfig::GetInstance().GetPreferenceBoolean(PREF_IOP_FILEIO_STDLOGGING))
	{
		try
		{
			m_files[1] = new CStdStream(fopen("ps2_stdout.txt", "ab"));
			m_files[2] = new CStdStream(fopen("ps2_stderr.txt", "ab"));
		}
		catch(...)
		{
			//Humm, some error occured when opening these files...
		}
	}
}

CIoman::~CIoman()
{
    for(FileMapType::iterator fileIterator(m_files.begin());
        m_files.end() != fileIterator; fileIterator++)
    {
        delete fileIterator->second;
    }
	m_devices.clear();
    if(m_fileIoHandler != NULL)
    {
        delete m_fileIoHandler;
    }
}

std::string CIoman::GetId() const
{
    return "ioman";
}

void CIoman::RegisterDevice(const char* name, const DevicePtr& device)
{
    m_devices[name] = device;
}

uint32 CIoman::Open(uint32 flags, const char* path)
{
    uint32 handle = 0xFFFFFFFF;
    try
    {
        string fullPath(path);
        string::size_type position = fullPath.find(":");
        if(position == string::npos) 
        {
            throw runtime_error("Invalid path.");
        }
        string deviceName(fullPath.begin(), fullPath.begin() + position);
        string devicePath(fullPath.begin() + position + 1, fullPath.end());
        DeviceMapType::iterator device(m_devices.find(deviceName));
        if(device == m_devices.end())
        {
            throw runtime_error("Device not found.");
        }
        CStream* stream = device->second->GetFile(flags, devicePath.c_str());
        if(stream == NULL)
        {
            throw runtime_error("File not found.");
        }
        handle = m_nextFileHandle++;
        m_files[handle] = stream;
    }
    catch(const exception& except)
    {
        printf("%s: Error occured while trying to open file : %s\r\n", __FUNCTION__, except.what());
    }
    return handle;
}

uint32 CIoman::Close(uint32 handle)
{
    uint32 result = 0xFFFFFFFF;
    try
    {
        FileMapType::iterator file(m_files.find(handle));
        if(file == m_files.end())
        {
            throw runtime_error("Invalid file handle.");
        }
        delete file->second;
        m_files.erase(file);
        result = 0;
    }
    catch(const exception& except)
    {
        printf("%s: Error occured while trying to close file : %s\r\n", __FUNCTION__, except.what());
    }
    return result;
}

uint32 CIoman::Read(uint32 handle, uint32 size, void* buffer)
{
    uint32 result = 0xFFFFFFFF;
    try
    {
        CStream* stream = GetFileStream(handle);
        result = static_cast<uint32>(stream->Read(buffer, size));
    }
    catch(const exception& except)
    {
        printf("%s: Error occured while trying to read file : %s\r\n", __FUNCTION__, except.what());
    }
    return result;
}

uint32 CIoman::Write(uint32 handle, uint32 size, void* buffer)
{
    uint32 result = 0xFFFFFFFF;
    try
    {
        CStream* stream = GetFileStream(handle);
        result = static_cast<uint32>(stream->Write(buffer, size));
    }
    catch(const exception& except)
    {
        printf("%s: Error occured while trying to write file : %s\r\n", __FUNCTION__, except.what());
    }
    return result;
}

uint32 CIoman::Seek(uint32 handle, uint32 position, uint32 whence)
{
    uint32 result = 0xFFFFFFFF;
    try
    {
        CStream* stream = GetFileStream(handle);
        switch(whence)
        {
        case 0:
            whence = Framework::STREAM_SEEK_SET;
            break;
        case 1:
            whence = Framework::STREAM_SEEK_CUR;
            break;
        case 2:
            whence = Framework::STREAM_SEEK_END;
            break;
        }

        stream->Seek(position, static_cast<STREAM_SEEK_DIRECTION>(whence));
        result = static_cast<uint32>(stream->Tell());
    }
    catch(const exception& except)
    {
        printf("%s: Error occured while trying to seek file : %s\r\n", __FUNCTION__, except.what());
    }
    return result;
}

uint32 CIoman::DelDrv(const char* deviceName)
{
    return -1;
}

CStream* CIoman::GetFileStream(uint32 handle)
{
    FileMapType::iterator file(m_files.find(handle));
    if(file == m_files.end())
    {
        throw runtime_error("Invalid file handle.");
    }
    return file->second;
}

//IOP Invoke
void CIoman::Invoke(CMIPS& context, unsigned int functionId)
{
    switch(functionId)
    {
    case 4:
        context.m_State.nGPR[CMIPS::V0].nD0 = static_cast<int32>(Open(
            context.m_State.nGPR[CMIPS::A1].nV[0],
            reinterpret_cast<char*>(&m_ram[context.m_State.nGPR[CMIPS::A0].nV[0]])
            ));
        break;
    case 5:
        context.m_State.nGPR[CMIPS::V0].nD0 = static_cast<int32>(Close(
            context.m_State.nGPR[CMIPS::A0].nV[0]
            ));
        break;
    case 6:
        context.m_State.nGPR[CMIPS::V0].nD0 = static_cast<int32>(Read(
            context.m_State.nGPR[CMIPS::A0].nV[0],
            context.m_State.nGPR[CMIPS::A2].nV[0],
            &m_ram[context.m_State.nGPR[CMIPS::A1].nV[0]]
            ));
        break;
    case 8:
        context.m_State.nGPR[CMIPS::V0].nD0 = static_cast<int32>(Seek(
            context.m_State.nGPR[CMIPS::A0].nV[0],
            context.m_State.nGPR[CMIPS::A1].nV[0],
            context.m_State.nGPR[CMIPS::A2].nV[0]));
        break;
    case 21:
        context.m_State.nGPR[CMIPS::V0].nD0 = static_cast<int32>(DelDrv(
            reinterpret_cast<char*>(&m_ram[context.m_State.nGPR[CMIPS::A0].nD0])
        ));
        break;
    default:
        printf("%s(%0.8X): Unknown function (%d) called.\r\n", __FUNCTION__, context.m_State.nPC, functionId);
        break;
    }
}

//SIF Invoke
void CIoman::Invoke(uint32 method, uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
    if(m_fileIoHandler == NULL)
    {
        if(method == 0xFF)
        {
            m_fileIoHandler = new CFileIoHandler3100(this, m_sifMan);
        }
        else
        {
            m_fileIoHandler = new CFileIoHandlerBasic(this);
        }
    }
    m_fileIoHandler->Invoke(method, args, argsSize, ret, retSize, ram);
}

//--------------------------------------------------
// CFile
//--------------------------------------------------

CIoman::CFile::CFile(uint32 handle, CIoman& ioman) :
m_handle(handle),
m_ioman(ioman)
{

}

CIoman::CFile::~CFile()
{
    m_ioman.Close(m_handle);
}

CIoman::CFile::operator uint32()
{
    return m_handle;
}

//--------------------------------------------------
// CFileIoHandler
//--------------------------------------------------

CIoman::CFileIoHandler::CFileIoHandler(CIoman* ioman) :
m_ioman(ioman)
{

}

//--------------------------------------------------
// CFileIoHandlerBasic
//--------------------------------------------------

CIoman::CFileIoHandlerBasic::CFileIoHandlerBasic(CIoman* ioman) :
CFileIoHandler(ioman)
{

}

void CIoman::CFileIoHandlerBasic::Invoke(uint32 method, uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
    switch(method)
    {
    case 0:
        assert(retSize == 4);
        *ret = m_ioman->Open(*args, reinterpret_cast<char*>(args) + 4);
        break;
	case 1:
        assert(0);
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
        printf("%s: Unknown function (%d) called.\r\n", __FUNCTION__, method);
        break;
    }
}

//--------------------------------------------------
// CFileIoHandler3100
//--------------------------------------------------

CIoman::CFileIoHandler3100::CFileIoHandler3100(CIoman* ioman, CSifMan& sifMan) :
CFileIoHandler(ioman),
m_sifMan(sifMan)
{

}

void CIoman::CFileIoHandler3100::Invoke(uint32 method, uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
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
                size_t packetSize = sizeof(CSIF::PACKETHDR);
                uint8* callbackPacket = reinterpret_cast<uint8*>(alloca(packetSize));
                CSIF::PACKETHDR* header = reinterpret_cast<CSIF::PACKETHDR*>(callbackPacket);
                header->nCID = 0x80000011;
                header->nSize = packetSize;
                header->nDest = 0;
                header->nOptional = 0;
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
                size_t packetSize = sizeof(CSIF::PACKETHDR);
                uint8* callbackPacket = reinterpret_cast<uint8*>(alloca(packetSize));
                CSIF::PACKETHDR* header = reinterpret_cast<CSIF::PACKETHDR*>(callbackPacket);
                header->nCID = 0x80000011;
                header->nSize = packetSize;
                header->nDest = 0;
                header->nOptional = 0;
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
                size_t packetSize = sizeof(CSIF::PACKETHDR);
                uint8* callbackPacket = reinterpret_cast<uint8*>(alloca(packetSize));
                CSIF::PACKETHDR* header = reinterpret_cast<CSIF::PACKETHDR*>(callbackPacket);
                header->nCID = 0x80000011;
                header->nSize = packetSize;
                header->nDest = 0;
                header->nOptional = 0;
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
                size_t packetSize = sizeof(CSIF::PACKETHDR);
                uint8* callbackPacket = reinterpret_cast<uint8*>(alloca(packetSize));
                CSIF::PACKETHDR* header = reinterpret_cast<CSIF::PACKETHDR*>(callbackPacket);
                header->nCID = 0x80000011;
                header->nSize = packetSize;
                header->nDest = 0;
                header->nOptional = 0;
                m_sifMan.SendPacket(callbackPacket, packetSize);
            }
        }
        break;
    case 255:
        //Not really sure about that...
        assert(retSize == 8);
        memcpy(ret, "....rawr", 8);
        resultPtr[0] = args[0];
        resultPtr[1] = args[1];
        break;
    default:
        printf("%s: Unknown function (%d) called.\r\n", __FUNCTION__, method);
        break;
    }
}

void CIoman::CFileIoHandler3100::CopyHeader(REPLYHEADER& reply, const COMMANDHEADER& command)
{
    reply.semaphoreId = command.semaphoreId;
    reply.resultPtr = command.resultPtr;
    reply.resultSize = command.resultSize;
}
