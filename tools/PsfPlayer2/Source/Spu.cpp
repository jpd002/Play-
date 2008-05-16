#include <assert.h>
#include <math.h>
#include "Spu.h"
#include "Log.h"

using namespace std;

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
	m_voiceOn0 = 0;
	m_channelOn0 = 0;

	memset(m_channel, 0, sizeof(m_channel));
	memset(m_ram, 0, RAMSIZE);
}

void CSpu::Render(int16* samples, unsigned int sampleCount, unsigned int sampleRate)
{
	memset(samples, 0, sizeof(int16) * sampleCount);
	int16* bufferTemp = reinterpret_cast<int16*>(_alloca(sizeof(int16) * sampleCount));
	for(unsigned int i = 0; i < 24; i++)
	{
		CHANNEL& channel(m_channel[i]);
		CSampleReader& reader(m_reader[i]);
		if(m_voiceOn0 & (1 << i))
		{
			if(channel.status == STOPPED)
			{
				reader.SetParams(m_ram + (channel.address * 8), m_ram + (channel.repeat * 8), channel.pitch);
				channel.status = KEY_ON;
			}
			reader.GetSamples(bufferTemp, sampleCount, sampleRate);
			//Mix samples
			for(unsigned int j = 0; j < sampleCount; j++)
			{
				//int32 resultSample = (static_cast<int32>(bufferTemp[i]) + static_cast<int32>(samples[i])) / 2;
				int32 resultSample = static_cast<int32>(bufferTemp[i]);
				resultSample = max<int32>(resultSample, SHRT_MIN);
				resultSample = min<int32>(resultSample, SHRT_MAX);
				samples[i] = static_cast<int16>(resultSample);
			}
		}
	}
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
#ifdef _DEBUG
	DisassembleWrite(address, value);
#endif
	if(address >= SPU_GENERAL_BASE)
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
		case VOICE_ON_0:
			m_voiceOn0 = value;
			break;
		case VOICE_OFF_0:
			m_voiceOn0 &= ~value;
			break;
		case CHANNEL_ON_0:
			m_channelOn0 = value;
			break;
		case BUFFER_ADDR:
			m_bufferAddr = value * 8;
			break;
		}
	}
	else
	{
		unsigned int channel = (address - SPU_BEGIN) / 0x10;
		unsigned int registerId = address & 0x0F;
		switch(registerId)
		{
		case CH_VOL_LEFT:
			m_channel[channel].volumeLeft = value;
			break;
		case CH_VOL_RIGHT:
			m_channel[channel].volumeRight = value;
			break;
		case CH_PITCH:
			m_channel[channel].pitch = value;
			break;
		case CH_ADDRESS:
			m_channel[channel].address = value;
			break;
		case CH_ADSR_LEVEL:
			m_channel[channel].adsrLevel = value;
			break;
		case CH_ADSR_RATE:
			m_channel[channel].adsrRate = value;
			break;
		case CH_ADSR_VOLUME:
			m_channel[channel].adsrVolume = value;
			break;
		case CH_REPEAT:
			m_channel[channel].repeat = value;
			break;
		}
	}
}

uint32 CSpu::ReceiveDma(uint8* buffer, uint32 blockSize, uint32 blockAmount)
{
	unsigned int blocksTransfered = 0;
	for(unsigned int i = 0; i < blockAmount; i++)
	{
		memcpy(m_ram + m_bufferAddr, buffer, blockSize);
		m_bufferAddr += blockSize;
		buffer += blockSize;
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


CSpu::CSampleReader::CSampleReader() :
m_nextSample(NULL)
{
	m_done = false;
}

CSpu::CSampleReader::~CSampleReader()
{

}

void CSpu::CSampleReader::SetParams(uint8* address, uint8* repeat, uint16 pitch)
{
	m_currentTime = 0;
	m_nextSample = address;
	m_repeat = repeat;
	m_sourceSamplingRate = 22100;
	m_s1 = 0;
	m_s2 = 0;
	UnpackSamples();
}

void CSpu::CSampleReader::GetSamples(int16* samples, unsigned int sampleCount, unsigned int destSamplingRate)
{
	assert(m_nextSample != NULL);
	double currentDstTime = m_currentTime;
	double dstTimeDelta = 1.0 / static_cast<double>(destSamplingRate);
	for(unsigned int i = 0; i < sampleCount; i++)
	{
		while(currentDstTime > GetNextTime())
		{
			UnpackSamples();
			m_currentTime += GetBufferStep();
		}
		samples[i] = GetSample(currentDstTime);
		currentDstTime += dstTimeDelta;
	}
}

int16 CSpu::CSampleReader::GetSample(double time)
{
	time -= m_currentTime;
	double sample = time * static_cast<double>(m_sourceSamplingRate);
	double sampleInt = 0;
	double alpha = modf(sample, &sampleInt);
	unsigned int sampleIndex = static_cast<int>(sampleInt);
	assert(sampleIndex < BUFFER_SAMPLES);
	if(sampleIndex == BUFFER_SAMPLES - 1)
	{
		return m_buffer[sampleIndex];
	}
	int16 currentSample = m_buffer[sampleIndex];
	int16 nextSample = m_buffer[sampleIndex + 1];
	double resultSample = 
		(static_cast<double>(currentSample) * (1 - alpha)) + (static_cast<double>(nextSample) * alpha);
	return static_cast<int16>(resultSample);
}

void CSpu::CSampleReader::UnpackSamples()
{
	double workBuffer[28];

	//Read header
	uint8 shiftFactor = m_nextSample[0] & 0xF;
	uint8 predictNumber = m_nextSample[0] >> 4;
	uint8 flags = m_nextSample[1];
	assert(predictNumber < 5);

	if(m_done)
	{
		memset(m_buffer, 0, sizeof(int16) * BUFFER_SAMPLES);
		return;
	}

	//Get intermediate values
	{
		unsigned int workBufferPtr = 0;
		for(unsigned int i = 2; i < 16; i++)
		{
			uint8 sampleByte = m_nextSample[i];
			int16 firstSample = ((sampleByte & 0x0F) << 12);
			int16 secondSample = ((sampleByte & 0xF0) << 8);
			firstSample >>= shiftFactor;
			secondSample >>= shiftFactor;
			workBuffer[workBufferPtr++] = firstSample;
			workBuffer[workBufferPtr++] = secondSample;
		}
	}

	//Generate PCM samples
	{
		double predictorTable[5][2] = 
		{
			{	0.0,			0.0				},
			{   60.0 / 64.0,	0.0				},
			{  115.0 / 64.0,	-52.0 / 64.0	},
			{	98.0 / 64.0,	-55.0 / 64.0	},
			{  122.0 / 64.0,	-60.0 / 64.0	},
		};

		for(unsigned int i = 0; i < 28; i++)
		{
			workBuffer[i] = workBuffer[i] +
				m_s1 * predictorTable[predictNumber][0] +
				m_s2 * predictorTable[predictNumber][1];
			m_s2 = m_s1;
			m_s1 = workBuffer[i];
			m_buffer[i] = static_cast<int16>(workBuffer[i] + 0.5);
		}
	}

	if(flags & 0x01)
	{
		m_nextSample = m_repeat;
	}
	else
	{
		m_nextSample += 0x10;
	}
}

double CSpu::CSampleReader::GetBufferStep() const
{
	return static_cast<double>(BUFFER_SAMPLES) / static_cast<double>(m_sourceSamplingRate);
}

double CSpu::CSampleReader::GetNextTime() const
{
	return m_currentTime + GetBufferStep();
}
