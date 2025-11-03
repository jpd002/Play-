#include <assert.h>
#include "make_unique.h"
#include "Iop_Spu2.h"
#include "Log.h"
#include "placeholder_def.h"

#define LOG_NAME ("iop_spu2")

#define SPDIF_OUT_BYPASS 0x0100

using namespace Iop;
using namespace Iop::Spu2;

CSpu2::CSpu2(CSpuBase& spuBase0, CSpuBase& spuBase1)
{
	for(unsigned int i = 0; i < CORE_NUM; i++)
	{
		CSpuBase& base(i == 0 ? spuBase0 : spuBase1);
		m_core[i] = std::make_unique<CCore>(i, base);
	}

	m_readDispatchInfo.global = std::bind(&CSpu2::ReadRegisterImpl, this, std::placeholders::_1, std::placeholders::_2);
	m_writeDispatchInfo.global = std::bind(&CSpu2::WriteRegisterImpl, this, std::placeholders::_1, std::placeholders::_2);
	for(unsigned int i = 0; i < CORE_NUM; i++)
	{
		m_readDispatchInfo.core[i] = std::bind(&CCore::ReadRegister, m_core[i].get(), std::placeholders::_1, std::placeholders::_2);
		m_writeDispatchInfo.core[i] = std::bind(&CCore::WriteRegister, m_core[i].get(), std::placeholders::_1, std::placeholders::_2);
	}
}

void CSpu2::Reset()
{
	for(unsigned int i = 0; i < CORE_NUM; i++)
	{
		m_core[i]->Reset();
	}
}

CCore* CSpu2::GetCore(unsigned int coreId)
{
	assert(coreId < CORE_NUM);
	if(coreId >= CORE_NUM) return nullptr;
	return m_core[coreId].get();
}

uint32 CSpu2::ReadRegister(uint32 address)
{
	return ProcessRegisterAccess(m_readDispatchInfo, address, 0);
}

uint32 CSpu2::WriteRegister(uint32 address, uint32 value)
{
	return ProcessRegisterAccess(m_writeDispatchInfo, address, value);
}

uint32 CSpu2::ProcessRegisterAccess(const REGISTER_DISPATCH_INFO& dispatchInfo, uint32 address, uint32 value)
{
	uint32 tmpAddress = address - REGS_BEGIN;
	if(tmpAddress < 0x760)
	{
		unsigned int coreId = (tmpAddress & 0x400) ? 1 : 0;
		address &= ~0x400;
		return dispatchInfo.core[coreId](address, value);
	}
	else if(tmpAddress < 0x7B0)
	{
		unsigned int coreId = (tmpAddress - 0x760) / 40;
		address -= coreId * 40;
		return dispatchInfo.core[coreId](address, value);
	}
	return dispatchInfo.global(address, value);
}

uint32 CSpu2::ReadRegisterImpl(uint32 address, uint32 value)
{
	uint32 result = 0;
	switch(address)
	{
	case C_IRQINFO:
		for(unsigned int i = 0; i < CORE_NUM; i++)
		{
			auto& core = m_core[i];
			if(core->GetSpuBase().GetIrqPending())
			{
				result |= (1 << (i + 2));
				core->GetSpuBase().ClearIrqPending();
			}
		}
		break;
	case C_SPDIF_OUT:
		result = m_spdifOutput;
		break;
	case C_SPDIF_MODE:
		result = m_spdifMode;
		break;
	case C_SPDIF_MEDIA:
		result = m_spdifMedia;
		break;
	}
	LogRead(address);
	return result;
}

uint32 CSpu2::WriteRegisterImpl(uint32 address, uint32 value)
{
	switch(address)
	{
	case C_SPDIF_OUT:
		m_spdifOutput = value;
		m_core[0]->GetSpuBase().SetInputBypass(value & SPDIF_OUT_BYPASS ? true : false);
		break;
	case C_SPDIF_MODE:
		m_spdifMode = value;
		break;
	case C_SPDIF_MEDIA:
		m_spdifMedia = value;
		break;
	}
	LogWrite(address, value);
	return 0;
}

void CSpu2::LogRead(uint32 address)
{
#define LOG_GET(registerId)                                      \
	case registerId:                                             \
		CLog::GetInstance().Print(LOG_NAME, #registerId "\r\n"); \
		break;

	switch(address)
	{
		LOG_GET(C_IRQINFO)
		LOG_GET(C_SPDIF_OUT)
		LOG_GET(C_SPDIF_MODE)
		LOG_GET(C_SPDIF_MEDIA)

	default:
		CLog::GetInstance().Warn(LOG_NAME, "Read an unknown register 0x%08X.\r\n", address);
		break;
	}
}

void CSpu2::LogWrite(uint32 address, uint32 value)
{
#define LOG_SET(registerId)                                                      \
	case registerId:                                                             \
		CLog::GetInstance().Print(LOG_NAME, #registerId " = 0x%04X\r\n", value); \
		break;

	switch(address)
	{
		LOG_SET(C_SPDIF_OUT)
		LOG_SET(C_SPDIF_MODE)
		LOG_SET(C_SPDIF_MEDIA)

	default:
		CLog::GetInstance().Warn(LOG_NAME, "Wrote 0x%08X to unknown register 0x%08X.\r\n", value, address);
		break;
	}
}
