#include "Iop_SifManPs2.h"

using namespace Iop;

CSifManPs2::CSifManPs2(CSIF& sif) :
m_sif(sif)
{

}

CSifManPs2::~CSifManPs2()
{

}

void CSifManPs2::RegisterModule(uint32 id, CSifModule* module)
{
    m_sif.RegisterModule(id, module);
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

void CSifManPs2::SendCallReply(uint32 serverId, void* returnData)
{
    m_sif.SendCallReply(serverId, returnData);
}
