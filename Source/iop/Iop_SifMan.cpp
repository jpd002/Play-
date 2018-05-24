#include "Iop_SifMan.h"
#include "Iop_Sysmem.h"
#include "../MIPSAssembler.h"
#include "../Log.h"

#define LOG_NAME ("iop_sifman")

#define FUNCTION_SIFINIT "SifInit"
#define FUNCTION_SIFSETDMA "SifSetDma"
#define FUNCTION_SIFDMASTAT "SifDmaStat"
#define FUNCTION_SIFCHECKINIT "SifCheckInit"
#define FUNCTION_SIFSETDMACALLBACK "SifSetDmaCallback"

using namespace Iop;

CSifMan::CSifMan()
    : m_sifSetDmaCallbackHandlerPtr(0)
{
}

CSifMan::~CSifMan()
{
}

std::string CSifMan::GetId() const
{
	return "sifman";
}

std::string CSifMan::GetFunctionName(unsigned int functionId) const
{
	switch(functionId)
	{
	case 5:
		return FUNCTION_SIFINIT;
		break;
	case 7:
		return FUNCTION_SIFSETDMA;
		break;
	case 8:
		return FUNCTION_SIFDMASTAT;
		break;
	case 29:
		return FUNCTION_SIFCHECKINIT;
		break;
	case 32:
		return FUNCTION_SIFSETDMACALLBACK;
		break;
	}
	return "unknown";
}

void CSifMan::Invoke(CMIPS& context, unsigned int functionId)
{
	switch(functionId)
	{
	case 7:
		context.m_State.nGPR[CMIPS::V0].nD0 = static_cast<int32>(SifSetDma(
		    context.m_State.nGPR[CMIPS::A0].nV0,
		    context.m_State.nGPR[CMIPS::A1].nV0));
		break;
	case 8:
		context.m_State.nGPR[CMIPS::V0].nV0 = SifDmaStat(context.m_State.nGPR[CMIPS::A0].nV0);
		break;
	case 32:
		context.m_State.nGPR[CMIPS::V0].nD0 = static_cast<int32>(SifSetDmaCallback(
		    context,
		    context.m_State.nGPR[CMIPS::A0].nV0,
		    context.m_State.nGPR[CMIPS::A1].nV0,
		    context.m_State.nGPR[CMIPS::A2].nV0,
		    context.m_State.nGPR[CMIPS::A3].nV0));
		break;
	default:
		CLog::GetInstance().Warn(LOG_NAME, "%08X: Unknown function (%d) called.\r\n", context.m_State.nPC, functionId);
		break;
	}
}

void CSifMan::GenerateHandlers(uint8* ram, CSysmem& sysMem)
{
	const uint32 handlerAllocSize = 64;

	assert(m_sifSetDmaCallbackHandlerPtr == 0);
	m_sifSetDmaCallbackHandlerPtr = sysMem.AllocateMemory(handlerAllocSize, 0, 0);

	CMIPSAssembler assembler(reinterpret_cast<uint32*>(ram + m_sifSetDmaCallbackHandlerPtr));

	assembler.ADDIU(CMIPS::SP, CMIPS::SP, 0xFFF0);
	assembler.SW(CMIPS::RA, 0x00, CMIPS::SP);
	assembler.SW(CMIPS::S0, 0x04, CMIPS::SP);

	assembler.ADDU(CMIPS::S0, CMIPS::V0, CMIPS::R0);
	assembler.JALR(CMIPS::A1);
	assembler.NOP();

	assembler.ADDU(CMIPS::V0, CMIPS::S0, CMIPS::R0);

	assembler.LW(CMIPS::S0, 0x04, CMIPS::SP);
	assembler.LW(CMIPS::RA, 0x00, CMIPS::SP);
	assembler.JR(CMIPS::RA);
	assembler.ADDIU(CMIPS::SP, CMIPS::SP, 0x0010);

	assert((assembler.GetProgramSize() * 4) <= handlerAllocSize);
}

uint32 CSifMan::SifSetDma(uint32 structAddr, uint32 count)
{
	CLog::GetInstance().Print(LOG_NAME, FUNCTION_SIFSETDMA "(structAddr = 0x%08X, count = %d);\r\n",
	                          structAddr, count);
	return count;
}

uint32 CSifMan::SifDmaStat(uint32 transferId)
{
	CLog::GetInstance().Print(LOG_NAME, FUNCTION_SIFDMASTAT "(transferId = %X);\r\n",
	                          transferId);
	return -1;
}

uint32 CSifMan::SifSetDmaCallback(CMIPS& context, uint32 structAddr, uint32 count, uint32 callbackPtr, uint32 callbackParam)
{
	CLog::GetInstance().Print(LOG_NAME, FUNCTION_SIFSETDMACALLBACK "(structAddr = 0x%08X, count = %d, callbackPtr = 0x%08X, callbackParam = 0x%08X);\r\n",
	                          structAddr, count, callbackPtr, callbackParam);

	//Modify context so we can execute the callback function
	context.m_State.nPC = m_sifSetDmaCallbackHandlerPtr;
	context.m_State.nGPR[CMIPS::A0].nV0 = callbackParam;
	context.m_State.nGPR[CMIPS::A1].nV0 = callbackPtr;

	return SifSetDma(structAddr, count);
}
