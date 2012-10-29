#include <assert.h>
#include <math.h>
#include "Iop_Spu.h"
#include "../Log.h"

using namespace std;
using namespace Iop;

#define LOG_NAME ("spu")
#define MAX_GENERAL_REG_NAME (64)

static const char* g_channelRegisterName[8] = 
{
	"VOL_LEFT",
	"VOL_RIGHT",
	"PITCH",
	"ADDRESS",
	"ADSR_LEVEL",
	"ADSR_RATE",
	"ADSR_VOLUME",
	"REPEAT"
};

static const char* g_generalRegisterName[MAX_GENERAL_REG_NAME] =
{
	"MAIN_VOL_LEFT",
	"MAIN_VOL_RIGHT",
	"REVERB_LEFT",
	"REVERB_RIGHT",
	"VOICE_ON_0",
	"VOICE_ON_1",
	"VOICE_OFF_0",
	"VOICE_OFF_1",
	"FM_0",
	"FM_1",
	"NOISE_0",
	"NOISE_1",
	"REVERB_0",
	"REVERB_1",
	"CHANNEL_ON_0",
	"CHANNEL_ON_1",
	NULL,
	"REVERB_WORK",
	"IRQ_ADDR",
	"BUFFER_ADDR",
	"SPU_DATA",
	"SPU_CTRL0",
	"SPU_STATUS0",
	"SPU_STATUS1",
	"CD_VOL_LEFT",
	"CD_VOL_RIGHT",
	"EXT_VOL_LEFT",
	"EXT_VOL_RIGHT",
	NULL,
	NULL,
	NULL,
	NULL,
	"FB_SRC_A",
	"FB_SRC_B",
	"IIR_ALPHA",
	"ACC_COEF_A",
	"ACC_COEF_B",
	"ACC_COEF_C",
	"ACC_COEF_D",
	"IIR_COEF",
	"FB_ALPHA",
	"FB_X",
	"IIR_DEST_A0",
	"IIR_DEST_A1",
	"ACC_SRC_A0",
	"ACC_SRC_A1",
	"ACC_SRC_B0",
	"ACC_SRC_B1",
	"IIR_SRC_A0",
	"IIR_SRC_A1",
	"IIR_DEST_B0",
	"IIR_DEST_B1",
	"ACC_SRC_C0",
	"ACC_SRC_C1",
	"ACC_SRC_D0",
	"ACC_SRC_D1",
	"IIR_SRC_B1",
	"IIR_SRC_B0",
	"MIX_DEST_A0",
	"MIX_DEST_A1",
	"MIX_DEST_B0",
	"MIX_DEST_B1",
	"IN_COEF_L",
	"IN_COEF_R"
};

CSpu::CSpu(CSpuBase& base) :
m_base(base)
{
	Reset();
}

CSpu::~CSpu()
{

}

void CSpu::Reset()
{
	m_status0 = 0;
	m_status1 = 0;
}

uint16 CSpu::ReadRegister(uint32 address)
{
#ifdef _DEBUG
	DisassembleRead(address);
#endif
	if(address >= SPU_GENERAL_BASE)
	{
		switch(address)
		{
		case SPU_CTRL0:
			return m_base.GetControl();
			break;
		case SPU_STATUS0:
			return m_status0;
			break;
		case REVERB_0:
			return m_base.GetChannelReverb().h0;
			break;
		case REVERB_1:
			return m_base.GetChannelReverb().h1;
			break;
		case BUFFER_ADDR:
			return static_cast<int16>(m_base.GetTransferAddress() / 8);
			break;
		}
	}
	else
	{
		unsigned int channelId = (address - SPU_BEGIN) / 0x10;
		unsigned int registerId = address & 0x0F;
		CSpuBase::CHANNEL& channel(m_base.GetChannel(channelId));
		switch(registerId)
		{
		case CH_ADSR_LEVEL:
			return static_cast<uint16>(channel.adsrLevel);
			break;
		case CH_ADSR_RATE:
			return static_cast<uint16>(channel.adsrRate);
			break;
		case CH_ADSR_VOLUME:
			return static_cast<uint16>(channel.adsrVolume >> 16);
			break;
		}
	}
	return 0;
}

void CSpu::WriteRegister(uint32 address, uint16 value)
{
#ifdef _DEBUG
	DisassembleWrite(address, value);
#endif
	if(address >= REVERB_START && address < REVERB_END)
	{
		uint32 registerId = (address - REVERB_START) / 2;
		uint32 result = value;
		if(CSpuBase::g_reverbParamIsAddress[registerId])
		{
			result *= 8;
		}
		m_base.SetReverbParam(registerId, result);
	}
	else if(address >= SPU_GENERAL_BASE)
	{
		switch(address)
		{
		case SPU_CTRL0:
			m_base.SetControl(value);
			break;
		case SPU_STATUS0:
			m_status0 = value;
			break;
		case SPU_DATA:
			m_base.WriteWord(value);
			break;
		case VOICE_ON_0:
			m_base.SendKeyOn(value);
			break;
		case VOICE_ON_1:
			m_base.SendKeyOn(value << 16);
			break;
		case VOICE_OFF_0:
			m_base.SendKeyOff(value);
			break;
		case VOICE_OFF_1:
			m_base.SendKeyOff(value << 16);
			break;
		case CHANNEL_ON_0:
			m_base.SetChannelOnLo(value);
			break;
		case CHANNEL_ON_1:
			m_base.SetChannelOnHi(value);
			break;
		case REVERB_0:
			m_base.SetChannelReverbLo(value);
			break;
		case REVERB_1:
			m_base.SetChannelReverbHi(value);
			break;
		case REVERB_WORK:
			m_base.SetReverbWorkAddressStart(value * 8);
			break;
		case BUFFER_ADDR:
			m_base.SetTransferAddress(value * 8);
			break;
		}
	}
	else
	{
		unsigned int channelId = (address - SPU_BEGIN) / 0x10;
		unsigned int registerId = address & 0x0F;
		CSpuBase::CHANNEL& channel(m_base.GetChannel(channelId));
		switch(registerId)
		{
		case CH_VOL_LEFT:
			channel.volumeLeft <<= value;
			break;
		case CH_VOL_RIGHT:
			channel.volumeRight <<= value;
			break;
		case CH_PITCH:
			channel.pitch = value;
			break;
		case CH_ADDRESS:
			channel.address = value * 8;
			channel.current = value * 8;
			break;
		case CH_ADSR_LEVEL:
			channel.adsrLevel <<= value;
			break;
		case CH_ADSR_RATE:
			channel.adsrRate <<= value;
			break;
		case CH_REPEAT:
			channel.repeat = value * 8;
			break;
		}
	}
}

void CSpu::DisassembleRead(uint32 address)
{
	if(address >= SPU_GENERAL_BASE)
	{
		bool invalid = (address & 1) != 0;
		unsigned int registerId = (address - SPU_GENERAL_BASE) / 2;
		if(invalid || registerId >= MAX_GENERAL_REG_NAME)
		{
			CLog::GetInstance().Print(LOG_NAME, "Read an unknown register (0x%0.8X).\r\n", address);
		}
		else
		{
			CLog::GetInstance().Print(LOG_NAME, "= %s\r\n",
				g_generalRegisterName[registerId]);
		}
	}
	else
	{
		unsigned int channel = (address - SPU_BEGIN) / 0x10;
		unsigned int registerId = address & 0x0F;
		if(address & 0x01)
		{
			CLog::GetInstance().Print(LOG_NAME, "CH%i : Read an unknown register (0x%X).\r\n", channel, registerId);
		}
		else
		{
			CLog::GetInstance().Print(LOG_NAME, "CH%i : = %s\r\n", 
				channel, g_channelRegisterName[registerId / 2]);
		}
	}
}

void CSpu::DisassembleWrite(uint32 address, uint16 value)
{
	if(address >= SPU_GENERAL_BASE)
	{
		bool invalid = (address & 1) != 0;
		unsigned int registerId = (address - SPU_GENERAL_BASE) / 2;
		if(invalid || registerId >= MAX_GENERAL_REG_NAME)
		{
			CLog::GetInstance().Print(LOG_NAME, "Wrote to an unknown register (0x%0.8X, 0x%0.4X).\r\n", address, value);
		}
		else
		{
			CLog::GetInstance().Print(LOG_NAME, "%s = 0x%0.4X\r\n",
				g_generalRegisterName[registerId], value);
		}
	}
	else
	{
		unsigned int channel = (address - SPU_BEGIN) / 0x10;
		unsigned int registerId = address & 0x0F;
		if(address & 0x1)
		{
			CLog::GetInstance().Print(LOG_NAME, "CH%i : Wrote to an unknown register (0x%X, 0x%0.4X).\r\n", 
				channel, registerId, value);
		}
		else
		{
			CLog::GetInstance().Print(LOG_NAME, "CH%i : %s = 0x%0.4X\r\n", 
				channel, g_channelRegisterName[registerId / 2], value);
		}
	}
}
