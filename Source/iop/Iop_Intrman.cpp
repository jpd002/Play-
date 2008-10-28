#include "Iop_Intrman.h"
#include "../Log.h"
#include "../COP_SCU.h"
#include "Iop_Intc.h"
#include "IopBios.h"

#define LOGNAME "iop_intrman"

using namespace Iop;
using namespace std;

CIntrman::CIntrman(CIopBios& bios, uint8* ram) :
m_bios(bios),
m_ram(ram)
{

}

CIntrman::~CIntrman()
{

}

string CIntrman::GetId() const
{
    return "intrman";
}

void CIntrman::Invoke(CMIPS& context, unsigned int functionId)
{
    switch(functionId)
    {
    case 4:
        context.m_State.nGPR[CMIPS::V0].nD0 = static_cast<int32>(RegisterIntrHandler(
            context.m_State.nGPR[CMIPS::A0].nV0,
            context.m_State.nGPR[CMIPS::A1].nV0,
            context.m_State.nGPR[CMIPS::A2].nV0,
            context.m_State.nGPR[CMIPS::A3].nV0
            ));
        break;
    case 5:
        context.m_State.nGPR[CMIPS::V0].nD0 = static_cast<int32>(ReleaseIntrHandler(
            context.m_State.nGPR[CMIPS::A0].nV0
            ));
        break;
    case 6:
        context.m_State.nGPR[CMIPS::V0].nD0 = static_cast<int32>(EnableIntrLine(
			context,
            context.m_State.nGPR[CMIPS::A0].nV0
            ));
        break;
    case 7:
        context.m_State.nGPR[CMIPS::V0].nD0 = static_cast<int32>(DisableIntrLine(
			context,
            context.m_State.nGPR[CMIPS::A0].nV0,
            context.m_State.nGPR[CMIPS::A1].nV0
            ));
        break;
    case 9:
        context.m_State.nGPR[CMIPS::V0].nD0 = static_cast<int32>(EnableInterrupts(
            context
            ));
        break;
    case 17:
        context.m_State.nGPR[CMIPS::V0].nD0 = static_cast<int32>(SuspendInterrupts(
            context,
            reinterpret_cast<uint32*>(&m_ram[context.m_State.nGPR[CMIPS::A0].nV0])
            ));
        break;
    case 18:
        context.m_State.nGPR[CMIPS::V0].nD0 = static_cast<int32>(ResumeInterrupts(
            context,
            context.m_State.nGPR[CMIPS::A0].nV0
            ));
        break;
    case 23:
        context.m_State.nGPR[CMIPS::V0].nD0 = static_cast<int32>(QueryIntrContext(
            context
            ));
        break;
    default:
        CLog::GetInstance().Print(LOGNAME, "%0.8X: Unknown function (%d) called.\r\n", context.m_State.nPC, functionId);
        break;
    }
}

uint32 CIntrman::RegisterIntrHandler(uint32 line, uint32 mode, uint32 handler, uint32 arg)
{
#ifdef _DEBUG
    CLog::GetInstance().Print(LOGNAME, "RegisterIntrHandler(line = %d, mode = %d, handler = 0x%0.8X, arg = 0x%0.8X);\r\n",
        line, mode, handler, arg);
#endif
    return m_bios.RegisterIntrHandler(line, mode, handler, arg) ? 1 : 0;
}

uint32 CIntrman::ReleaseIntrHandler(uint32 line)
{
#ifdef _DEBUG
    CLog::GetInstance().Print(LOGNAME, "ReleaseIntrHandler(line = %d);\r\n",
        line);
#endif
    return m_bios.ReleaseIntrHandler(line) ? 1 : 0;
}

uint32 CIntrman::EnableIntrLine(CMIPS& context, uint32 line)
{
#ifdef _DEBUG
    CLog::GetInstance().Print(LOGNAME, "EnableIntrLine(line = %d);\r\n",
        line);
#endif
	UNION64_32 mask(
		context.m_pMemoryMap->GetWord(CIntc::MASK0),
		context.m_pMemoryMap->GetWord(CIntc::MASK1));
	mask.f |= 1LL << line;
	context.m_pMemoryMap->SetWord(CIntc::MASK0, mask.h0);
	context.m_pMemoryMap->SetWord(CIntc::MASK1, mask.h1);
	return 0;
}

uint32 CIntrman::DisableIntrLine(CMIPS& context, uint32 line, uint32 res)
{
#ifdef _DEBUG
    CLog::GetInstance().Print(LOGNAME, "DisableIntrLine(line = %d, res = %0.8X);\r\n",
        line, res);
#endif
	UNION64_32 mask(
		context.m_pMemoryMap->GetWord(CIntc::MASK0),
		context.m_pMemoryMap->GetWord(CIntc::MASK1));
	mask.f &= ~(1LL << line);
	context.m_pMemoryMap->SetWord(CIntc::MASK0, mask.h0);
	context.m_pMemoryMap->SetWord(CIntc::MASK1, mask.h1);
	return 0;
}

uint32 CIntrman::EnableInterrupts(CMIPS& context)
{
#ifdef _DEBUG
    CLog::GetInstance().Print(LOGNAME, "EnableInterrupts();\r\n");
#endif

    uint32& statusRegister = context.m_State.nCOP0[CCOP_SCU::STATUS];
    statusRegister |= CMIPS::STATUS_INT;
    return 0;
}

uint32 CIntrman::SuspendInterrupts(CMIPS& context, uint32* state)
{
#ifdef _DEBUG
    CLog::GetInstance().Print(LOGNAME, "SuspendInterrupts();\r\n");
#endif
    uint32& statusRegister = context.m_State.nCOP0[CCOP_SCU::STATUS];
    (*state) = statusRegister & CMIPS::STATUS_INT;
    statusRegister &= ~CMIPS::STATUS_INT;
    return 0;
}

uint32 CIntrman::ResumeInterrupts(CMIPS& context, uint32 state)
{
#ifdef _DEBUG
    CLog::GetInstance().Print(LOGNAME, "ResumeInterrupts();\r\n");
#endif
    uint32& statusRegister = context.m_State.nCOP0[CCOP_SCU::STATUS];
    if(state)
    {
        statusRegister |= CMIPS::STATUS_INT;
    }
    else
    {
        statusRegister &= ~CMIPS::STATUS_INT;
    }
    return 0;
}

uint32 CIntrman::QueryIntrContext(CMIPS& context)
{
#ifdef _DEBUG
    CLog::GetInstance().Print(LOGNAME, "QueryIntrContext();\r\n");
#endif
    uint32& statusRegister = context.m_State.nCOP0[CCOP_SCU::STATUS];
    return (statusRegister & CMIPS::STATUS_EXL ? 1 : 0);
}
