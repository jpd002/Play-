#include "Iop_Vblank.h"
#include "IopBios.h"
#include "../Log.h"

using namespace Iop;
using namespace std;

#define LOG_NAME                "iop_vblank"
#define FUNCTION_WAITVBLANKEND  "WaitVblankEnd"

CVblank::CVblank(CIopBios& bios) :
m_bios(bios)
{

}

CVblank::~CVblank()
{

}

string CVblank::GetId() const
{
    return "vblank";
}

string CVblank::GetFunctionName(unsigned int functionId) const
{
    switch(functionId)
    {
    case 5:
        return FUNCTION_WAITVBLANKEND;
        break;
    default:
        return "unknown";
        break;
    }
}

void CVblank::Invoke(CMIPS& context, unsigned int functionId)
{
    switch(functionId)
    {
    case 5:
        WaitVblankEnd();
        break;
    default:
        CLog::GetInstance().Print(LOG_NAME, "Unknown function called (%d).\r\n", functionId);
        break;
    }
}

void CVblank::WaitVblankEnd()
{
#ifdef _DEBUG
    CLog::GetInstance().Print(LOG_NAME, FUNCTION_WAITVBLANKEND "();\r\n");
#endif
    m_bios.DelayThread(100);
}
