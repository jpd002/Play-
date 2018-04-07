#include <assert.h>
#include "Iop_Spu2_Core.h"
#include "../Log.h"
#include "string_format.h"

#define LOG_NAME_FORMAT ("iop_spu2_core_%d")
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
	m_logName = string_format(LOG_NAME_FORMAT, m_coreId);

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
	auto logName = m_logName.c_str();
#define LOG_GET(registerId) case registerId: CLog::GetInstance().Print(logName, "= " #registerId " = 0x%04X\r\n", value); break;

	switch(address)
	{
		LOG_GET(CORE_ATTR)
		LOG_GET(STATX)
		LOG_GET(S_PMON_HI)
		LOG_GET(S_PMON_LO)
		LOG_GET(S_NON_HI)
		LOG_GET(S_NON_LO)
		LOG_GET(S_VMIXL_HI)
		LOG_GET(S_VMIXL_LO)
		LOG_GET(S_VMIXEL_HI)
		LOG_GET(S_VMIXEL_LO)
		LOG_GET(S_VMIXR_HI)
		LOG_GET(S_VMIXR_LO)
		LOG_GET(S_VMIXER_HI)
		LOG_GET(S_VMIXER_LO)
		LOG_GET(S_ENDX_HI)
		LOG_GET(S_ENDX_LO)
		LOG_GET(A_TSA_HI)
		LOG_GET(A_TSA_LO)
		LOG_GET(A_TS_MODE)
		LOG_GET(A_ESA_HI)
		LOG_GET(A_ESA_LO)
		LOG_GET(A_EEA_HI)
		LOG_GET(A_EEA_LO)

	default:
		CLog::GetInstance().Print(logName, "Read an unknown register 0x%04X.\r\n", address);
		break;
	}

#undef LOG_GET
}

void CCore::LogWrite(uint32 address, uint32 value)
{
	auto logName = m_logName.c_str();
#define LOG_SET(registerId) case registerId: CLog::GetInstance().Print(logName, #registerId " = 0x%04X\r\n", value); break;

	switch(address)
	{
		LOG_SET(S_PMON_HI)
		LOG_SET(S_PMON_LO)
		LOG_SET(S_NON_HI)
		LOG_SET(S_NON_LO)
		LOG_SET(S_VMIXL_HI)
		LOG_SET(S_VMIXL_LO)
		LOG_SET(S_VMIXEL_HI)
		LOG_SET(S_VMIXEL_LO)
		LOG_SET(S_VMIXR_HI)
		LOG_SET(S_VMIXR_LO)
		LOG_SET(S_VMIXER_HI)
		LOG_SET(S_VMIXER_LO)
		LOG_SET(P_MMIX)
		LOG_SET(CORE_ATTR)
		LOG_SET(A_KON_HI)
		LOG_SET(A_KON_LO)
		LOG_SET(A_KOFF_HI)
		LOG_SET(A_KOFF_LO)
		LOG_SET(S_ENDX_HI)
		LOG_SET(S_ENDX_LO)
		LOG_SET(A_IRQA_HI)
		LOG_SET(A_IRQA_LO)
		LOG_SET(A_TSA_HI)
		LOG_SET(A_TSA_LO)
		LOG_SET(A_STD)
		LOG_SET(A_TS_MODE)
		LOG_SET(A_ESA_HI)
		LOG_SET(A_ESA_LO)
		LOG_SET(A_EEA_HI)
		LOG_SET(A_EEA_LO)
		LOG_SET(P_MVOLL)
		LOG_SET(P_MVOLR)
		LOG_SET(P_EVOLL)
		LOG_SET(P_EVOLR)
		LOG_SET(P_BVOLL)
		LOG_SET(P_BVOLR)

	default:
		CLog::GetInstance().Print(logName, "Write 0x%04X to an unknown register 0x%04X.\r\n", value, address);
		break;
	}

#undef LOG_SET
}

void CCore::LogChannelRead(unsigned int channelId, uint32 address, uint32 value)
{
	auto logName = m_logName.c_str();
#define LOG_GET(registerId) case registerId: CLog::GetInstance().Print(logName, "ch%02d: = " #registerId " = 0x%04X\r\n", channelId, value); break;

	switch(address)
	{
		LOG_GET(VP_VOLL)
		LOG_GET(VP_VOLR)
		LOG_GET(VP_PITCH)
		LOG_GET(VP_ADSR1)
		LOG_GET(VP_ADSR2)
		LOG_GET(VP_ENVX)
		LOG_GET(VP_VOLXL)
		LOG_GET(VP_VOLXR)
		LOG_GET(VA_SSA_HI)
		LOG_GET(VA_SSA_LO)
		LOG_GET(VA_LSAX_HI)
		LOG_GET(VA_LSAX_LO)
		LOG_GET(VA_NAX_HI)
		LOG_GET(VA_NAX_LO)

	default:
		CLog::GetInstance().Print(logName, "ch%02d: Read an unknown register 0x%04X.\r\n", 
			channelId, address);
		break;
	}

#undef LOG_GET
}

void CCore::LogChannelWrite(unsigned int channelId, uint32 address, uint32 value)
{
	auto logName = m_logName.c_str();
#define LOG_SET(registerId) case registerId: CLog::GetInstance().Print(logName, "ch%02d: " #registerId " = 0x%04X\r\n", channelId, value); break;

	switch(address)
	{
		LOG_SET(VP_VOLL)
		LOG_SET(VP_VOLR)
		LOG_SET(VP_PITCH)
		LOG_SET(VP_ADSR1)
		LOG_SET(VP_ADSR2)
		LOG_SET(VP_ENVX)
		LOG_SET(VP_VOLXL)
		LOG_SET(VP_VOLXR)
		LOG_SET(VA_SSA_HI)
		LOG_SET(VA_SSA_LO)
		LOG_SET(VA_LSAX_HI)
		LOG_SET(VA_LSAX_LO)

	default:
		CLog::GetInstance().Print(logName, "ch%02d: Wrote %04X to an unknown register 0x%04X.\r\n", 
			channelId, value, address);
		break;
	}

#undef LOG_SET
}
