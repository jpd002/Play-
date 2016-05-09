#include "Iop_SifDynamic.h"
#include "Iop_SifCmd.h"

using namespace Iop;

CSifDynamic::CSifDynamic(CSifCmd& sifCmd, uint32 serverDataAddress)
: m_sifCmd(sifCmd)
, m_serverDataAddress(serverDataAddress)
{

}

CSifDynamic::~CSifDynamic()
{

}

bool CSifDynamic::Invoke(uint32 method, uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	m_sifCmd.ProcessInvocation(m_serverDataAddress, method, args, argsSize);
	return false;
}

uint32 CSifDynamic::GetServerDataAddress() const
{
	return m_serverDataAddress;
}
