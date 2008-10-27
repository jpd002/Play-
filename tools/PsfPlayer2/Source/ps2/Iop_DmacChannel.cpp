#include <assert.h>
#include "Iop_DmacChannel.h"
#include "Iop_Dmac.h"

using namespace Iop;
using namespace Iop::Dmac;

CChannel::CChannel(uint32 baseAddress, unsigned int number, CDmac& dmac) :
m_dmac(dmac),
m_number(number),
m_baseAddress(baseAddress)
{
    Reset();
}

CChannel::~CChannel()
{

}

void CChannel::Reset()
{
    m_CHCR <<= 0;
    m_BCR <<= 0;
    m_MADR = 0;
}

void CChannel::SetReceiveFunction(const ReceiveFunctionType& receiveFunction)
{
	m_receiveFunction = receiveFunction;
}

void CChannel::ResumeDma()
{
	if(m_CHCR.tr == 0) return;
	assert(m_CHCR.co == 1 && m_CHCR.dr == 1);
	assert(m_receiveFunction);
	uint32 address = m_MADR & 0x1FFFFFFF;
	uint32 blocksTransfered = m_receiveFunction(m_dmac.GetRam() + address, m_BCR.bs * 4, m_BCR.ba);
	assert(blocksTransfered <= m_BCR.ba);
	m_BCR.ba -= blocksTransfered;

	if(m_BCR.ba == 0)
	{
		//Trigger interrupt
		m_CHCR.tr = 0;
		m_dmac.AssertLine(m_number);
	}
}

uint32 CChannel::ReadRegister(uint32 address)
{
	switch(address - m_baseAddress)
	{
	case REG_MADR:
		return m_MADR;
		break;
	case REG_BCR:
		return m_BCR;
		break;
	case REG_CHCR:
		return m_CHCR;
		break;
	}
	return 0;
}

void CChannel::WriteRegister(uint32 address, uint32 value)
{
	assert(m_CHCR.tr == 0);
	switch(address - m_baseAddress)
	{
	case REG_MADR:
		m_MADR = value;
		break;
	case REG_BCR:
		m_BCR <<= value;
		break;
    case REG_BCR + 2:
        //Not really cool...
        m_BCR.ba = static_cast<uint16>(value);
        break;
	case REG_CHCR:
		m_CHCR <<= value;
		if(m_CHCR.tr)
		{
			ResumeDma();
		}
		break;
	}
}
