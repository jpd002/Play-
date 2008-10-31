#include <assert.h>
#include <math.h>
#include "Spu.h"
#include "Log.h"

using namespace std;

#define LOG_NAME ("spu")
#define MAX_GENERAL_REG_NAME (64)

//#define SAMPLE_RATE (44100)
#define SAMPLE_RATE (48000)

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

CSpu::CSpu() :
m_ram(new uint8[RAMSIZE])
{
	Reset();

	//Init log table for ADSR
	memset(m_adsrLogTable, 0, sizeof(m_adsrLogTable));

	uint32 value = 3;
	uint32 columnIncrement = 1;
	uint32 column = 0;

	for(unsigned int i = 32; i < 160; i++)
	{
		if(value < 0x3FFFFFFF)
		{
			value += columnIncrement;
			column++;
			if(column == 5)
			{
				column = 1;
				columnIncrement *= 2;
			}
		}
		else
		{
			value = 0x3FFFFFFF;
		}
		m_adsrLogTable[i] = value;
	}
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
	m_channelOn.f = 0;
	m_channelReverb.f = 0;
	m_reverbTicks = 0;

	m_reverbCurrAddr = 0;
	m_reverbWorkAddr = 0;

	memset(m_channel, 0, sizeof(m_channel));
	memset(m_ram, 0, RAMSIZE);
	memset(m_reverb, 0, sizeof(m_reverb));
}

uint32 CSpu::GetChannelOn() const
{
	return m_channelOn.f;
}

uint32 CSpu::GetChannelReverb() const
{
	return m_channelReverb.f;
}

CSpu::CHANNEL& CSpu::GetChannel(unsigned int channelNumber)
{
	return m_channel[channelNumber];
}

uint32 CSpu::GetTransferAddress() const
{
	return m_bufferAddr;
}

void CSpu::SetTransferAddress(uint32 value)
{
	m_bufferAddr = value;
}

void CSpu::Render(int16* samples, unsigned int sampleCount, unsigned int sampleRate)
{
	struct SampleMixer
	{
		void operator() (int32 inputSample, const CHANNEL_VOLUME& volume, int16* output) const
		{
			if(!volume.mode.mode)
			{
				inputSample = (inputSample * static_cast<int32>(volume.volume.volume)) / 0x3FFF;
			}
			int32 resultSample = inputSample + static_cast<int32>(*output);
			resultSample = max<int32>(resultSample, SHRT_MIN);
			resultSample = min<int32>(resultSample, SHRT_MAX);
			*output = static_cast<int16>(resultSample);
		}
	};

	assert((sampleCount & 0x01) == 0);
	//ticks are 44100Hz ticks
	unsigned int ticks = sampleCount / 2;
	memset(samples, 0, sizeof(int16) * sampleCount);
//	int16* bufferTemp = reinterpret_cast<int16*>(_alloca(sizeof(int16) * ticks));
	for(unsigned int j = 0; j < ticks; j++)
	{
		int16 reverbSample[2] = { 0, 0 };
		//Update channels
		for(unsigned int i = 0; i < 24; i++)
		{
			CHANNEL& channel(m_channel[i]);
			if(channel.status == STOPPED) continue;
			CSampleReader& reader(m_reader[i]);
			if(channel.status == KEY_ON)
			{
				reader.SetParams(m_ram + (channel.address * 8), m_ram + (channel.repeat * 8));
				channel.status = ATTACK;
				channel.adsrVolume = 0;
			}
			else
			{
				uint8* repeat = reader.GetRepeat();
				channel.repeat = (repeat - m_ram) / 8;
			}
			int16 readSample = 0;
			reader.SetPitch(channel.pitch);
			reader.GetSamples(&readSample, 1, sampleRate);
            channel.current = (reader.GetCurrent() - m_ram);
			//Mix samples
			UpdateAdsr(channel);
			int32 inputSample = static_cast<int32>(readSample);
			//Mix adsrVolume
			{
				int64 result = (static_cast<int64>(inputSample) * static_cast<int64>(channel.adsrVolume)) / static_cast<int64>(MAX_ADSR_VOLUME);
				inputSample = static_cast<int32>(result);
			}
			SampleMixer()(inputSample, channel.volumeLeft,	samples + 0);
			SampleMixer()(inputSample, channel.volumeRight, samples + 1);
			//Mix in reverb if enabled for this channel
			if(m_channelReverb.f & (1 << i))
			{
				SampleMixer()(inputSample, channel.volumeLeft,	reverbSample + 0);
				SampleMixer()(inputSample, channel.volumeRight, reverbSample + 1);
			}
		}
		//Update reverb
		{
			//Feed samples to FIR filter
			if(m_reverbTicks & 1)
			{
				if(m_ctrl & 0x80)
				{
					//IIR_INPUT_A0 = buffer[IIR_SRC_A0] * IIR_COEF + INPUT_SAMPLE_L * IN_COEF_L;
					//IIR_INPUT_A1 = buffer[IIR_SRC_A1] * IIR_COEF + INPUT_SAMPLE_R * IN_COEF_R;
					//IIR_INPUT_B0 = buffer[IIR_SRC_B0] * IIR_COEF + INPUT_SAMPLE_L * IN_COEF_L;
					//IIR_INPUT_B1 = buffer[IIR_SRC_B1] * IIR_COEF + INPUT_SAMPLE_R * IN_COEF_R;

					float input_sample_l = static_cast<float>(reverbSample[0]) * 0.5f;
					float input_sample_r = static_cast<float>(reverbSample[1]) * 0.5f;

					float irr_coef = GetReverbCoef(IIR_COEF);
					float in_coef_l = GetReverbCoef(IN_COEF_L);
					float in_coef_r = GetReverbCoef(IN_COEF_R);

					float iir_input_a0 = GetReverbSample(GetReverbOffset(ACC_SRC_A0)) * irr_coef + input_sample_l * in_coef_l;
					float iir_input_a1 = GetReverbSample(GetReverbOffset(ACC_SRC_A1)) * irr_coef + input_sample_r * in_coef_r;
					float iir_input_b0 = GetReverbSample(GetReverbOffset(ACC_SRC_B0)) * irr_coef + input_sample_l * in_coef_l;
					float iir_input_b1 = GetReverbSample(GetReverbOffset(ACC_SRC_B1)) * irr_coef + input_sample_r * in_coef_r;

					//IIR_A0 = IIR_INPUT_A0 * IIR_ALPHA + buffer[IIR_DEST_A0] * (1.0 - IIR_ALPHA);
					//IIR_A1 = IIR_INPUT_A1 * IIR_ALPHA + buffer[IIR_DEST_A1] * (1.0 - IIR_ALPHA);
					//IIR_B0 = IIR_INPUT_B0 * IIR_ALPHA + buffer[IIR_DEST_B0] * (1.0 - IIR_ALPHA);
					//IIR_B1 = IIR_INPUT_B1 * IIR_ALPHA + buffer[IIR_DEST_B1] * (1.0 - IIR_ALPHA);

					float iir_alpha = GetReverbCoef(IIR_ALPHA);

					float iir_a0 = iir_input_a0 * iir_alpha + GetReverbSample(GetReverbOffset(IIR_DEST_A0)) * (1.0f - iir_alpha);
					float iir_a1 = iir_input_a1 * iir_alpha + GetReverbSample(GetReverbOffset(IIR_DEST_A1)) * (1.0f - iir_alpha);
					float iir_b0 = iir_input_b0 * iir_alpha + GetReverbSample(GetReverbOffset(IIR_DEST_B0)) * (1.0f - iir_alpha);
					float iir_b1 = iir_input_b1 * iir_alpha + GetReverbSample(GetReverbOffset(IIR_DEST_B1)) * (1.0f - iir_alpha);

					//buffer[IIR_DEST_A0 + 1sample] = IIR_A0;
					//buffer[IIR_DEST_A1 + 1sample] = IIR_A1;
					//buffer[IIR_DEST_B0 + 1sample] = IIR_B0;
					//buffer[IIR_DEST_B1 + 1sample] = IIR_B1;

					SetReverbSample(GetReverbOffset(IIR_DEST_A0) + 2, iir_a0);
					SetReverbSample(GetReverbOffset(IIR_DEST_A1) + 2, iir_a1);
					SetReverbSample(GetReverbOffset(IIR_DEST_B0) + 2, iir_b0);
					SetReverbSample(GetReverbOffset(IIR_DEST_B1) + 2, iir_b1);

					//ACC0 = buffer[ACC_SRC_A0] * ACC_COEF_A +
					//	   buffer[ACC_SRC_B0] * ACC_COEF_B +
					//	   buffer[ACC_SRC_C0] * ACC_COEF_C +
					//	   buffer[ACC_SRC_D0] * ACC_COEF_D;
					//ACC1 = buffer[ACC_SRC_A1] * ACC_COEF_A +
					//	   buffer[ACC_SRC_B1] * ACC_COEF_B +
					//	   buffer[ACC_SRC_C1] * ACC_COEF_C +
					//	   buffer[ACC_SRC_D1] * ACC_COEF_D;

					float acc_coef_a = GetReverbCoef(ACC_COEF_A);
					float acc_coef_b = GetReverbCoef(ACC_COEF_B);
					float acc_coef_c = GetReverbCoef(ACC_COEF_C);
					float acc_coef_d = GetReverbCoef(ACC_COEF_D);

					float acc0 = 
						GetReverbSample(GetReverbOffset(ACC_SRC_A0)) * acc_coef_a +
						GetReverbSample(GetReverbOffset(ACC_SRC_B0)) * acc_coef_b +
						GetReverbSample(GetReverbOffset(ACC_SRC_C0)) * acc_coef_c +
						GetReverbSample(GetReverbOffset(ACC_SRC_D0)) * acc_coef_d;

					float acc1 = 
						GetReverbSample(GetReverbOffset(ACC_SRC_A1)) * acc_coef_a +
						GetReverbSample(GetReverbOffset(ACC_SRC_B1)) * acc_coef_b +
						GetReverbSample(GetReverbOffset(ACC_SRC_C1)) * acc_coef_c +
						GetReverbSample(GetReverbOffset(ACC_SRC_D1)) * acc_coef_d;

					//FB_A0 = buffer[MIX_DEST_A0 - FB_SRC_A];
					//FB_A1 = buffer[MIX_DEST_A1 - FB_SRC_A];
					//FB_B0 = buffer[MIX_DEST_B0 - FB_SRC_B];
					//FB_B1 = buffer[MIX_DEST_B1 - FB_SRC_B];

					float fb_a0 = GetReverbSample(GetReverbOffset(MIX_DEST_A0) - GetReverbOffset(FB_SRC_A));
					float fb_a1 = GetReverbSample(GetReverbOffset(MIX_DEST_A1) - GetReverbOffset(FB_SRC_A));
					float fb_b0 = GetReverbSample(GetReverbOffset(MIX_DEST_B0) - GetReverbOffset(FB_SRC_B));
					float fb_b1 = GetReverbSample(GetReverbOffset(MIX_DEST_B1) - GetReverbOffset(FB_SRC_B));

					//buffer[MIX_DEST_A0] = ACC0 - FB_A0 * FB_ALPHA;
					//buffer[MIX_DEST_A1] = ACC1 - FB_A1 * FB_ALPHA;
					//buffer[MIX_DEST_B0] = (FB_ALPHA * ACC0) - FB_A0 * (FB_ALPHA^0x8000) - FB_B0 * FB_X;
					//buffer[MIX_DEST_B1] = (FB_ALPHA * ACC1) - FB_A1 * (FB_ALPHA^0x8000) - FB_B1 * FB_X;	

					float fb_alpha = GetReverbCoef(FB_ALPHA);
					float fb_x = GetReverbCoef(FB_X);

					SetReverbSample(GetReverbOffset(MIX_DEST_A0), acc0 - fb_a0 * fb_alpha);
					SetReverbSample(GetReverbOffset(MIX_DEST_A1), acc1 - fb_a1 * fb_alpha);
					SetReverbSample(GetReverbOffset(MIX_DEST_B0), (fb_alpha * acc0) - fb_a0 * -fb_alpha - fb_b0 * fb_x);
					SetReverbSample(GetReverbOffset(MIX_DEST_B1), (fb_alpha * acc1) - fb_a1 * -fb_alpha - fb_b1 * fb_x);
				}
				m_reverbCurrAddr += 2;
				if(m_reverbCurrAddr >= RAMSIZE)
				{
					m_reverbCurrAddr = m_reverbWorkAddr;
				}
			}

			float sampleL = 0.333f * (GetReverbSample(GetReverbOffset(MIX_DEST_A0)) + GetReverbSample(GetReverbOffset(MIX_DEST_B0)));
			float sampleR = 0.333f * (GetReverbSample(GetReverbOffset(MIX_DEST_A1)) + GetReverbSample(GetReverbOffset(MIX_DEST_B1)));

			{
				int16* output = samples + 0;
				int32 resultSample = static_cast<int32>(sampleL) + static_cast<int32>(*output);
				resultSample = max<int32>(resultSample, SHRT_MIN);
				resultSample = min<int32>(resultSample, SHRT_MAX);
				*output = static_cast<int16>(resultSample);
			}

			{
				int16* output = samples + 1;
				int32 resultSample = static_cast<int32>(sampleR) + static_cast<int32>(*output);
				resultSample = max<int32>(resultSample, SHRT_MIN);
				resultSample = min<int32>(resultSample, SHRT_MAX);
				*output = static_cast<int16>(resultSample);
			}

			m_reverbTicks++;
		}
		samples += 2;
	}
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
			return m_ctrl;
			break;
		case SPU_STATUS0:
			return m_status0;
			break;
		case REVERB_0:
			return m_channelReverb.h0;
			break;
		case REVERB_1:
			return m_channelReverb.h1;
			break;
		case BUFFER_ADDR:
			return static_cast<int16>(m_bufferAddr / 8);
			break;
		}
	}
	else
	{
		unsigned int channel = (address - SPU_BEGIN) / 0x10;
		unsigned int registerId = address & 0x0F;
		switch(registerId)
		{
		case CH_ADSR_VOLUME:
			return static_cast<uint16>(m_channel[channel].adsrVolume >> 16);
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
		m_reverb[registerId] = value;
	}
	else if(address >= SPU_GENERAL_BASE)
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
			SendKeyOn(value);
			break;
		case VOICE_ON_1:
			SendKeyOn(value << 16);
			break;
		case VOICE_OFF_0:
			SendKeyOff(value);
			break;
		case VOICE_OFF_1:
			SendKeyOff(value << 16);
			break;
		case CHANNEL_ON_0:
			m_channelOn.h0 = value;
			break;
		case CHANNEL_ON_1:
			m_channelOn.h1 = value;
			break;
		case REVERB_0:
			m_channelReverb.h0 = value;
			break;
		case REVERB_1:
			m_channelReverb.h1 = value;
			break;
		case REVERB_WORK:
			m_reverbWorkAddr = value * 8;
			m_reverbCurrAddr = m_reverbWorkAddr;
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
			m_channel[channel].volumeLeft <<= value;
			break;
		case CH_VOL_RIGHT:
			m_channel[channel].volumeRight <<= value;
			break;
		case CH_PITCH:
			m_channel[channel].pitch = value;
			break;
		case CH_ADDRESS:
			m_channel[channel].address = value;
			break;
		case CH_ADSR_LEVEL:
			m_channel[channel].adsrLevel <<= value;
			break;
		case CH_ADSR_RATE:
			m_channel[channel].adsrRate <<= value;
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
		uint32 copySize = min<uint32>(RAMSIZE - m_bufferAddr, blockSize);
		memcpy(m_ram + m_bufferAddr, buffer, copySize);
		m_bufferAddr += blockSize;
		m_bufferAddr &= RAMSIZE - 1;
		buffer += blockSize;
		blocksTransfered++;
	}
	return blocksTransfered;
}

void CSpu::SendKeyOn(uint32 channels)
{
	for(unsigned int i = 0; i < MAX_CHANNEL; i++)
	{
		CHANNEL& channel = m_channel[i];
		if(channels & (1 << i))
		{
			channel.status = KEY_ON;
		}
	}
}

void CSpu::SendKeyOff(uint32 channels)
{
	for(unsigned int i = 0; i < MAX_CHANNEL; i++)
	{
		CHANNEL& channel = m_channel[i];
		if(channels & (1 << i))
		{
			if(channel.status == STOPPED) continue;
			if(channel.status == KEY_ON)
			{
				channel.status = STOPPED;
			}
			else
			{
				channel.status = RELEASE;
			}
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

uint32 CSpu::GetAdsrDelta(unsigned int index) const
{
	return m_adsrLogTable[index + 32];
}

float CSpu::GetReverbSample(uint32 address) const
{
	uint32 absoluteAddress = m_reverbCurrAddr + address;
	while(absoluteAddress >= RAMSIZE)
	{
		absoluteAddress -= RAMSIZE;
		absoluteAddress += m_reverbWorkAddr;
	}
	return static_cast<float>(*reinterpret_cast<int16*>(m_ram + absoluteAddress));
}

void CSpu::SetReverbSample(uint32 address, float value)
{
	uint32 absoluteAddress = m_reverbCurrAddr + address;
	while(absoluteAddress >= RAMSIZE)
	{
		absoluteAddress -= RAMSIZE;
		absoluteAddress += m_reverbWorkAddr;
	}
	assert(absoluteAddress < RAMSIZE);
	value = max<float>(value, SHRT_MIN);
	value = min<float>(value, SHRT_MAX);
	int16 intValue = static_cast<int16>(value);
	*reinterpret_cast<int16*>(m_ram + absoluteAddress) = intValue;
}

uint32 CSpu::GetReverbOffset(unsigned int registerId) const
{
	uint16 value = m_reverb[registerId];
	return value * 8;
}

float CSpu::GetReverbCoef(unsigned int registerId) const
{
	int16 value = m_reverb[registerId];
	return static_cast<float>(value) / static_cast<float>(0x8000);
}

void CSpu::UpdateAdsr(CHANNEL& channel)
{
	static unsigned int logIndex[8] = { 0, 4, 6, 8, 9, 10, 11, 12 };
	int64 currentAdsrLevel = channel.adsrVolume;
	if(channel.status == ATTACK)
	{
		if(channel.adsrLevel.attackMode == 0)
		{
			//Linear mode
			currentAdsrLevel += GetAdsrDelta((channel.adsrLevel.attackRate ^ 0x7F) - 0x10);
		}
		else
		{
			if(currentAdsrLevel < 0x60000000)
			{
				currentAdsrLevel += GetAdsrDelta((channel.adsrLevel.attackRate ^ 0x7F) - 0x10);
			}
			else
			{
				currentAdsrLevel += GetAdsrDelta((channel.adsrLevel.attackRate ^ 0x7F) - 0x18);
			}
		}
		//Terminasion condition
		if(currentAdsrLevel >= MAX_ADSR_VOLUME)
		{
			channel.status = DECAY;
		}
	}
	else if(channel.status == DECAY)
	{
		unsigned int decayType = (static_cast<uint32>(currentAdsrLevel) >> 28) & 0x7;
		currentAdsrLevel -= GetAdsrDelta((4 * (channel.adsrLevel.decayRate ^ 0x1F)) - 0x18 + logIndex[decayType]);
		//Terminasion condition
		if(((currentAdsrLevel >> 27) & 0xF) <= channel.adsrLevel.sustainLevel)
		{
			channel.status = SUSTAIN;
		}
	}
	else if(channel.status == SUSTAIN)
	{
		if(channel.adsrRate.sustainDirection == 0)
		{
			//Increment
			if(channel.adsrRate.sustainMode == 0)
			{
				currentAdsrLevel += GetAdsrDelta((channel.adsrRate.sustainRate ^ 0x7F) - 0x10);
			}
			else
			{
				if(currentAdsrLevel < 0x60000000)
				{
					currentAdsrLevel += GetAdsrDelta((channel.adsrRate.sustainRate ^ 0x7F) - 0x10);
				}
				else
				{
					currentAdsrLevel += GetAdsrDelta((channel.adsrRate.sustainRate ^ 0x7F) - 0x18);
				}
			}
		}
		else
		{
			//Decrement
			if(channel.adsrRate.sustainMode == 0)
			{
				//Linear
				currentAdsrLevel -= GetAdsrDelta((channel.adsrRate.sustainRate ^ 0x7F) - 0x0F);
			}
			else
			{
				unsigned int sustainType = (static_cast<uint32>(currentAdsrLevel) >> 28) & 0x7;
				currentAdsrLevel -= GetAdsrDelta((channel.adsrRate.sustainRate ^ 0x7F) - 0x1B + logIndex[sustainType]);
			}
		}
	}
	else if(channel.status == RELEASE)
	{
		if(channel.adsrRate.releaseMode == 0)
		{
			//Linear
			currentAdsrLevel -= GetAdsrDelta((4 * (channel.adsrRate.releaseRate ^ 0x1F)) - 0x0C);
		}
		else
		{
			unsigned int releaseType = (static_cast<uint32>(currentAdsrLevel) >> 28) & 0x7;
			currentAdsrLevel -= GetAdsrDelta((4 * (channel.adsrRate.releaseRate ^ 0x1F)) - 0x18 + logIndex[releaseType]);
		}

		if(currentAdsrLevel <= 0)
		{
			channel.status = STOPPED;
		}
	}
	currentAdsrLevel = min<int64>(currentAdsrLevel, MAX_ADSR_VOLUME);
	currentAdsrLevel = max<int64>(currentAdsrLevel, 0);
	channel.adsrVolume = static_cast<uint32>(currentAdsrLevel);
}

///////////////////////////////////////////////////////
// CSampleReader
///////////////////////////////////////////////////////

CSpu::CSampleReader::CSampleReader() :
m_nextSample(NULL)
{
	m_done = false;
	m_nextValid = false;
}

CSpu::CSampleReader::~CSampleReader()
{

}

void CSpu::CSampleReader::SetParams(uint8* address, uint8* repeat)
{
	m_currentTime = 0;
	m_dstTime = 0;
	m_nextSample = address;
	m_repeat = repeat;
	m_s1 = 0;
	m_s2 = 0;
//	m_pitch = pitch;
//	m_pitch = 0;
	m_nextValid = false;
	m_done = false;
	AdvanceBuffer();
}

void CSpu::CSampleReader::SetPitch(uint16 pitch)
{
//	pitch = 0x2000;
//	m_sourceSamplingRate = 44100 * (pitch & 0x3FFF) / 0x4000;
	m_sourceSamplingRate = SAMPLE_RATE * pitch / 4096;
}

void CSpu::CSampleReader::GetSamples(int16* samples, unsigned int sampleCount, unsigned int destSamplingRate)
{
	assert(m_nextSample != NULL);
	double dstTimeDelta = 1.0 / static_cast<double>(destSamplingRate);
	for(unsigned int i = 0; i < sampleCount; i++)
	{
		samples[i] = GetSample(m_dstTime);
		m_dstTime += dstTimeDelta;
	}
}

int16 CSpu::CSampleReader::GetSample(double time)
{
	time -= m_currentTime;
	double sample = time * static_cast<double>(GetSamplingRate());
	double sampleInt = 0;
	double alpha = modf(sample, &sampleInt);
	unsigned int sampleIndex = static_cast<int>(sampleInt);
	int16 currentSample = m_buffer[sampleIndex];
	int16 nextSample = m_buffer[sampleIndex + 1];
	double resultSample = 
		(static_cast<double>(currentSample) * (1 - alpha)) + (static_cast<double>(nextSample) * alpha);
	if(sampleIndex >= BUFFER_SAMPLES)
	{
		AdvanceBuffer();
	}
	return static_cast<int16>(resultSample);
}

void CSpu::CSampleReader::AdvanceBuffer()
{
	if(m_nextValid)
	{
		memmove(m_buffer, m_buffer + BUFFER_SAMPLES, sizeof(int16) * BUFFER_SAMPLES);
		UnpackSamples(m_buffer + BUFFER_SAMPLES);
		m_currentTime += GetBufferStep();
	}
	else
	{
		assert(m_currentTime == 0);
		UnpackSamples(m_buffer);
		UnpackSamples(m_buffer + BUFFER_SAMPLES);
		m_nextValid = true;
	}
}

void CSpu::CSampleReader::UnpackSamples(int16* dst)
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
			dst[i] = static_cast<int16>(workBuffer[i] + 0.5);
		}
	}

	if(flags & 0x04)
	{
		m_repeat = m_nextSample;
	}

	m_nextSample += 0x10;

	if(flags & 0x01)
	{
		if(flags == 0x03)
		{
			m_nextSample = m_repeat;
		}
		else
		{
			m_done = true;
		}
	}
}

uint8* CSpu::CSampleReader::GetRepeat() const
{
	return m_repeat;
}

uint8* CSpu::CSampleReader::GetCurrent() const
{
    return m_nextSample;
}

double CSpu::CSampleReader::GetSamplingRate() const
{
	return m_sourceSamplingRate;
}

double CSpu::CSampleReader::GetBufferStep() const
{
	return static_cast<double>(BUFFER_SAMPLES) / static_cast<double>(GetSamplingRate());
}

double CSpu::CSampleReader::GetNextTime() const
{
	return m_currentTime + GetBufferStep();
}
