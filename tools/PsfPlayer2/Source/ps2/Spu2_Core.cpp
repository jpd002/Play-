#include <boost/lexical_cast.hpp>
#include <assert.h>
#include "Spu2_Core.h"
#include "Log.h"

#define LOG_NAME_PREFIX ("spu2_core_")
#define SPU_BASE_SAMPLING_RATE (48000)

using namespace PS2::Spu2;
using namespace std;
using namespace std::tr1;
using namespace Framework;
using namespace boost;

CCore::CCore(unsigned int coreId) :
m_coreId(coreId),
m_spuBase(NULL)
//m_ram(new uint8[RAMSIZE])
{
	m_logName = LOG_NAME_PREFIX + lexical_cast<string>(m_coreId);

	m_readDispatch.core		= &CCore::ReadRegisterCore;
	m_readDispatch.channel	= &CCore::ReadRegisterChannel;

	m_writeDispatch.core	= &CCore::WriteRegisterCore;
	m_writeDispatch.channel = &CCore::WriteRegisterChannel;

	Reset();
}

CCore::~CCore()
{
//    delete [] m_ram;
}

void CCore::Reset()
{
	if(m_spuBase)
	{
		m_spuBase->Reset();
	}
//    memset(m_ram, 0, RAMSIZE);
//	m_transferAddress.f = 0;
	m_coreAttr = 0;
}

void CCore::SetSpu(CSpu* spu)
{
	m_spuBase = spu;
}

//CSpu& CCore::GetSpu()
//{
//    return m_spuBase;
//}

uint32 CCore::ReadRegister(uint32 address, uint32 value)
{
	return ProcessRegisterAccess(m_readDispatch, address, value);
}

uint32 CCore::WriteRegister(uint32 address, uint32 value)
{
	return ProcessRegisterAccess(m_writeDispatch, address, value);
}

uint32 CCore::ReceiveDma(uint8* buffer, uint32 blockSize, uint32 blockAmount)
{
/*
	assert((blockSize & 1) == 0);
	unsigned int blocksTransfered = 0;
	uint32 dstAddress = m_transferAddress.f << 1;
	for(unsigned int i = 0; i < blockAmount; i++)
	{
		uint32 copySize = min<uint32>(RAMSIZE - dstAddress, blockSize);
		memcpy(m_ram + dstAddress, buffer, copySize);
		dstAddress += blockSize;
		dstAddress &= RAMSIZE - 1;
		buffer += blockSize;
		blocksTransfered++;
	}
	m_transferAddress.f = dstAddress >> 1;
*/
    return m_spuBase->ReceiveDma(buffer, blockSize, blockAmount);
}

uint32 CCore::ProcessRegisterAccess(const REGISTER_DISPATCH_INFO& dispatchInfo, uint32 address, uint32 value)
{
	if(address < S_REG_BASE)
	{
		//Channel access
		unsigned int channelId = (address >> 4) & 0x3F;
		address &= ~(0x3F << 4);
		return ((this)->*(dispatchInfo.channel))(channelId, address, value);
	}
	else if(address >= VA_REG_BASE && address < R_REG_BASE)
	{
		//Channel access
		unsigned int channelId = (address - VA_REG_BASE) / 12;
		address -= channelId * 12;
		return ((this)->*(dispatchInfo.channel))(channelId, address, value);
	}
	else
	{
		//Core write
		return ((this)->*(dispatchInfo.core))(0, address, value);
	}
}

uint32 CCore::ReadRegisterCore(unsigned int channelId, uint32 address, uint32 value)
{
	uint32 result = 0;
	switch(address)
	{
	case STATX:
		result = 0x0000;
		if(m_coreAttr & CORE_DMA)
		{
			result |= 0x80;
		}
		break;
	case CORE_ATTR:
		result = m_coreAttr;
		break;
    case A_TSA_HI:
		if(m_spuBase)
		{
			uint32 transferAddress = m_spuBase->GetTransferAddress();
			result = (transferAddress >> (16 + 1));
        }
        break;
	}
    LogRead(address);
	return result;
}

uint32 CCore::WriteRegisterCore(unsigned int channelId, uint32 address, uint32 value)
{
	switch(address)
	{
	case CORE_ATTR:
		if(m_spuBase)
		{
			m_spuBase->SetBaseSamplingRate(SPU_BASE_SAMPLING_RATE);
		}
		m_coreAttr = static_cast<uint16>(value);
		break;
    case A_STD:
        {
//            uint32 address = m_transferAddress.f << 1;
//            address &= RAMSIZE - 1;
//            *reinterpret_cast<uint16*>(m_ram + address) = static_cast<uint16>(value);
//            m_transferAddress.f++;
        }
        break;
    case A_KON_HI:
		if(m_spuBase)
		{
			m_spuBase->SendKeyOn(value);
		}
        break;
    case A_KON_LO:
		if(m_spuBase)
		{
	        m_spuBase->SendKeyOn(value << 16);
		}
        break;
    case A_KOFF_HI:
		if(m_spuBase)
		{
	        m_spuBase->SendKeyOff(value);
		}
        break;
    case A_KOFF_LO:
		if(m_spuBase)
		{
			m_spuBase->SendKeyOff(value << 16);
		}
        break;
	case A_TSA_HI:
		if(m_spuBase)
		{
			uint32 transferAddress = m_spuBase->GetTransferAddress();
			transferAddress &= 0xFFFF << 1;
			transferAddress |= value << (1 + 16);
			transferAddress &= RAMSIZE - 1;
			m_spuBase->SetTransferAddress(transferAddress);
		}
		break;
	case A_TSA_LO:
		if(m_spuBase)
		{
			uint32 transferAddress = m_spuBase->GetTransferAddress();
			transferAddress &= 0xFFFF << (1 + 16);
			transferAddress |= value << 1;
			transferAddress &= RAMSIZE - 1;
			m_spuBase->SetTransferAddress(transferAddress);
		}
		break;
	}
	LogWrite(address, value);
	return 0;
}

uint32 CCore::ReadRegisterChannel(unsigned int channelId, uint32 address, uint32 value)
{
	assert(channelId < MAX_CHANNEL);
	if(channelId >= MAX_CHANNEL)
	{
		return 0;
	}
	if(!m_spuBase) return 0;
	uint32 result = 0;
	CSpu::CHANNEL& channel(m_spuBase->GetChannel(channelId));
	switch(address)
	{
	case VA_NAX_HI:
		result = ((channel.current) >> (16 + 1)) & 0xFFFF;
		break;
	case VA_NAX_LO:
		result = ((channel.current) >> 1) & 0xFFFF;
		break;
	}
	LogChannelRead(channelId, address, result);
	return result;
}

uint32 CCore::WriteRegisterChannel(unsigned int channelId, uint32 address, uint32 value)
{
	assert(channelId < MAX_CHANNEL);
	if(channelId >= MAX_CHANNEL)
	{
		return 0;
	}
	if(!m_spuBase) return 0;
	CSpu::CHANNEL& channel(m_spuBase->GetChannel(channelId));
	switch(address)
	{
	case VP_VOLL:
		channel.volumeLeft <<= static_cast<uint16>(value);
		break;
	case VP_VOLR:
		channel.volumeRight <<= static_cast<uint16>(value);
		break;
	case VP_PITCH:
		channel.pitch = static_cast<uint16>(value);
		break;
	case VP_ADSR1:
		channel.adsrLevel <<= static_cast<uint16>(value);
		break;
	case VP_ADSR2:
		channel.adsrRate <<= static_cast<uint16>(value);
		break;
	case VP_ENVX:
		channel.adsrVolume = static_cast<uint16>(value);
		break;
	case VP_VOLXL:
//		channel.volxLeft = static_cast<uint16>(value);
		break;
	case VP_VOLXR:
//		channel.volxRight = static_cast<uint16>(value);
		break;
	case VA_SSA_HI:
		{
			uint32 address = channel.address * 8;
			address &= 0xFFFF << 1;
			address |= value << (1 + 16);
			assert((address & 0x7) == 0);
			channel.address = address / 8;
		}
		break;
	case VA_SSA_LO:
		{
			uint32 address = channel.address * 8;
			address &= 0xFFFF << (1 + 16);
			address |= value << 1;
			assert((address & 0x7) == 0);
			channel.address = address / 8;
		}
		break;
	case VA_LSAX_HI:
		{
			uint32 address = channel.repeat * 8;
			address &= 0xFFFF << 1;
			address |= value << (1 + 16);
			assert((address & 0x7) == 0);
			channel.repeat = address / 8;
		}
		break;
	case VA_LSAX_LO:
		{
			uint32 address = channel.repeat * 8;
			address &= 0xFFFF << (1 + 16);
			address |= value << 1;
			assert((address & 0x7) == 0);
			channel.repeat = address / 8;
		}
		break;
	}
/*
	CChannel& channel(m_channel[channelId]);
	switch(address)
	{
	case VP_VOLL:
		channel.volLeft = static_cast<uint16>(value);
		break;
	case VP_VOLR:
		channel.volRight = static_cast<uint16>(value);
		break;
	case VP_PITCH:
		channel.pitch = static_cast<uint16>(value);
		break;
	case VP_ADSR1:
		channel.adsr1 = static_cast<uint16>(value);
		break;
	case VP_ADSR2:
		channel.adsr2 = static_cast<uint16>(value);
		break;
	case VP_ENVX:
		channel.envx = static_cast<uint16>(value);
		break;
	case VP_VOLXL:
		channel.volxLeft = static_cast<uint16>(value);
		break;
	case VP_VOLXR:
		channel.volxRight = static_cast<uint16>(value);
		break;
	}
*/
	LogChannelWrite(channelId, address, value);
	return 0;
}

void CCore::LogRead(uint32 address)
{
	const char* logName = m_logName.c_str();
    switch(address)
    {
	case CORE_ATTR:
		CLog::GetInstance().Print(logName, "= CORE_ATTR\r\n");
		break;
    case STATX:
		CLog::GetInstance().Print(logName, "= STATX\r\n");
        break;
    case A_TSA_HI:
        CLog::GetInstance().Print(logName, "= A_TSA_HI.\r\n");
        break;
    default:
		CLog::GetInstance().Print(logName, "Read an unknown register 0x%0.4X.\r\n", address);
        break;
    }
}

void CCore::LogWrite(uint32 address, uint32 value)
{
	const char* logName = m_logName.c_str();
    switch(address)
    {
	case CORE_ATTR:
		CLog::GetInstance().Print(logName, "CORE_ATTR = 0x%0.4X\r\n", value);
		break;
    case A_KON_HI:
		CLog::GetInstance().Print(logName, "A_KON_HI = 0x%0.4X\r\n", value);
        break;
    case A_KON_LO:
		CLog::GetInstance().Print(logName, "A_KON_LO = 0x%0.4X\r\n", value);
        break;
    case A_KOFF_HI:
		CLog::GetInstance().Print(logName, "A_KOFF_HI = 0x%0.4X\r\n", value);
        break;
    case A_KOFF_LO:
		CLog::GetInstance().Print(logName, "A_KOFF_LO = 0x%0.4X\r\n", value);
        break;
	case A_TSA_HI:
		CLog::GetInstance().Print(logName, "A_TSA_HI = 0x%0.4X\r\n", value);
		break;
	case A_TSA_LO:
		CLog::GetInstance().Print(logName, "A_TSA_LO = 0x%0.4X\r\n", value);
		break;
	case A_STD:
		CLog::GetInstance().Print(logName, "A_STD = 0x%0.4X\r\n", value);
		break;
    default:
		CLog::GetInstance().Print(logName, "Write 0x%0.4X to an unknown register 0x%0.4X.\r\n", value, address);
        break;
    }
}

void CCore::LogChannelRead(unsigned int channelId, uint32 address, uint32 result)
{
	const char* logName = m_logName.c_str();
    switch(address)
    {
	case VA_NAX_HI:
		CLog::GetInstance().Print(logName, "ch%0.2i: = VA_NAX_HI = 0x%0.4X.\r\n", 
			channelId, result);
		break;
	case VA_NAX_LO:
		CLog::GetInstance().Print(logName, "ch%0.2i: = VA_NAX_LO = 0x%0.4X.\r\n", 
			channelId, result);
		break;
    default:
		CLog::GetInstance().Print(logName, "ch%0.2i: Read an unknown register 0x%0.4X.\r\n", 
			channelId, address);
        break;
    }
}

void CCore::LogChannelWrite(unsigned int channelId, uint32 address, uint32 value)
{
	const char* logName = m_logName.c_str();
    switch(address)
    {
	case VP_VOLL:
		CLog::GetInstance().Print(logName, "ch%0.2i: VP_VOLL = %0.4X.\r\n", 
			channelId, value);
		break;
	case VP_VOLR:
		CLog::GetInstance().Print(logName, "ch%0.2i: VP_VOLR = %0.4X.\r\n", 
			channelId, value);
		break;
	case VP_PITCH:
		CLog::GetInstance().Print(logName, "ch%0.2i: VP_PITCH = %0.4X.\r\n", 
			channelId, value);
		break;
	case VP_ADSR1:
		CLog::GetInstance().Print(logName, "ch%0.2i: VP_ADSR1 = %0.4X.\r\n", 
			channelId, value);
		break;
	case VP_ADSR2:
		CLog::GetInstance().Print(logName, "ch%0.2i: VP_ADSR2 = %0.4X.\r\n", 
			channelId, value);
		break;
	case VP_ENVX:
		CLog::GetInstance().Print(logName, "ch%0.2i: VP_ENVX = %0.4X.\r\n", 
			channelId, value);
		break;
	case VP_VOLXL:
		CLog::GetInstance().Print(logName, "ch%0.2i: VP_VOLXL = %0.4X.\r\n", 
			channelId, value);
		break;
	case VP_VOLXR:
		CLog::GetInstance().Print(logName, "ch%0.2i: VP_VOLXR = %0.4X.\r\n", 
			channelId, value);
		break;
	case VA_SSA_HI:
		CLog::GetInstance().Print(logName, "ch%0.2i: VA_SSA_HI = %0.4X.\r\n", 
			channelId, value);
		break;
	case VA_SSA_LO:
		CLog::GetInstance().Print(logName, "ch%0.2i: VA_SSA_LO = %0.4X.\r\n", 
			channelId, value);
		break;
	case VA_LSAX_HI:
		CLog::GetInstance().Print(logName, "ch%0.2i: VA_LSAX_HI = %0.4X.\r\n", 
			channelId, value);
		break;
	case VA_LSAX_LO:
		CLog::GetInstance().Print(logName, "ch%0.2i: VA_LSAX_LO = %0.4X.\r\n", 
			channelId, value);
		break;
	default:
		CLog::GetInstance().Print(logName, "ch%0.2i: Wrote %0.4X an unknown register 0x%0.4X.\r\n", 
			channelId, value, address);
        break;
    }
}
