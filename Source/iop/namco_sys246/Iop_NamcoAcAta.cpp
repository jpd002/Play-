#include "Iop_NamcoAcAta.h"
#include <cstring>
#include "Log.h"
#include "../IopBios.h"

using namespace Iop;
using namespace Iop::Namco;

#define LOG_NAME ("iop_namco_acata")

#define FUNCTION_ATASETUP "AtaSetup"
#define FUNCTION_ATAREQUEST "AtaRequest"

enum ATA_REGISTER
{
	ATA_REGISTER_DATA,
	ATA_REGISTER_FEATURES,
	ATA_REGISTER_SECTOR_COUNT,
	ATA_REGISTER_LBA_LO,
	ATA_REGISTER_LBA_MID,
	ATA_REGISTER_LBA_HI,
	ATA_REGISTER_DRIVE,
	ATA_REGISTER_COMMAND,
	ATA_REGISTER_MAX,
};

enum ATA_COMMAND
{
	ATA_COMMAND_INITIALIZE_DEVICE_PARAMETERS = 0x91,
	ATA_COMMAND_READ_DMA = 0xC9,
	ATA_COMMAND_IDENTIFY_DEVICE = 0xEC,
	ATA_COMMAND_SET_FEATURE = 0xEF,
};

struct ATA_CONTEXT
{
	uint32 doneFctPtr;
	uint32 param;
};

CAcAta::CAcAta(CIopBios& bios, uint8* ram)
    : m_bios(bios)
    , m_ram(ram)
{
}

void CAcAta::SetHddStream(std::unique_ptr<Framework::CStream> hddStream)
{
	m_hddStream = std::move(hddStream);
}

std::string CAcAta::GetId() const
{
	return "acata";
}

std::string CAcAta::GetFunctionName(unsigned int functionId) const
{
	switch(functionId)
	{
	case 8:
		return FUNCTION_ATASETUP;
	case 9:
		return FUNCTION_ATAREQUEST;
	default:
		return "unknown";
	}
}

void CAcAta::Invoke(CMIPS& ctx, unsigned int functionId)
{
	switch(functionId)
	{
	case 8:
		ctx.m_State.nGPR[CMIPS::V0].nV0 = AtaSetup(
		    ctx.m_State.nGPR[CMIPS::A0].nV0,
		    ctx.m_State.nGPR[CMIPS::A1].nV0,
		    ctx.m_State.nGPR[CMIPS::A2].nV0,
		    ctx.m_State.nGPR[CMIPS::A3].nV0);
		break;
	case 9:
		ctx.m_State.nGPR[CMIPS::V0].nV0 = AtaRequest(
		    ctx.m_State.nGPR[CMIPS::A0].nV0,
		    ctx.m_State.nGPR[CMIPS::A1].nV0,
		    ctx.m_State.nGPR[CMIPS::A2].nV0,
		    ctx.m_State.nGPR[CMIPS::A3].nV0,
		    ctx.m_pMemoryMap->GetWord(ctx.m_State.nGPR[CMIPS::SP].nV0 + 0x10),
		    ctx.m_pMemoryMap->GetWord(ctx.m_State.nGPR[CMIPS::SP].nV0 + 0x14));
		break;
	default:
		CLog::GetInstance().Warn(LOG_NAME, "Unknown IOP method invoked (0x%08X).\r\n", functionId);
		break;
	}
}

int32 CAcAta::AtaSetup(uint32 ataCtxPtr, uint32 doneFctPtr, uint32 param, uint32 timeout)
{
	CLog::GetInstance().Print(LOG_NAME, FUNCTION_ATASETUP "(ataCtxPtr = 0x%08X, doneFctPtr = 0x%08X, param = 0x%08X, timeout = %d);",
	                          ataCtxPtr, doneFctPtr, param, timeout);

	assert(ataCtxPtr != 0);
	auto ataCtx = reinterpret_cast<ATA_CONTEXT*>(m_ram + ataCtxPtr);
	ataCtx->doneFctPtr = doneFctPtr;
	ataCtx->param = param;

	return ataCtxPtr;
}

int32 CAcAta::AtaRequest(uint32 ataCtxPtr, uint32 flag, uint32 commandPtr, uint32 itemCount, uint32 bufferPtr, uint32 bufferSize)
{
	CLog::GetInstance().Print(LOG_NAME, FUNCTION_ATAREQUEST "(ataCtxPtr = 0x%08X, flag = 0x%08X, commandPtr = 0x%08X, itemCount = %d, bufferPtr = 0x%08X, bufferSize = %d);",
	                          ataCtxPtr, flag, commandPtr, itemCount, bufferPtr, bufferSize);

	auto ataCtx = reinterpret_cast<ATA_CONTEXT*>(m_ram + ataCtxPtr);
	auto command = reinterpret_cast<uint16*>(m_ram + commandPtr);
	auto buffer = reinterpret_cast<uint8*>(m_ram + bufferPtr);

	uint8 registers[ATA_REGISTER_MAX] = {};
	for(int i = 0; i < itemCount; i++)
	{
		uint16 item = command[i];
		uint8 regId = static_cast<uint8>(item >> 8);
		assert(regId < ATA_REGISTER_MAX);
		if(regId >= ATA_REGISTER_MAX)
		{
			continue;
		}
		registers[regId] = static_cast<uint8>(item & 0xFF);
	}

	if(bufferSize != 0)
	{
		assert(bufferPtr != 0);
		memset(buffer, 0xCC, bufferSize);
	}

	switch(registers[ATA_REGISTER_COMMAND])
	{
	case ATA_COMMAND_INITIALIZE_DEVICE_PARAMETERS:
		//Set some head number and sector count?
		break;
	case ATA_COMMAND_READ_DMA:
	{
		constexpr uint32 sectorSize = 512;
		uint32 sectorCount = registers[ATA_REGISTER_SECTOR_COUNT];
		uint32 totalSize = sectorSize * sectorCount;
		assert(totalSize <= bufferSize);
		//Make sure we're in LBA mode
		assert((registers[ATA_REGISTER_DRIVE] & 0x40) == 0x40);
		//Head value is upper bits of LBA
		assert((registers[ATA_REGISTER_DRIVE] & 0x0F) == 0x00);
		uint32 lba =
		    static_cast<uint32>((registers[ATA_REGISTER_LBA_LO]) << 0) |
		    static_cast<uint32>((registers[ATA_REGISTER_LBA_MID] << 8)) |
		    static_cast<uint32>((registers[ATA_REGISTER_LBA_HI] << 16));
		m_hddStream->Seek(lba * sectorSize, Framework::STREAM_SEEK_SET);
		for(uint32 sector = 0; sector < sectorCount; sector++)
		{
			m_hddStream->Read(buffer, sectorSize);
			buffer += sectorSize;
		}
	}
	break;
	case ATA_COMMAND_IDENTIFY_DEVICE:
		break;
	case ATA_COMMAND_SET_FEATURE:
		break;
	default:
		assert(false);
		break;
	}

	uint32 result = 0;
	m_bios.TriggerCallback(ataCtx->doneFctPtr, ataCtxPtr, ataCtx->param, result);

	return 0;
}
