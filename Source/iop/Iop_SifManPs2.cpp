#include "Iop_SifManPs2.h"

using namespace Iop;

CSifManPs2::CSifManPs2(CSIF& sif, uint8* eeRam, uint8* iopRam) 
: m_sif(sif)
, m_eeRam(eeRam)
, m_iopRam(iopRam)
{

}

CSifManPs2::~CSifManPs2()
{

}

void CSifManPs2::RegisterModule(uint32 id, CSifModule* module)
{
	m_sif.RegisterModule(id, module);
}

bool CSifManPs2::IsModuleRegistered(uint32 id)
{
	return m_sif.IsModuleRegistered(id);
}

void CSifManPs2::UnregisterModule(uint32 id)
{
	m_sif.UnregisterModule(id);
}

void CSifManPs2::SendPacket(void* packet, uint32 size)
{
	m_sif.SendPacket(packet, size);
}

void CSifManPs2::SetDmaBuffer(uint32 bufferAddress, uint32 size)
{
	m_sif.SetDmaBuffer(bufferAddress, size);
}

void CSifManPs2::SetCmdBuffer(uint32 bufferAddress, uint32 size)
{
	m_sif.SetCmdBuffer(bufferAddress, size);
}

void CSifManPs2::SendCallReply(uint32 serverId, const void* returnData)
{
	m_sif.SendCallReply(serverId, returnData);
}

void CSifManPs2::GetOtherData(uint32 dst, uint32 src, uint32 size)
{
	uint8* srcPtr = m_eeRam + src;
	uint8* dstPtr = m_iopRam + dst;
	memcpy(dstPtr, srcPtr, size);
}

void CSifManPs2::SetModuleResetHandler(const ModuleResetHandler& moduleResetHandler)
{
	m_sif.SetModuleResetHandler(moduleResetHandler);
}

void CSifManPs2::SetCustomCommandHandler(const CustomCommandHandler& customCommandHandler)
{
	m_sif.SetCustomCommandHandler(customCommandHandler);
}

uint32 CSifManPs2::SifSetDma(uint32 structAddr, uint32 count)
{
	CSifMan::SifSetDma(structAddr, count);

	if(structAddr == 0)
	{
		return 0;
	}

	auto dmaReg = reinterpret_cast<SIFDMAREG*>(m_iopRam + structAddr);
	for(unsigned int i = 0; i < count; i++)
	{
		uint8* src = m_iopRam + dmaReg[i].srcAddr;
		uint8* dst = m_eeRam + dmaReg[i].dstAddr;
		memcpy(dst, src, dmaReg[i].size);
	}

	return count;
}

uint8* CSifManPs2::GetEeRam() const
{
	return m_eeRam;
}
