#include "Iop_SifManNull.h"

using namespace Iop;

void CSifManNull::RegisterModule(uint32, CSifModule*)
{

}

bool CSifManNull::IsModuleRegistered(uint32)
{
	return false;
}

void CSifManNull::UnregisterModule(uint32)
{

}

void CSifManNull::SendPacket(void*, uint32)
{

}

void CSifManNull::SetDmaBuffer(uint32, uint32)
{

}

void CSifManNull::SendCallReply(uint32, const void*)
{

}

void CSifManNull::GetOtherData(uint32, uint32, uint32)
{

}

void CSifManNull::SetCustomCommandHandler(const CustomCommandHandler&)
{

}
