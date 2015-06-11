#include <boost/lexical_cast.hpp>
#include <assert.h>
#include "Iop_Spu2_Core.h"
#include "../Log.h"

#define LOG_NAME_PREFIX ("iop_spu2_core_")
#define SPU_BASE_SAMPLING_RATE (48000)

using namespace Iop;
using namespace Iop::Spu2;

#define MAX_ADDRESS_REGISTER		(22)
#define MAX_COEFFICIENT_REGISTER	(10)

static unsigned int g_addressRegisterMapping[MAX_ADDRESS_REGISTER] =
{
	CSpuBase::FB_SRC_A,
	CSpuBase::FB_SRC_B,
	CSpuBase::IIR_DEST_A0,
	CSpuBase::IIR_DEST_A1,
	CSpuBase::ACC_SRC_A0,
	CSpuBase::ACC_SRC_A1,
	CSpuBase::ACC_SRC_B0,
	CSpuBase::ACC_SRC_B1,
	CSpuBase::IIR_SRC_A0,
	CSpuBase::IIR_SRC_A1,
	CSpuBase::IIR_DEST_B0,
	CSpuBase::IIR_DEST_B1,
	CSpuBase::ACC_SRC_C0,
	CSpuBase::ACC_SRC_C1,
	CSpuBase::ACC_SRC_D0,
	CSpuBase::ACC_SRC_D1,
	CSpuBase::IIR_SRC_B1,
	CSpuBase::IIR_SRC_B0,
	CSpuBase::MIX_DEST_A0,
	CSpuBase::MIX_DEST_A1,
	CSpuBase::MIX_DEST_B0,
	CSpuBase::MIX_DEST_B1
};

static unsigned int g_coefficientRegisterMapping[MAX_COEFFICIENT_REGISTER] =
{
	CSpuBase::IIR_ALPHA,
	CSpuBase::ACC_COEF_A,
	CSpuBase::ACC_COEF_B,
	CSpuBase::ACC_COEF_C,
	CSpuBase::ACC_COEF_D,
	CSpuBase::IIR_COEF,
	CSpuBase::FB_ALPHA,
	CSpuBase::FB_X,
	CSpuBase::IN_COEF_L,
	CSpuBase::IN_COEF_R
};

CCore::CCore(unsigned int coreId, CSpuBase& spuBase)
: m_coreId(coreId)
, m_spuBase(spuBase)
{
	m_logName = LOG_NAME_PREFIX + boost::lexical_cast<std::string>(m_coreId);

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

}

CSpuBase& CCore::GetSpuBase() const
{
	return m_spuBase;
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
	case S_ENDX_HI:
		result = m_spuBase.GetEndFlags().h0;
		break;
	case S_ENDX_LO:
		result = m_spuBase.GetEndFlags().h1;
		break;
	case CORE_ATTR:
		result = m_spuBase.GetControl();
		break;
	case A_TS_MODE:
		result = m_spuBase.GetTransferMode();
		break;
	case A_TSA_HI:
		result = GetAddressHi(m_spuBase.GetTransferAddress());
		break;
	case A_ESA_LO:
		result = GetAddressLo(m_spuBase.GetReverbWorkAddressStart());
		break;
	case A_EEA_HI:
		result = GetAddressHi(m_spuBase.GetReverbWorkAddressEnd());
		break;
	}
	LogRead(address, result);
	return result;
}

uint32 CCore::WriteRegisterCore(unsigned int channelId, uint32 address, uint32 value)
{
	if(address >= RVB_A_REG_BASE && address < RVB_A_REG_END)
	{
		//Address reverb register
		unsigned int regIndex = (address - RVB_A_REG_BASE) / 4;
		assert(regIndex < MAX_ADDRESS_REGISTER);
		unsigned int reverbParamId = g_addressRegisterMapping[regIndex];
		uint32 previousValue = m_spuBase.GetReverbParam(reverbParamId);
		if(address & 0x02)
		{
			value = SetAddressLo(previousValue, static_cast<uint16>(value));
		}
		else
		{
			value = SetAddressHi(previousValue, static_cast<uint16>(value));
		}
		m_spuBase.SetReverbParam(reverbParamId, value);
	}
	else if(address >= RVB_C_REG_BASE && address < RVB_C_REG_END)
	{
		//Coefficient reverb register
		unsigned int regIndex = (address - RVB_C_REG_BASE) / 2;
		assert(regIndex < MAX_COEFFICIENT_REGISTER);
		m_spuBase.SetReverbParam(g_coefficientRegisterMapping[regIndex], value);
	}
	else
	{
		switch(address)
		{
		case CORE_ATTR:
			m_spuBase.SetBaseSamplingRate(SPU_BASE_SAMPLING_RATE);
			m_spuBase.SetControl(static_cast<uint16>(value));
			break;
		case A_STD:
			m_spuBase.WriteWord(static_cast<uint16>(value));
			break;
		case A_TS_MODE:
			m_spuBase.SetTransferMode(static_cast<uint16>(value));
			break;
		case S_VMIXER_HI:
			m_spuBase.SetChannelReverbLo(static_cast<uint16>(value));
			break;
		case S_VMIXER_LO:
			m_spuBase.SetChannelReverbHi(static_cast<uint16>(value));
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
		case S_ENDX_LO:
		case S_ENDX_HI:
			if(value)
			{
				m_spuBase.ClearEndFlags();
			}
			break;
		case A_IRQA_HI:
			m_spuBase.SetIrqAddress(SetAddressHi(m_spuBase.GetIrqAddress(), static_cast<uint16>(value)));
			break;
		case A_IRQA_LO:
			m_spuBase.SetIrqAddress(SetAddressLo(m_spuBase.GetIrqAddress(), static_cast<uint16>(value)));
			break;
		case A_TSA_HI:
			m_spuBase.SetTransferAddress(SetAddressHi(m_spuBase.GetTransferAddress(), static_cast<uint16>(value)));
			break;
		case A_TSA_LO:
			m_spuBase.SetTransferAddress(SetAddressLo(m_spuBase.GetTransferAddress(), static_cast<uint16>(value)));
			break;
		case A_ESA_HI:
			m_spuBase.SetReverbWorkAddressStart(SetAddressHi(m_spuBase.GetReverbWorkAddressStart(), static_cast<uint16>(value)));
			break;
		case A_ESA_LO:
			m_spuBase.SetReverbWorkAddressStart(SetAddressLo(m_spuBase.GetReverbWorkAddressStart(), static_cast<uint16>(value)));
			break;
		case A_EEA_HI:
			m_spuBase.SetReverbWorkAddressEnd(((value & 0x0F) << 17) | 0x1FFFF);
			break;
		}
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
	case VP_VOLL:
		result = channel.volumeLeft;
		break;
	case VP_VOLR:
		result = channel.volumeRight;
		break;
	case VP_PITCH:
		result = channel.pitch;
		break;
	case VP_ADSR1:
		result = channel.adsrLevel;
		break;
	case VP_ADSR2:
		result = channel.adsrRate;
		break;
	case VP_ENVX:
		result = (channel.adsrVolume >> 16);
		break;
	case VP_VOLXL:
		result = (channel.volumeLeftAbs >> 16);
		break;
	case VP_VOLXR:
		result = (channel.volumeRightAbs >> 16);
		break;
	case VA_SSA_HI:
		result = GetAddressHi(channel.address);
		break;
	case VA_SSA_LO:
		result = GetAddressLo(channel.address);
		break;
	case VA_LSAX_HI:
		result = GetAddressHi(channel.repeat);
		break;
	case VA_LSAX_LO:
		result = GetAddressLo(channel.repeat);
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
		if(channel.volumeLeft.mode.mode == 0)
		{
			channel.volumeLeftAbs = channel.volumeLeft.volume.volume << 17;
		}
		break;
	case VP_VOLR:
		channel.volumeRight <<= static_cast<uint16>(value);
		if(channel.volumeRight.mode.mode == 0)
		{
			channel.volumeRightAbs = channel.volumeRight.volume.volume << 17;
		}
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
	case S_PMON_HI:
		CLog::GetInstance().Print(logName, "= S_PMON_HI = 0x%0.4X.\r\n", value);
		break;
	case S_PMON_LO:
		CLog::GetInstance().Print(logName, "= S_PMON_LO = 0x%0.4X.\r\n", value);
		break;
	case S_NON_HI:
		CLog::GetInstance().Print(logName, "= S_NON_HI = 0x%0.4X.\r\n", value);
		break;
	case S_NON_LO:
		CLog::GetInstance().Print(logName, "= S_NON_LO = 0x%0.4X.\r\n", value);
		break;
	case S_VMIXL_HI:
		CLog::GetInstance().Print(logName, "= S_VMIXL_HI = 0x%0.4X.\r\n", value);
		break;
	case S_VMIXL_LO:
		CLog::GetInstance().Print(logName, "= S_VMIXL_LO = 0x%0.4X.\r\n", value);
		break;
	case S_VMIXEL_HI:
		CLog::GetInstance().Print(logName, "= S_VMIXEL_HI = 0x%0.4X.\r\n", value);
		break;
	case S_VMIXEL_LO:
		CLog::GetInstance().Print(logName, "= S_VMIXEL_LO = 0x%0.4X.\r\n", value);
		break;
	case S_VMIXR_HI:
		CLog::GetInstance().Print(logName, "= S_VMIXR_HI = 0x%0.4X.\r\n", value);
		break;
	case S_VMIXR_LO:
		CLog::GetInstance().Print(logName, "= S_VMIXR_LO = 0x%0.4X.\r\n", value);
		break;
	case S_VMIXER_HI:
		CLog::GetInstance().Print(logName, "= S_VMIXER_HI = 0x%0.4X.\r\n", value);
		break;
	case S_VMIXER_LO:
		CLog::GetInstance().Print(logName, "= S_VMIXER_LO = 0x%0.4X.\r\n", value);
		break;
	case S_ENDX_HI:
		CLog::GetInstance().Print(logName, "= S_ENDX_HI = 0x%0.4X.\r\n", value);
		break;
	case S_ENDX_LO:
		CLog::GetInstance().Print(logName, "= S_ENDX_LO = 0x%0.4X.\r\n", value);
		break;
	case A_TSA_HI:
		CLog::GetInstance().Print(logName, "= A_TSA_HI = 0x%0.4X.\r\n", value);
		break;
	case A_TS_MODE:
		CLog::GetInstance().Print(logName, "= A_TS_MODE = 0x%0.4X.\r\n", value);
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
	case S_PMON_HI:
		CLog::GetInstance().Print(logName, "S_PMON_HI = 0x%0.4X\r\n", value);
		break;
	case S_PMON_LO:
		CLog::GetInstance().Print(logName, "S_PMON_LO = 0x%0.4X\r\n", value);
		break;
	case S_NON_HI:
		CLog::GetInstance().Print(logName, "S_NON_HI = 0x%0.4X\r\n", value);
		break;
	case S_NON_LO:
		CLog::GetInstance().Print(logName, "S_NON_LO = 0x%0.4X\r\n", value);
		break;
	case S_VMIXL_HI:
		CLog::GetInstance().Print(logName, "S_VMIXL_HI = 0x%0.4X\r\n", value);
		break;
	case S_VMIXL_LO:
		CLog::GetInstance().Print(logName, "S_VMIXL_LO = 0x%0.4X\r\n", value);
		break;
	case S_VMIXEL_HI:
		CLog::GetInstance().Print(logName, "S_VMIXEL_HI = 0x%0.4X\r\n", value);
		break;
	case S_VMIXEL_LO:
		CLog::GetInstance().Print(logName, "S_VMIXEL_LO = 0x%0.4X\r\n", value);
		break;
	case S_VMIXR_HI:
		CLog::GetInstance().Print(logName, "S_VMIXR_HI = 0x%0.4X\r\n", value);
		break;
	case S_VMIXR_LO:
		CLog::GetInstance().Print(logName, "S_VMIXR_LO = 0x%0.4X\r\n", value);
		break;
	case S_VMIXER_HI:
		CLog::GetInstance().Print(logName, "S_VMIXER_HI = 0x%0.4X\r\n", value);
		break;
	case S_VMIXER_LO:
		CLog::GetInstance().Print(logName, "S_VMIXER_LO = 0x%0.4X\r\n", value);
		break;
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
	case S_ENDX_LO:
		CLog::GetInstance().Print(logName, "S_ENDX_LO = 0x%0.4X\r\n", value);
		break;
	case S_ENDX_HI:
		CLog::GetInstance().Print(logName, "S_ENDX_HI = 0x%0.4X\r\n", value);
		break;
	case A_IRQA_HI:
		CLog::GetInstance().Print(logName, "A_IRQA_HI = 0x%0.4X\r\n", value);
		break;
	case A_IRQA_LO:
		CLog::GetInstance().Print(logName, "A_IRQA_LO = 0x%0.4X\r\n", value);
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
	case A_TS_MODE:
		CLog::GetInstance().Print(logName, "A_TS_MODE = 0x%0.4X\r\n", value);
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
	case VP_VOLL:
		CLog::GetInstance().Print(logName, "ch%0.2i: = VP_VOLL = %0.4X.\r\n", 
			channelId, result);
		break;
	case VP_VOLR:
		CLog::GetInstance().Print(logName, "ch%0.2i: = VP_VOLR = %0.4X.\r\n", 
			channelId, result);
		break;
	case VP_PITCH:
		CLog::GetInstance().Print(logName, "ch%0.2i: = VP_PITCH = %0.4X.\r\n", 
			channelId, result);
		break;
	case VP_ADSR1:
		CLog::GetInstance().Print(logName, "ch%0.2i: = VP_ADSR1 = %0.4X.\r\n", 
			channelId, result);
		break;
	case VP_ADSR2:
		CLog::GetInstance().Print(logName, "ch%0.2i: = VP_ADSR2 = %0.4X.\r\n", 
			channelId, result);
		break;
	case VP_ENVX:
		CLog::GetInstance().Print(logName, "ch%0.2i: = VP_ENVX = 0x%0.4X.\r\n", 
			channelId, result);
		break;
	case VP_VOLXL:
		CLog::GetInstance().Print(logName, "ch%0.2i: = VP_VOLXL = 0x%0.4X.\r\n", 
			channelId, result);
		break;
	case VP_VOLXR:
		CLog::GetInstance().Print(logName, "ch%0.2i: = VP_VOLXR = 0x%0.4X.\r\n", 
			channelId, result);
		break;
	case VA_SSA_HI:
		CLog::GetInstance().Print(logName, "ch%0.2i: = VA_SSA_HI = %0.4X.\r\n", 
			channelId, result);
		break;
	case VA_SSA_LO:
		CLog::GetInstance().Print(logName, "ch%0.2i: = VA_SSA_LO = %0.4X.\r\n", 
			channelId, result);
		break;
	case VA_LSAX_HI:
		CLog::GetInstance().Print(logName, "ch%0.2i: = VA_LSAX_HI = 0x%0.4X.\r\n", 
			channelId, result);
		break;
	case VA_LSAX_LO:
		CLog::GetInstance().Print(logName, "ch%0.2i: = VA_LSAX_LO = 0x%0.4X.\r\n", 
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
