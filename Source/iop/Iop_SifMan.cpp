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
	case 29:
		context.m_State.nGPR[CMIPS::V0].nV0 = SifCheckInit();
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

void CSifMan::PrepareModuleData(uint8* ram, CSysmem& sysMem)
{
	assert(m_moduleData == nullptr);
	assert(m_sifSetDmaCallbackHandlerAddr == 0);

	uint32 moduleDataAddr = sysMem.AllocateMemory(sizeof(MODULEDATA), 0, 0);

	m_moduleData = reinterpret_cast<MODULEDATA*>(ram + moduleDataAddr);
	m_sifSetDmaCallbackHandlerAddr = moduleDataAddr + offsetof(MODULEDATA, sifSetDmaCallbackHandler);

	//Assemble handler
	{
		CMIPSAssembler assembler(reinterpret_cast<uint32*>(ram + m_sifSetDmaCallbackHandlerAddr));

		assembler.ADDIU(CMIPS::SP, CMIPS::SP, 0xFFF0);
		//SP + 0x00 is the space to store A0 by handler
		assembler.SW(CMIPS::RA, 0x04, CMIPS::SP);
		assembler.SW(CMIPS::S0, 0x08, CMIPS::SP);

		assembler.ADDU(CMIPS::S0, CMIPS::V0, CMIPS::R0);
		assembler.JALR(CMIPS::A1);
		assembler.NOP();

		assembler.ADDU(CMIPS::V0, CMIPS::S0, CMIPS::R0);

		assembler.LW(CMIPS::S0, 0x08, CMIPS::SP);
		assembler.LW(CMIPS::RA, 0x04, CMIPS::SP);
		assembler.JR(CMIPS::RA);
		assembler.ADDIU(CMIPS::SP, CMIPS::SP, 0x0010);

		assert((assembler.GetProgramSize() * 4) <= SIFSETDMACALLBACK_HANDLER_SIZE);
	}

	m_moduleData->dmaTransferTime = 0;
}

void CSifMan::CountTicks(int32 ticks)
{
	m_moduleData->dmaTransferTime = std::max(0, m_moduleData->dmaTransferTime - ticks);
}

uint32 CSifMan::SifSetDma(uint32 structAddr, uint32 count)
{
	CLog::GetInstance().Print(LOG_NAME, FUNCTION_SIFSETDMA "(structAddr = 0x%08X, count = %d);\r\n",
	                          structAddr, count);
	//Reset delay regardless of what we're sending
	//This doesn't seem to cause harm, but if it does, we could associate a transfer id
	//with a time and have SifDmaStat check for a specific transfer id
	//This is needed because RenderWare FS code doesn't expect SIF CMD callbacks to
	//arrive so quickly
	m_moduleData->dmaTransferTime = 0x800;
	return count;
}

uint32 CSifMan::SifDmaStat(uint32 transferId)
{
	CLog::GetInstance().Print(LOG_NAME, FUNCTION_SIFDMASTAT "(transferId = %X);\r\n",
	                          transferId);
	if(m_moduleData->dmaTransferTime != 0)
	{
		return 0;
	}
	else
	{
		return -1;
	}
}

uint32 CSifMan::SifCheckInit()
{
	CLog::GetInstance().Print(LOG_NAME, FUNCTION_SIFCHECKINIT "();\r\n");
	// Since we don't handle the init call, always return true for check init.
	return 1;
}

uint32 CSifMan::SifSetDmaCallback(CMIPS& context, uint32 structAddr, uint32 count, uint32 callbackPtr, uint32 callbackParam)
{
	CLog::GetInstance().Print(LOG_NAME, FUNCTION_SIFSETDMACALLBACK "(structAddr = 0x%08X, count = %d, callbackPtr = 0x%08X, callbackParam = 0x%08X);\r\n",
	                          structAddr, count, callbackPtr, callbackParam);

	//Modify context so we can execute the callback function
	context.m_State.nPC = m_sifSetDmaCallbackHandlerAddr;
	context.m_State.nGPR[CMIPS::A0].nV0 = callbackParam;
	context.m_State.nGPR[CMIPS::A1].nV0 = callbackPtr;

	return SifSetDma(structAddr, count);
}
