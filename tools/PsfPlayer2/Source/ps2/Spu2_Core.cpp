#include <boost/lexical_cast.hpp>
#include <assert.h>
#include "Spu2_Core.h"
#include "Log.h"

#define LOG_NAME_PREFIX ("spu2_core_")
#define SPU_BASE_SAMPLING_RATE (48000)

using namespace Iop;
using namespace Iop::Spu2;
using namespace std;
using namespace std::tr1;
using namespace Framework;
using namespace boost;

CCore::CCore(unsigned int coreId, CSpuBase& spuBase) :
m_coreId(coreId),
m_spuBase(spuBase)
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

}

void CCore::Reset()
{
    m_tempReverb = 0;
    m_tempReverbA = 0;
}

uint16 CCore::GetAddressLo(uint32 address)
{
    return static_cast<uint16>((address >> 1) & 0xFFFF);
}

uint16 CCore::GetAddressHi(uint32 address)
{
    return static_cast<uint16>((address >> (16 + 1)) & 0xFFFF);
}

uint32 CCore::SetAddressLo(uint32 address, uint16 value)
{
    address &= 0xFFFF << (1 + 16);
    address |= value << 1;
    return address;
}

uint32 CCore::SetAddressHi(uint32 address, uint16 value)
{
    address &= 0xFFFF << 1;
    address |= value << (1 + 16);
    return address;
}

uint32 CCore::ReadRegister(uint32 address, uint32 value)
{
	return ProcessRegisterAccess(m_readDispatch, address, value);
}

uint32 CCore::WriteRegister(uint32 address, uint32 value)
{
	return ProcessRegisterAccess(m_writeDispatch, address, value);
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
        if(m_spuBase.GetControl() & CSpuBase::CONTROL_DMA)
		{
			result |= 0x80;
		}
		break;
	case CORE_ATTR:
        result = m_spuBase.GetControl();
		break;
    case A_TSA_HI:
        result = GetAddressHi(m_spuBase.GetTransferAddress());
        break;
	case A_ESA_LO:
		result = (m_tempReverb & 0xFFFF);
		break;
	case A_EEA_HI:
		result = (m_tempReverbA >> 16);
		break;
	}
    LogRead(address, result);
	return result;
}

uint32 CCore::WriteRegisterCore(unsigned int channelId, uint32 address, uint32 value)
{
	switch(address)
	{
	case CORE_ATTR:
		m_spuBase.SetBaseSamplingRate(SPU_BASE_SAMPLING_RATE);
        m_spuBase.SetControl(static_cast<uint16>(value & ~CSpuBase::CONTROL_REVERB));
		break;
    case A_STD:
		m_spuBase.WriteWord(static_cast<uint16>(value));
        break;
    case A_KON_HI:
		m_spuBase.SendKeyOn(value);
        break;
    case A_KON_LO:
        m_spuBase.SendKeyOn(value << 16);
        break;
    case A_KOFF_HI:
        m_spuBase.SendKeyOff(value);
        break;
    case A_KOFF_LO:
		m_spuBase.SendKeyOff(value << 16);
        break;
	case A_TSA_HI:
        m_spuBase.SetTransferAddress(SetAddressHi(m_spuBase.GetTransferAddress(), static_cast<uint16>(value)));
		break;
	case A_TSA_LO:
        m_spuBase.SetTransferAddress(SetAddressLo(m_spuBase.GetTransferAddress(), static_cast<uint16>(value)));
		break;
	case A_ESA_HI:
		m_tempReverb = (m_tempReverb & 0xFFFF) | (value << 16);
		break;
	case A_ESA_LO:
		m_tempReverb = (m_tempReverb & 0xF0000) | (value);
		break;
	case A_EEA_HI:
		m_tempReverbA = ((value & 0x0F) << 16) | 0xFFFF;
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
	uint32 result = 0;
	CSpuBase::CHANNEL& channel(m_spuBase.GetChannel(channelId));
	switch(address)
	{
	case VP_ENVX:
		return (channel.adsrVolume >> 16);
		break;
	case VA_NAX_HI:
        result = GetAddressHi(channel.current);
		break;
	case VA_NAX_LO:
        result = GetAddressLo(channel.current);
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
	LogChannelWrite(channelId, address, value);
	CSpuBase::CHANNEL& channel(m_spuBase.GetChannel(channelId));
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
        channel.address = SetAddressHi(channel.address, static_cast<uint16>(value));
		break;
	case VA_SSA_LO:
        channel.address = SetAddressLo(channel.address, static_cast<uint16>(value));
		break;
	case VA_LSAX_HI:
        channel.repeat = SetAddressHi(channel.repeat, static_cast<uint16>(value));
		break;
	case VA_LSAX_LO:
        channel.repeat = SetAddressLo(channel.repeat, static_cast<uint16>(value));
		break;
	}
	return 0;
}

void CCore::LogRead(uint32 address, uint32 value)
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
        CLog::GetInstance().Print(logName, "= A_TSA_HI = 0x%0.4X.\r\n", value);
        break;
	case A_ESA_LO:
		CLog::GetInstance().Print(logName, "= A_ESA_LO = 0x%0.4X.\r\n", value);
		break;
	case A_EEA_HI:
		CLog::GetInstance().Print(logName, "= A_EEA_HI = 0x%0.4X.\r\n", value);
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
	case A_ESA_LO:
		CLog::GetInstance().Print(logName, "A_ESA_LO = 0x%0.4X\r\n", value);
		break;
	case A_ESA_HI:
		CLog::GetInstance().Print(logName, "A_ESA_HI = 0x%0.4X\r\n", value);
		break;
	case A_EEA_HI:
		CLog::GetInstance().Print(logName, "A_EEA_HI = 0x%0.4X\r\n", value);
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
	case VP_ENVX:
		CLog::GetInstance().Print(logName, "ch%0.2i: = VP_ENVX = 0x%0.4X.\r\n", 
			channelId, result);
		break;
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
