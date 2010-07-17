#include <cassert>
#include <cmath>
#include "../Log.h"
#include "Iop_SpuBase.h"

using namespace Iop;
using namespace std;

#define INIT_SAMPLE_RATE (44100)
#define LOG_NAME ("iop_spubase")

bool CSpuBase::g_reverbParamIsAddress[REVERB_PARAM_COUNT] =
{
	true,
	true,
	false,
	false,
	false,
	false,
	false,
	false,
	false,
	false,
	true,
	true,
	true,
	true,
	true,
	true,
	true,
	true,
	true,
	true,
	true,
	true,
	true,
	true,
	true,
	true,
	true,
	true,
	true,
	true,
	false,
	false
};

CSpuBase::CSpuBase(uint8* ram, uint32 ramSize) :
m_ram(ram),
m_ramSize(ramSize),
m_reverbEnabled(true),
m_dmaDisabled(false)
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

CSpuBase::~CSpuBase()
{

}

void CSpuBase::Reset()
{
    m_dmaDisabled = false;
    m_ctrl = 0;

	m_volumeAdjust = 1.0f;

    m_channelOn.f = 0;
	m_channelReverb.f = 0;
	m_reverbTicks = 0;
	m_bufferAddr = 0;

	m_reverbCurrAddr = 0;
	m_reverbWorkAddrStart = 0;
//    m_reverbWorkAddrEnd = 0x7FFFF;
    m_reverbWorkAddrEnd = 0x80000;
	m_baseSamplingRate = 44100;

	memset(m_channel, 0, sizeof(m_channel));
	memset(m_reverb, 0, sizeof(m_reverb));

    for(unsigned int i = 0; i < MAX_CHANNEL; i++)
    {
        m_reader[i].Reset();
    }
}

bool CSpuBase::IsEnabled() const
{
	return (m_ctrl & 0x8000) != 0;
}

void CSpuBase::SetVolumeAdjust(float volumeAdjust)
{
	m_volumeAdjust = volumeAdjust;
}

void CSpuBase::SetReverbEnabled(bool enabled)
{
	m_reverbEnabled = enabled;
}

void CSpuBase::SetDmaDisabled(bool disabled)
{
    m_dmaDisabled = disabled;
}

uint16 CSpuBase::GetControl() const
{
    return m_ctrl;
}

void CSpuBase::SetControl(uint16 value)
{
    m_ctrl = value;
}

void CSpuBase::SetBaseSamplingRate(uint32 samplingRate)
{
	m_baseSamplingRate = samplingRate;
}

uint32 CSpuBase::GetTransferAddress() const
{
	return m_bufferAddr;
}

void CSpuBase::SetTransferAddress(uint32 value)
{
	m_bufferAddr = value & (m_ramSize - 1);
}

UNION32_16 CSpuBase::GetChannelOn() const
{
	return m_channelOn;
}

void CSpuBase::SetChannelOnLo(uint16 value)
{
    m_channelOn.h0 = value;
}

void CSpuBase::SetChannelOnHi(uint16 value)
{
    m_channelOn.h1 = value;
}

UNION32_16 CSpuBase::GetChannelReverb() const
{
	return m_channelReverb;
}

void CSpuBase::SetChannelReverbLo(uint16 value)
{
    m_channelReverb.h0 = value;
}

void CSpuBase::SetChannelReverbHi(uint16 value)
{
    m_channelReverb.h1 = value;
}

uint32 CSpuBase::GetReverbParam(unsigned int param) const
{
    assert(param < REVERB_PARAM_COUNT);
	return m_reverb[param];
}

void CSpuBase::SetReverbParam(unsigned int param, uint32 value)
{
    assert(param < REVERB_PARAM_COUNT);
    m_reverb[param] = value;
}

UNION32_16 CSpuBase::GetEndFlags() const
{
	UNION32_16 result(0);
	for(unsigned int i = 0; i < MAX_CHANNEL; i++)
	{
		if(m_reader[i].GetEndFlag())
		{
			result.f |= (1 << i);
		}
	}
	return result;
}

void CSpuBase::ClearEndFlags()
{
	for(unsigned int i = 0; i < MAX_CHANNEL; i++)
	{
		m_reader[i].ClearEndFlag();
	}
}

CSpuBase::CHANNEL& CSpuBase::GetChannel(unsigned int channelNumber)
{
    assert(channelNumber < MAX_CHANNEL);
	return m_channel[channelNumber];
}

void CSpuBase::SendKeyOn(uint32 channels)
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

void CSpuBase::SendKeyOff(uint32 channels)
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

uint32 CSpuBase::GetReverbWorkAddressStart() const
{
	return m_reverbWorkAddrStart;
}

void CSpuBase::SetReverbWorkAddressStart(uint32 address)
{
	assert(address <= m_ramSize);
	m_reverbWorkAddrStart = address;
	m_reverbCurrAddr = address;
}

uint32 CSpuBase::GetReverbWorkAddressEnd() const
{
	return m_reverbWorkAddrEnd - 1;
}

void CSpuBase::SetReverbWorkAddressEnd(uint32 address)
{
	assert((address & 0xFFFF) == 0xFFFF);
	assert(address <= m_ramSize);
	m_reverbWorkAddrEnd = address + 1;
}

void CSpuBase::SetReverbCurrentAddress(uint32 address)
{
	m_reverbCurrAddr = address;
}

uint32 CSpuBase::ReceiveDma(uint8* buffer, uint32 blockSize, uint32 blockAmount)
{
#ifdef _DEBUG
	CLog::GetInstance().Print(LOG_NAME, "Receiving DMA transfer to 0x%0.8X. Size = 0x%0.8X bytes.\r\n", 
		m_bufferAddr, blockSize * blockAmount);
#endif
    if(m_dmaDisabled) return 0;
	unsigned int blocksTransfered = 0;
	for(unsigned int i = 0; i < blockAmount; i++)
	{
		uint32 copySize = min<uint32>(m_ramSize - m_bufferAddr, blockSize);
		memcpy(m_ram + m_bufferAddr, buffer, copySize);
		m_bufferAddr += blockSize;
		m_bufferAddr &= m_ramSize - 1;
		buffer += blockSize;
		blocksTransfered++;
	}
	return blocksTransfered;
}

void CSpuBase::WriteWord(uint16 value)
{
	assert((m_bufferAddr + 1) < m_ramSize);
	*reinterpret_cast<uint16*>(&m_ram[m_bufferAddr]) = value;
	m_bufferAddr += 2;
}

int32 CSpuBase::ComputeChannelVolume(const CHANNEL_VOLUME& volume)
{
	int32 volumeLevel = 0;
	if(!volume.mode.mode)
	{
		if(volume.volume.phase)
		{
			volumeLevel = 0x3FFF - volume.volume.volume;
		}
		else
		{
			volumeLevel = volume.volume.volume;
		}
	}
	else
	{
		assert(0);
	}
	volumeLevel = min<int32>(0x3FFF, static_cast<int32>(static_cast<float>(volumeLevel) * m_volumeAdjust));
	return volumeLevel;
}

void CSpuBase::MixSamples(int32 inputSample, int32 volumeLevel, int16* output)
{
	inputSample = (inputSample * volumeLevel) / 0x3FFF;
	int32 resultSample = inputSample + static_cast<int32>(*output);
	resultSample = max<int32>(resultSample, SHRT_MIN);
	resultSample = min<int32>(resultSample, SHRT_MAX);
	*output = static_cast<int16>(resultSample);
}

void CSpuBase::Render(int16* samples, unsigned int sampleCount, unsigned int sampleRate)
{
	assert((sampleCount & 0x01) == 0);
	//ticks are 44100Hz ticks
	unsigned int ticks = sampleCount / 2;
	memset(samples, 0, sizeof(int16) * sampleCount);
//	int16* bufferTemp = reinterpret_cast<int16*>(_alloca(sizeof(int16) * ticks));

	//Precompute volume values
	for(unsigned int i = 0; i < 24; i++)
	{
		CHANNEL& channel(m_channel[i]);
		channel.volumeLeftAbs	= ComputeChannelVolume(channel.volumeLeft);
		channel.volumeRightAbs	= ComputeChannelVolume(channel.volumeRight);
	}

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
				reader.SetParams(m_ram + channel.address, m_ram + channel.repeat);
				reader.ClearEndFlag();
				channel.status = ATTACK;
				channel.adsrVolume = 0;
			}
			else
			{
				if(reader.IsDone())
				{
					channel.status = STOPPED;
					channel.adsrVolume = 0;
					continue;
				}
				uint8* repeat = reader.GetRepeat();
				channel.repeat = static_cast<uint32>(repeat - m_ram);
			}
			int16 readSample = 0;
			reader.SetPitch(m_baseSamplingRate, channel.pitch);
			reader.GetSamples(&readSample, 1, sampleRate);
            channel.current = static_cast<uint32>(reader.GetCurrent() - m_ram);
			//Mix samples
			UpdateAdsr(channel);
			int32 inputSample = static_cast<int32>(readSample);
			//Mix adsrVolume
			{
				inputSample = (inputSample * static_cast<int32>(channel.adsrVolume >> 16)) / static_cast<int32>(MAX_ADSR_VOLUME >> 16);
			}
			MixSamples(inputSample, channel.volumeLeftAbs,	samples + 0);
			MixSamples(inputSample, channel.volumeRightAbs, samples + 1);
			//Mix in reverb if enabled for this channel
			if(m_channelReverb.f & (1 << i))
			{
				MixSamples(inputSample, channel.volumeLeftAbs,	reverbSample + 0);
				MixSamples(inputSample, channel.volumeRightAbs, reverbSample + 1);
			}
		}
		//Update reverb
		if(m_reverbEnabled)
		{
			//Feed samples to FIR filter
			if(m_reverbTicks & 1)
			{
				if(m_ctrl & CONTROL_REVERB)
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
				if(m_reverbCurrAddr >= m_reverbWorkAddrEnd)
				{
					m_reverbCurrAddr = m_reverbWorkAddrStart;
				}
			}

			if(m_reverbWorkAddrStart != 0)
			{
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
			}

			m_reverbTicks++;
		}
		samples += 2;
	}
}

uint32 CSpuBase::GetAdsrDelta(unsigned int index) const
{
	return m_adsrLogTable[index + 32];
}

float CSpuBase::GetReverbSample(uint32 address) const
{
	uint32 absoluteAddress = m_reverbCurrAddr + address;
	while(absoluteAddress >= m_reverbWorkAddrEnd)
	{
		absoluteAddress -= m_reverbWorkAddrEnd;
		absoluteAddress += m_reverbWorkAddrStart;
	}
	return static_cast<float>(*reinterpret_cast<int16*>(m_ram + absoluteAddress));
}

void CSpuBase::SetReverbSample(uint32 address, float value)
{
	uint32 absoluteAddress = m_reverbCurrAddr + address;
	while(absoluteAddress >= m_reverbWorkAddrEnd)
	{
		absoluteAddress -= m_reverbWorkAddrEnd;
		absoluteAddress += m_reverbWorkAddrStart;
	}
	value = max<float>(value, SHRT_MIN);
	value = min<float>(value, SHRT_MAX);
	int16 intValue = static_cast<int16>(value);
	*reinterpret_cast<int16*>(m_ram + absoluteAddress) = intValue;
}

uint32 CSpuBase::GetReverbOffset(unsigned int registerId) const
{
	return m_reverb[registerId];
}

float CSpuBase::GetReverbCoef(unsigned int registerId) const
{
	int16 value = static_cast<int16>(m_reverb[registerId]);
	return static_cast<float>(value) / static_cast<float>(0x8000);
}

void CSpuBase::UpdateAdsr(CHANNEL& channel)
{
	static unsigned int logIndex[8] = { 0, 4, 6, 8, 9, 10, 11, 12 };
	int32 currentAdsrLevel = channel.adsrVolume;
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
		if(currentAdsrLevel < 0)
		{
			currentAdsrLevel = MAX_ADSR_VOLUME;
			channel.status = DECAY;
		}
	}
	else if(channel.status == DECAY)
	{
		unsigned int decayType = (static_cast<uint32>(currentAdsrLevel) >> 28) & 0x7;
		currentAdsrLevel -= GetAdsrDelta((4 * (channel.adsrLevel.decayRate ^ 0x1F)) - 0x18 + logIndex[decayType]);
		//Terminasion condition
		if(static_cast<unsigned int>((currentAdsrLevel >> 27) & 0xF) <= channel.adsrLevel.sustainLevel)
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

			if(currentAdsrLevel < 0)
			{
				currentAdsrLevel = MAX_ADSR_VOLUME;
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

			if(currentAdsrLevel < 0)
			{
				currentAdsrLevel = 0;
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

		if(currentAdsrLevel < 0)
		{
			currentAdsrLevel = 0;
			channel.status = STOPPED;
		}
	}
	channel.adsrVolume = static_cast<uint32>(currentAdsrLevel);
}

///////////////////////////////////////////////////////
// CSampleReader
///////////////////////////////////////////////////////

CSpuBase::CSampleReader::CSampleReader() :
m_nextSample(NULL)
{
	Reset();
}

CSpuBase::CSampleReader::~CSampleReader()
{

}

void CSpuBase::CSampleReader::Reset()
{
	m_sourceSamplingRate = 0;
	m_nextSample = NULL;
	m_repeat = NULL;
	memset(m_buffer, 0, sizeof(m_buffer));
	m_pitch = 0;
	m_currentTime = 0;
	m_dstTime = 0;
	m_s1 = 0;
	m_s2 = 0;
	m_done = false;
	m_nextValid = false;
	m_endFlag = false;
}

void CSpuBase::CSampleReader::SetParams(uint8* address, uint8* repeat)
{
	m_currentTime = 0;
	m_dstTime = 0;
	m_nextSample = address;
	m_repeat = repeat;
	m_s1 = 0;
	m_s2 = 0;
	m_nextValid = false;
	m_done = false;
	AdvanceBuffer();
}

void CSpuBase::CSampleReader::SetPitch(uint32 baseSamplingRate, uint16 pitch)
{
	m_sourceSamplingRate = baseSamplingRate * pitch / 4096;
}

void CSpuBase::CSampleReader::GetSamples(int16* samples, unsigned int sampleCount, unsigned int destSamplingRate)
{
	assert(m_nextSample != NULL);
	float dstTimeDelta = 1.0f / static_cast<float>(destSamplingRate);
	for(unsigned int i = 0; i < sampleCount; i++)
	{
		samples[i] = GetSample(m_dstTime);
		m_dstTime += dstTimeDelta;
	}
}

int16 CSpuBase::CSampleReader::GetSample(float time)
{
	time -= m_currentTime;
	float sample = time * static_cast<float>(GetSamplingRate());
	float alpha = sample - (int)sample;
	float sampleInt = sample - alpha;
	unsigned int sampleIndex = static_cast<int>(sampleInt);
	int16 currentSample = m_buffer[sampleIndex];
	int16 nextSample = m_buffer[sampleIndex + 1];
	float resultSample = 
		(static_cast<float>(currentSample) * (1 - alpha)) + (static_cast<float>(nextSample) * alpha);
	if(sampleIndex >= BUFFER_SAMPLES)
	{
		AdvanceBuffer();
	}
	return static_cast<int16>(resultSample);
}

void CSpuBase::CSampleReader::AdvanceBuffer()
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

void CSpuBase::CSampleReader::UnpackSamples(int16* dst)
{
	float workBuffer[28];

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
		static float predictorTable[5][2] = 
		{
			{	0.0f,			0.0f			},
			{   60.0f / 64.0f,	0.0f			},
			{  115.0f / 64.0f,	-52.0f / 64.0f	},
			{	98.0f / 64.0f,	-55.0f / 64.0f	},
			{  122.0f / 64.0f,	-60.0f / 64.0f	},
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
		m_endFlag = true;

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

uint8* CSpuBase::CSampleReader::GetRepeat() const
{
	return m_repeat;
}

uint8* CSpuBase::CSampleReader::GetCurrent() const
{
    return m_nextSample;
}

bool CSpuBase::CSampleReader::IsDone() const
{
	return m_done;
}

bool CSpuBase::CSampleReader::GetEndFlag() const
{
	return m_endFlag;
}

void CSpuBase::CSampleReader::ClearEndFlag()
{
	m_endFlag = false;
}

float CSpuBase::CSampleReader::GetSamplingRate() const
{
	return static_cast<float>(m_sourceSamplingRate);
}

float CSpuBase::CSampleReader::GetBufferStep() const
{
	return static_cast<float>(BUFFER_SAMPLES) / static_cast<float>(GetSamplingRate());
}

float CSpuBase::CSampleReader::GetNextTime() const
{
	return m_currentTime + GetBufferStep();
}
