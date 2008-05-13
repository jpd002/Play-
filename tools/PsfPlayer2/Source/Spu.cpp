#include <assert.h>
#include "Spu.h"
#include "Log.h"

#define LOG_NAME ("spu")
#define MAX_GENERAL_REG_NAME (28)

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
	"EXT_VOL_RIGHT"
};

CSpu::CSpu() :
m_ram(new uint8[RAMSIZE])
{
	Reset();
}

CSpu::~CSpu()
{
	delete [] m_ram;
}

void CSpu::Reset()
{
	m_status0 = 0;
	m_status1 = 0;
	m_bufferAddr = 0;

	memset(m_ram, 0, RAMSIZE);
}

uint16 CSpu::ReadRegister(uint32 address)
{
#ifdef _DEBUG
	DisassembleRead(address);
#endif
	switch(address)
	{
	case SPU_CTRL0:
		return m_ctrl;
		break;
	case SPU_STATUS0:
		return m_status0;
		break;
	case BUFFER_ADDR:
		return static_cast<int16>(m_bufferAddr / 8);
		break;
	}
	return 0;
}

void CSpu::WriteRegister(uint32 address, uint16 value)
{
	switch(address)
	{
	case SPU_CTRL0:
		m_ctrl = value;
		break;
	case SPU_STATUS0:
		m_status0 = value;
		break;
	case SPU_DATA:
		assert((m_bufferAddr + 1) < RAMSIZE);
		*reinterpret_cast<uint16*>(&m_ram[m_bufferAddr]) = value;
		m_bufferAddr += 2;
		break;
	case BUFFER_ADDR:
		m_bufferAddr = value * 8;
		break;
	}
#ifdef _DEBUG
	DisassembleWrite(address, value);
#endif
}

uint32 CSpu::ReceiveDma(uint8* buffer, uint32 blockSize, uint32 blockAmount)
{
	unsigned int blocksTransfered = 0;
	for(unsigned int i = 0; i < blockAmount; i++)
	{
		memcpy(m_ram + m_bufferAddr, buffer, blockSize);
		m_bufferAddr += blockSize;
		blocksTransfered++;
	}
	return blocksTransfered;
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
