#include "Iop_Ioman.h"
#include <stdexcept>

using namespace Iop;
using namespace std;
using namespace Framework;

CIoman::CIoman(uint8* ram) :
m_ram(ram)
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
    }
    catch(const exception& except)
    {
        printf("%s: Error occured while trying to open file : %s\r\n", __FUNCTION__, except.what());
    }
    return handle;
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
    default:
        printf("%s(%0.8X): Unknown function (%d) called.", __FUNCTION__, context.m_State.nPC, functionId);
        break;
    }
}
