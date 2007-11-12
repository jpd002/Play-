#include "Iop_Ioman.h"
#include <stdexcept>

using namespace Iop;
using namespace std;
using namespace Framework;

CIoman::CIoman(uint8* ram) :
m_ram(ram),
m_nextFileHandle(1)
{

}

CIoman::~CIoman()
{

}

std::string CIoman::GetId() const
{
    return "ioman";
}

void CIoman::RegisterDevice(const char* name, Ioman::CDevice* device)
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

uint32 CIoman::Seek(uint32 handle, uint32 position, uint32 whence)
{
    uint32 result = 0xFFFFFFFF;
    try
    {
        CStream* stream = GetFileStream(handle);
        switch(whence)
        {
        case 0:
            whence = STREAM_SEEK_SET;
            break;
        case 1:
            whence = STREAM_SEEK_CUR;
            break;
        case 2:
            whence = STREAM_SEEK_END;
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

CStream* CIoman::GetFileStream(uint32 handle)
{
    FileMapType::iterator file(m_files.find(handle));
    if(file == m_files.end())
    {
        throw runtime_error("Invalid file handle.");
    }
    return file->second;
}

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
    default:
        printf("%s(%0.8X): Unknown function (%d) called.\r\n", __FUNCTION__, context.m_State.nPC, functionId);
        break;
    }
}

//--------------------------------------------------
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
