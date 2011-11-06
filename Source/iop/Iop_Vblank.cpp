#include "Iop_Vblank.h"
#include "IopBios.h"
#include "Iop_Intc.h"
#include "../Log.h"

using namespace Iop;

#define LOG_NAME						"iop_vblank"
#define FUNCTION_WAITVBLANKSTART		"WaitVblankStart"
#define FUNCTION_WAITVBLANKEND			"WaitVblankEnd"
#define FUNCTION_REGISTERVBLANKHANDLER	"RegisterVblankHandler"

CVblank::CVblank(CIopBios& bios) :
m_bios(bios)
{

}

CVblank::~CVblank()
{

}

std::string CVblank::GetId() const
{
    return "vblank";
}

std::string CVblank::GetFunctionName(unsigned int functionId) const
{
    switch(functionId)
    {
    case 4:
        return FUNCTION_WAITVBLANKSTART;
        break;
    case 5:
        return FUNCTION_WAITVBLANKEND;
        break;
	case 8:
		return FUNCTION_REGISTERVBLANKHANDLER;
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
    case 4:
        WaitVblankStart();
        break;
    case 5:
        WaitVblankEnd();
        break;
	case 8:
        context.m_State.nGPR[CMIPS::V0].nD0 = RegisterVblankHandler(
			context,
            context.m_State.nGPR[CMIPS::A0].nV0,
            context.m_State.nGPR[CMIPS::A1].nV0,
            context.m_State.nGPR[CMIPS::A2].nV0,
			context.m_State.nGPR[CMIPS::A3].nV0);
		break;
    default:
        CLog::GetInstance().Print(LOG_NAME, "Unknown function called (%d).\r\n", functionId);
        break;
    }
}

void CVblank::WaitVblankStart()
{
#ifdef _DEBUG
	CLog::GetInstance().Print(LOG_NAME, FUNCTION_WAITVBLANKSTART "();\r\n");
#endif
	m_bios.SleepThreadTillVBlankStart();
}

void CVblank::WaitVblankEnd()
{
#ifdef _DEBUG
	CLog::GetInstance().Print(LOG_NAME, FUNCTION_WAITVBLANKEND "();\r\n");
#endif
	m_bios.SleepThreadTillVBlankEnd();
}

uint32 CVblank::RegisterVblankHandler(CMIPS& context, uint32 startEnd, uint32 priority, uint32 handlerPtr, uint32 handlerParam)
{
#ifdef _DEBUG
	CLog::GetInstance().Print(LOG_NAME, FUNCTION_REGISTERVBLANKHANDLER "(startEnd = %d, priority = %d, handler = 0x%0.8X, arg = 0x%0.8X).\r\n",
		startEnd, priority, handlerPtr, handlerParam);
#endif
	uint32 intrLine = startEnd ? CIntc::LINE_EVBLANK : CIntc::LINE_VBLANK;

	m_bios.RegisterIntrHandler(intrLine, 0, handlerPtr, handlerParam);

	uint32 mask = context.m_pMemoryMap->GetWord(CIntc::MASK0);
	mask |= (1 << intrLine);
	context.m_pMemoryMap->SetWord(CIntc::MASK0, mask);

	return 0;
}
