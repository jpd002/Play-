#include "../Log.h"
#include "Iop_Cdvdman.h"

#define LOG_NAME            "iop_cdvdman"

#define FUNCTION_CDREAD     "CdRead"
#define FUNCTION_CDSYNC     "CdSync"

using namespace Iop;
using namespace std;

CCdvdman::CCdvdman(uint8* ram) :
m_ram(ram),
m_image(NULL)
{

}

CCdvdman::~CCdvdman()
{

}

string CCdvdman::GetId() const
{
    return "cdvdman";
}

string CCdvdman::GetFunctionName(unsigned int functionId) const
{
    switch(functionId)
    {
    case 6:
        return FUNCTION_CDREAD;
        break;
    case 11:
        return FUNCTION_CDSYNC;
        break;
    default:
        return "unknown";
        break;
    }
}

void CCdvdman::Invoke(CMIPS& ctx, unsigned int functionId)
{
    switch(functionId)
    {
    case 6:
        ctx.m_State.nGPR[CMIPS::V0].nV0 = CdRead(
            ctx.m_State.nGPR[CMIPS::A0].nV0,
            ctx.m_State.nGPR[CMIPS::A1].nV0,
            ctx.m_State.nGPR[CMIPS::A2].nV0,
            ctx.m_State.nGPR[CMIPS::A3].nV0);
        break;
    case 11:
        ctx.m_State.nGPR[CMIPS::V0].nV0 = CdSync();
        break;
    default:
        CLog::GetInstance().Print(LOG_NAME, "Unknown function called (%d).\r\n", 
            functionId);
        break;
    }
}

void CCdvdman::SetIsoImage(CISO9660* image)
{
    m_image = image;
}

uint32 CCdvdman::CdRead(uint32 startSector, uint32 sectorCount, uint32 bufferPtr, uint32 modePtr)
{
    CLog::GetInstance().Print(LOG_NAME, FUNCTION_CDREAD "(startSector = 0x%X, sectorCount = 0x%X, bufferPtr = 0x%0.8X, modePtr = 0x%0.8X);\r\n",
        startSector, sectorCount, bufferPtr, modePtr);
    if(modePtr != 0)
    {
        uint8* mode = &m_ram[modePtr];
        //Does that make sure it's 2048 byte mode?
        assert(mode[2] == 0);
    }
    if(m_image != NULL && bufferPtr != 0)
    {
        uint8* buffer = &m_ram[bufferPtr];
        uint32 sectorSize = 2048;
        for(unsigned int i = 0; i < sectorCount; i++)
        {
            m_image->ReadBlock(startSector + i, buffer);
            buffer += sectorSize;
        }
    }
    return 1;
}

uint32 CCdvdman::CdSync()
{
    CLog::GetInstance().Print(LOG_NAME, FUNCTION_CDSYNC "();\r\n");
    return 1;
}
