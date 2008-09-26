#include <assert.h>
#include "DmaChannel.h"
#include "Dmac.h"

using namespace Psx;

CDmaChannel::CDmaChannel(uint32 baseAddress, unsigned int number, CDmac& dmac) :
m_baseAddress(baseAddress),
m_number(number),
m_dmac(dmac)
{
	Reset();
}

CDmaChannel::~CDmaChannel()
{

}

void CDmaChannel::Reset()
{
	m_MADR = 0;
	m_BCR <<= 0;
	m_CHCR <<= 0;
}

void CDmaChannel::SetReceiveFunction(const ReceiveFunctionType& receiveFunction)
{
	m_receiveFunction = receiveFunction;
}

void CDmaChannel::ResumeDma()
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

uint32 CDmaChannel::ReadRegister(uint32 address)
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

void CDmaChannel::WriteRegister(uint32 address, uint32 value)
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
	case REG_CHCR:
		m_CHCR <<= value;
		if(m_CHCR.tr)
		{
			ResumeDma();
		}
		break;
	}
}
