#include <cassert>
#include <cmath>
#include "string_format.h"
#include "../Log.h"
#include "../RegisterStateFile.h"
#include "Iop_SpuBase.h"

using namespace Iop;

#define INIT_SAMPLE_RATE (44100)
#define TIME_SCALE (0x1000)
#define LOG_NAME ("iop_spubase")

#define STATE_PATH_FORMAT					("iop_spu/spu_%d.xml")
#define STATE_REGS_CTRL						("CTRL")
#define STATE_REGS_IRQADDR					("IRQADDR")
#define STATE_REGS_TRANSFERADDR				("TRANSFERADDR")
#define STATE_REGS_TRANSFERMODE				("TRANSFERMODE")
#define STATE_REGS_REVERBWORKADDRSTART		("REVERBWORKADDRSTART")
#define STATE_REGS_REVERBWORKADDREND		("REVERBWORKADDREND")
#define STATE_REGS_REVERBCURRADDR			("REVERBCURRADDR")
#define STATE_CHANNEL_REGS_PREFIX			("CHANNEL%d_")
#define STATE_CHANNEL_REGS_VOLUMELEFT		("VOLUMELEFT")
#define STATE_CHANNEL_REGS_VOLUMERIGHT		("VOLUMERIGHT")
#define STATE_CHANNEL_REGS_VOLUMELEFTABS	("VOLUMELEFTABS")
#define STATE_CHANNEL_REGS_VOLUMERIGHTABS	("VOLUMERIGHTABS")
#define STATE_CHANNEL_REGS_STATUS			("STATUS")
#define STATE_CHANNEL_REGS_PITCH			("PITCH")
#define STATE_CHANNEL_REGS_ADSRLEVEL		("ADSRLEVEL")
#define STATE_CHANNEL_REGS_ADSRRATE			("ADSRRATE")
#define STATE_CHANNEL_REGS_ADSRVOLUME		("ADSRVOLUME")
#define STATE_CHANNEL_REGS_ADDRESS			("ADDRESS")
#define STATE_CHANNEL_REGS_REPEAT			("REPEAT")
#define STATE_CHANNEL_REGS_CURRENT			("CURRENT")

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

const uint32 CSpuBase::g_linearIncreaseSweepDeltas[0x80] = 
{
	0x3A0CC55E, 0x305FF9CE, 0x2976D61E, 0x203FFBDE, 0x1D0662AF, 0x182FFCE7, 0x1359971F, 0x101FFDEF,
	0x0DD2475F, 0x0C17FE73, 0x0A0233AF, 0x080FFEF7, 0x07144A05, 0x060BFF39, 0x050119D7, 0x03F9DC6C,
	0x037F3A27, 0x02FE04E5, 0x026B32E3, 0x01EF5BE9, 0x01B514DD, 0x018712AA, 0x01430F6B, 0x0100385E,
	0x00E129C7, 0x00BE85CF, 0x00A187B5, 0x00801C2F, 0x007094E3, 0x00607F9E, 0x004FE588, 0x003DEB7D,
	0x00392824, 0x00318930, 0x00271B77, 0x00204E57, 0x001B851B, 0x0017F80F, 0x00141506, 0x0010272B,
	0x000E0504, 0x000BFC07, 0x000A0A83, 0x0007FD5A, 0x0006C140, 0x00063126, 0x0004F41E, 0x0003E925,
	0x000389CC, 0x0002F8DF, 0x00027A0F, 0x0002021A, 0x0001C4E6, 0x00017C6F, 0x00014267, 0x0001010D,
	0x0000DFC9, 0x0000C023, 0x00009E83, 0x00007ECF, 0x00006FE4, 0x00005F1B, 0x00004F41, 0x00003F67,
	0x000037F2, 0x00002F8D, 0x000027A0, 0x0000203D, 0x00001BF9, 0x00001814, 0x00001405, 0x00000FD9,
	0x00000D96, 0x00000BE3, 0x00000A02, 0x000007EC, 0x0000070B, 0x000005F1, 0x00000501, 0x000003F6,
	0x00000385, 0x00000304, 0x00000280, 0x00000200, 0x000001BE, 0x0000017F, 0x00000140, 0x00000100,
	0x000000DF, 0x000000BF, 0x000000A0, 0x00000080, 0x0000006F, 0x0000005F, 0x00000050, 0x00000040,
	0x00000037, 0x0000002F, 0x00000028, 0x00000020, 0x0000001B, 0x00000017, 0x00000014, 0x00000010,
	0x0000000D, 0x0000000B, 0x0000000A, 0x00000008, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000
};

const uint32 CSpuBase::g_linearDecreaseSweepDeltas[0x80] = 
{
	0x488FF6B5, 0x3A0CC55E, 0x305FF9CE, 0x2976D61E, 0x203FFBDE, 0x1D0662AF, 0x182FFCE7, 0x1359971F,
	0x101FFDEF, 0x0DD2475F, 0x0C17FE73, 0x0A0233AF, 0x080FFEF7, 0x07144A05, 0x060BFF39, 0x050119D7,
	0x03F9DC6C, 0x037F3A27, 0x02FE04E5, 0x026B32E3, 0x01EF5BE9, 0x01B514DD, 0x018712AA, 0x01430F6B,
	0x0100385E, 0x00E129C7, 0x00BE85CF, 0x00A187B5, 0x00801C2F, 0x007094E3, 0x00607F9E, 0x004FE588,
	0x003DEB7D, 0x00392824, 0x00318930, 0x00271B77, 0x00204E57, 0x001B851B, 0x0017F80F, 0x00141506,
	0x0010272B, 0x000E0504, 0x000BFC07, 0x000A0A83, 0x0007FD5A, 0x0006C140, 0x00063126, 0x0004F41E,
	0x0003E925, 0x000389CC, 0x0002F8DF, 0x00027A0F, 0x0002021A, 0x0001C4E6, 0x00017C6F, 0x00014267,
	0x0001010D, 0x0000DFC9, 0x0000C023, 0x00009E83, 0x00007ECF, 0x00006FE4, 0x00005F1B, 0x00004F41,
	0x00003F67, 0x000037F2, 0x00002F8D, 0x000027A0, 0x0000203D, 0x00001BF9, 0x00001814, 0x00001405,
	0x00000FD9, 0x00000D96, 0x00000BE3, 0x00000A02, 0x000007EC, 0x0000070B, 0x000005F1, 0x00000501,
	0x000003F6, 0x00000385, 0x00000304, 0x00000280, 0x00000200, 0x000001BE, 0x0000017F, 0x00000140,
	0x00000100, 0x000000DF, 0x000000BF, 0x000000A0, 0x00000080, 0x0000006F, 0x0000005F, 0x00000050,
	0x00000040, 0x00000037, 0x0000002F, 0x00000028, 0x00000020, 0x0000001B, 0x00000017, 0x00000014,
	0x00000010, 0x0000000D, 0x0000000B, 0x0000000A, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000
};

CSpuBase::CSpuBase(uint8* ram, uint32 ramSize, unsigned int spuNumber)
: m_ram(ram)
, m_ramSize(ramSize)
, m_spuNumber(spuNumber)
, m_reverbEnabled(true)
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
	m_ctrl = 0;

	m_volumeAdjust = 1.0f;

	m_channelOn.f = 0;
	m_channelReverb.f = 0;
	m_reverbTicks = 0;
	m_irqAddr = 0;
	m_irqPending = false;
	m_transferMode = 0;
	m_transferAddr = 0;

	m_reverbCurrAddr = 0;
	m_reverbWorkAddrStart = 0;
	m_reverbWorkAddrEnd = 0x80000;
	m_baseSamplingRate = 44100;

	memset(m_channel, 0, sizeof(m_channel));
	memset(m_reverb, 0, sizeof(m_reverb));

	for(unsigned int i = 0; i < MAX_CHANNEL; i++)
	{
		m_reader[i].Reset();
		m_reader[i].SetMemory(m_ram, m_ramSize);
	}
}

void CSpuBase::LoadState(Framework::CZipArchiveReader& archive)
{
	auto path = string_format(STATE_PATH_FORMAT, m_spuNumber);

	CRegisterStateFile registerFile(*archive.BeginReadFile(path.c_str()));
	m_ctrl = registerFile.GetRegister32(STATE_REGS_CTRL);
	m_irqAddr = registerFile.GetRegister32(STATE_REGS_IRQADDR);
	m_transferMode = registerFile.GetRegister32(STATE_REGS_TRANSFERMODE);
	m_transferAddr = registerFile.GetRegister32(STATE_REGS_TRANSFERADDR);
	m_reverbWorkAddrStart = registerFile.GetRegister32(STATE_REGS_REVERBWORKADDRSTART);
	m_reverbWorkAddrEnd = registerFile.GetRegister32(STATE_REGS_REVERBWORKADDREND);
	m_reverbCurrAddr = registerFile.GetRegister32(STATE_REGS_REVERBCURRADDR);

	for(unsigned int i = 0; i < MAX_CHANNEL; i++)
	{
		auto& channel = m_channel[i];
		auto channelPrefix = string_format(STATE_CHANNEL_REGS_PREFIX, i);
		channel.volumeLeft <<= registerFile.GetRegister32((channelPrefix + STATE_CHANNEL_REGS_VOLUMELEFT).c_str());
		channel.volumeRight <<= registerFile.GetRegister32((channelPrefix + STATE_CHANNEL_REGS_VOLUMERIGHT).c_str());
		channel.volumeLeftAbs = registerFile.GetRegister32((channelPrefix + STATE_CHANNEL_REGS_VOLUMELEFTABS).c_str());
		channel.volumeRightAbs = registerFile.GetRegister32((channelPrefix + STATE_CHANNEL_REGS_VOLUMERIGHTABS).c_str());
		channel.status = registerFile.GetRegister32((channelPrefix + STATE_CHANNEL_REGS_STATUS).c_str());
		channel.pitch = registerFile.GetRegister32((channelPrefix + STATE_CHANNEL_REGS_PITCH).c_str());
		channel.adsrLevel <<= registerFile.GetRegister32((channelPrefix + STATE_CHANNEL_REGS_ADSRLEVEL).c_str());
		channel.adsrRate <<= registerFile.GetRegister32((channelPrefix + STATE_CHANNEL_REGS_ADSRRATE).c_str());
		channel.adsrVolume = registerFile.GetRegister32((channelPrefix + STATE_CHANNEL_REGS_ADSRVOLUME).c_str());
		channel.address = registerFile.GetRegister32((channelPrefix + STATE_CHANNEL_REGS_ADDRESS).c_str());
		channel.repeat = registerFile.GetRegister32((channelPrefix + STATE_CHANNEL_REGS_REPEAT).c_str());
		channel.current = registerFile.GetRegister32((channelPrefix + STATE_CHANNEL_REGS_CURRENT).c_str());

		//Reset reader (not actually ok, but better than nothing)
		auto& reader = m_reader[i];
		reader.Reset();
		if((channel.status != STOPPED) && (channel.status != KEY_ON))
		{
			reader.SetParams(channel.address, channel.repeat);
		}
	}
}

void CSpuBase::SaveState(Framework::CZipArchiveWriter& archive)
{
	auto path = string_format(STATE_PATH_FORMAT, m_spuNumber);

	CRegisterStateFile* registerFile = new CRegisterStateFile(path.c_str());
	registerFile->SetRegister32(STATE_REGS_CTRL, m_ctrl);
	registerFile->SetRegister32(STATE_REGS_IRQADDR, m_irqAddr);
	registerFile->SetRegister32(STATE_REGS_TRANSFERMODE, m_transferMode);
	registerFile->SetRegister32(STATE_REGS_TRANSFERADDR, m_transferAddr);
	registerFile->SetRegister32(STATE_REGS_REVERBWORKADDRSTART, m_reverbWorkAddrStart);
	registerFile->SetRegister32(STATE_REGS_REVERBWORKADDREND, m_reverbWorkAddrEnd);
	registerFile->SetRegister32(STATE_REGS_REVERBCURRADDR, m_reverbCurrAddr);

	for(unsigned int i = 0; i < MAX_CHANNEL; i++)
	{
		const auto& channel = m_channel[i];
		auto channelPrefix = string_format(STATE_CHANNEL_REGS_PREFIX, i);
		registerFile->SetRegister32((channelPrefix + STATE_CHANNEL_REGS_VOLUMELEFT).c_str(), channel.volumeLeft);
		registerFile->SetRegister32((channelPrefix + STATE_CHANNEL_REGS_VOLUMERIGHT).c_str(), channel.volumeRight);
		registerFile->SetRegister32((channelPrefix + STATE_CHANNEL_REGS_VOLUMELEFTABS).c_str(), channel.volumeLeftAbs);
		registerFile->SetRegister32((channelPrefix + STATE_CHANNEL_REGS_VOLUMERIGHTABS).c_str(), channel.volumeRightAbs);
		registerFile->SetRegister32((channelPrefix + STATE_CHANNEL_REGS_STATUS).c_str(), channel.status);
		registerFile->SetRegister32((channelPrefix + STATE_CHANNEL_REGS_PITCH).c_str(), channel.pitch);
		registerFile->SetRegister32((channelPrefix + STATE_CHANNEL_REGS_ADSRLEVEL).c_str(), channel.adsrLevel);
		registerFile->SetRegister32((channelPrefix + STATE_CHANNEL_REGS_ADSRRATE).c_str(), channel.adsrRate);
		registerFile->SetRegister32((channelPrefix + STATE_CHANNEL_REGS_ADSRVOLUME).c_str(), channel.adsrVolume);
		registerFile->SetRegister32((channelPrefix + STATE_CHANNEL_REGS_ADDRESS).c_str(), channel.address);
		registerFile->SetRegister32((channelPrefix + STATE_CHANNEL_REGS_REPEAT).c_str(), channel.repeat);
		registerFile->SetRegister32((channelPrefix + STATE_CHANNEL_REGS_CURRENT).c_str(), channel.current);
	}

	archive.InsertFile(registerFile);
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

uint16 CSpuBase::GetControl() const
{
	return m_ctrl;
}

void CSpuBase::SetControl(uint16 value)
{
	m_ctrl = value;
	if((m_ctrl & CONTROL_IRQ) == 0)
	{
		m_irqPending = false;
	}
}

void CSpuBase::SetBaseSamplingRate(uint32 samplingRate)
{
	m_baseSamplingRate = samplingRate;
}

bool CSpuBase::GetIrqPending() const
{
	return m_irqPending;
}

uint32 CSpuBase::GetIrqAddress() const
{
	return m_irqAddr;
}

void CSpuBase::SetIrqAddress(uint32 value)
{
	m_irqAddr = value & (m_ramSize - 1);
}

uint16 CSpuBase::GetTransferMode() const
{
	return m_transferMode;
}

void CSpuBase::SetTransferMode(uint16 transferMode)
{
	m_transferMode = transferMode;
}

uint32 CSpuBase::GetTransferAddress() const
{
	return m_transferAddr;
}

void CSpuBase::SetTransferAddress(uint32 value)
{
	m_transferAddr = value & (m_ramSize - 1);
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
		m_transferAddr, blockSize * blockAmount);
#endif
	if(m_transferMode != TRANSFER_MODE_VOICE)
	{
		//CORE0/1 block modes should have transferAddr == 0
		blockAmount = 1;
		return blockAmount;
	}
	if((m_ctrl & CONTROL_DMA) == CONTROL_DMA_READ)
	{
		//DMA reads need to be throttled to allow FFX IopSoundDriver to properly synchronize itself
		blockAmount = std::min<uint32>(blockAmount, 0x10);
		return blockAmount;
	}
	unsigned int blocksTransfered = 0;
	for(unsigned int i = 0; i < blockAmount; i++)
	{
		uint32 copySize = std::min<uint32>(m_ramSize - m_transferAddr, blockSize);
		memcpy(m_ram + m_transferAddr, buffer, copySize);
		m_transferAddr += blockSize;
		m_transferAddr &= m_ramSize - 1;
		buffer += blockSize;
		blocksTransfered++;
	}
	return blocksTransfered;
}

void CSpuBase::WriteWord(uint16 value)
{
	assert((m_transferAddr + 1) < m_ramSize);
	*reinterpret_cast<uint16*>(&m_ram[m_transferAddr]) = value;
	m_transferAddr += 2;
}

int32 CSpuBase::ComputeChannelVolume(const CHANNEL_VOLUME& volume, int32 currentVolume)
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
		volumeLevel <<= 17;
	}
	else
	{
		assert(volume.sweep.phase == 0);
		assert(volume.sweep.slope == 0);
		if(volume.sweep.decrease)
		{
			uint32 sweepDelta = g_linearDecreaseSweepDeltas[volume.sweep.volume];
			volumeLevel = currentVolume - sweepDelta;
		}
		else
		{
			uint32 sweepDelta = g_linearIncreaseSweepDeltas[volume.sweep.volume];
			volumeLevel = currentVolume + sweepDelta;
		}
		volumeLevel = std::max<int32>(volumeLevel, 0x00000000);
		volumeLevel = std::min<int32>(volumeLevel, 0x7FFFFFFF);
	}
	return volumeLevel;
}

void CSpuBase::MixSamples(int32 inputSample, int32 volumeLevel, int16* output)
{
	inputSample = (inputSample * volumeLevel) / 0x7FFF;
	int32 resultSample = inputSample + static_cast<int32>(*output);
	resultSample = std::max<int32>(resultSample, SHRT_MIN);
	resultSample = std::min<int32>(resultSample, SHRT_MAX);
	*output = static_cast<int16>(resultSample);
}

void CSpuBase::Render(int16* samples, unsigned int sampleCount, unsigned int sampleRate)
{
	assert((sampleCount & 0x01) == 0);
	//ticks are 44100Hz ticks
	unsigned int ticks = sampleCount / 2;
	memset(samples, 0, sizeof(int16) * sampleCount);

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
				reader.SetParams(channel.address, channel.repeat);
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
				if(reader.DidChangeRepeat())
				{
					channel.repeat = reader.GetRepeat();
					reader.ClearDidChangeRepeat();
				}
				//Update repeat in case it has been changed externally (needed for FFX)
				reader.SetRepeat(channel.repeat);
			}

			reader.SetIrqAddress(m_irqAddr);

			int16 readSample = 0;
			reader.SetPitch(m_baseSamplingRate, channel.pitch);
			reader.GetSamples(&readSample, 1, sampleRate);
			channel.current = reader.GetCurrent();

			if((m_ctrl & CONTROL_IRQ) && reader.GetIrqPending())
			{
				m_irqPending = true;
			}

			reader.ClearIrqPending();

			//Mix samples
			UpdateAdsr(channel);
			int32 inputSample = static_cast<int32>(readSample);
			//Mix adsrVolume
			{
				inputSample = (inputSample * static_cast<int32>(channel.adsrVolume >> 16)) / static_cast<int32>(MAX_ADSR_VOLUME >> 16);
			}

			channel.volumeLeftAbs	= ComputeChannelVolume(channel.volumeLeft, channel.volumeLeftAbs);
			channel.volumeRightAbs	= ComputeChannelVolume(channel.volumeRight, channel.volumeRightAbs);

			int32 adjustedLeftVolume = std::min<int32>(0x7FFF, static_cast<int32>(static_cast<float>(channel.volumeLeftAbs >> 16) * m_volumeAdjust));
			int32 adjustedRightVolume = std::min<int32>(0x7FFF, static_cast<int32>(static_cast<float>(channel.volumeRightAbs >> 16) * m_volumeAdjust));
			MixSamples(inputSample, adjustedLeftVolume, samples + 0);
			MixSamples(inputSample, adjustedRightVolume, samples + 1);
			//Mix in reverb if enabled for this channel
			if(m_reverbEnabled && (m_channelReverb.f & (1 << i)))
			{
				MixSamples(inputSample, adjustedLeftVolume,	reverbSample + 0);
				MixSamples(inputSample, adjustedRightVolume, reverbSample + 1);
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
					resultSample = std::max<int32>(resultSample, SHRT_MIN);
					resultSample = std::min<int32>(resultSample, SHRT_MAX);
					*output = static_cast<int16>(resultSample);
				}

				{
					int16* output = samples + 1;
					int32 resultSample = static_cast<int32>(sampleR) + static_cast<int32>(*output);
					resultSample = std::max<int32>(resultSample, SHRT_MIN);
					resultSample = std::min<int32>(resultSample, SHRT_MAX);
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
	value = std::max<float>(value, SHRT_MIN);
	value = std::min<float>(value, SHRT_MAX);
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

CSpuBase::CSampleReader::CSampleReader()
{
	Reset();
}

CSpuBase::CSampleReader::~CSampleReader()
{

}

void CSpuBase::CSampleReader::Reset()
{
	m_nextSampleAddr = 0;
	m_repeatAddr = 0;
	m_irqAddr = 0;
	memset(m_buffer, 0, sizeof(m_buffer));
	m_pitch = 0;
	m_srcSampleIdx = 0;
	m_srcSamplingRate = 0;
	m_s1 = 0;
	m_s2 = 0;
	m_done = false;
	m_didChangeRepeat = false;
	m_nextValid = false;
	m_endFlag = false;
	m_irqPending = false;
}

void CSpuBase::CSampleReader::SetMemory(uint8* ram, uint32 ramSize)
{
	m_ram = ram;
	m_ramSize = ramSize;
	assert((ramSize & (ramSize - 1)) == 0);
}

void CSpuBase::CSampleReader::SetParams(uint32 address, uint32 repeat)
{
	m_srcSampleIdx = 0;
	m_nextSampleAddr = address;
	m_repeatAddr = repeat;
	m_s1 = 0;
	m_s2 = 0;
	m_nextValid = false;
	m_done = false;
	m_didChangeRepeat = false;
	AdvanceBuffer();
}

void CSpuBase::CSampleReader::SetPitch(uint32 baseSamplingRate, uint16 pitch)
{
	m_srcSamplingRate = baseSamplingRate * pitch / 4096;
}

void CSpuBase::CSampleReader::GetSamples(int16* samples, unsigned int sampleCount, unsigned int dstSamplingRate)
{
	for(unsigned int i = 0; i < sampleCount; i++)
	{
		samples[i] = GetSample(dstSamplingRate);
	}
}

int16 CSpuBase::CSampleReader::GetSample(unsigned int dstSamplingRate)
{
	uint32 srcSampleIdx = m_srcSampleIdx / TIME_SCALE;
	int32 srcSampleAlpha = m_srcSampleIdx % TIME_SCALE;
	int32 currentSample = m_buffer[srcSampleIdx];
	int32 nextSample = m_buffer[srcSampleIdx + 1];
	int32 resultSample = (currentSample * (TIME_SCALE - srcSampleAlpha) / TIME_SCALE) +
		(nextSample * srcSampleAlpha / TIME_SCALE);
	m_srcSampleIdx += (m_srcSamplingRate * TIME_SCALE) / dstSamplingRate;
	if(srcSampleIdx >= BUFFER_SAMPLES)
	{
		m_srcSampleIdx -= BUFFER_SAMPLES * TIME_SCALE;
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
	}
	else
	{
		UnpackSamples(m_buffer);
		UnpackSamples(m_buffer + BUFFER_SAMPLES);
		m_nextValid = true;
	}
}

void CSpuBase::CSampleReader::UnpackSamples(int16* dst)
{
	if(m_done)
	{
		memset(dst, 0, sizeof(int16) * BUFFER_SAMPLES);
		return;
	}

	int32 workBuffer[BUFFER_SAMPLES];

	uint8* nextSample = m_ram + m_nextSampleAddr;

	if(m_nextSampleAddr == m_irqAddr)
	{
		m_irqPending = true;
	}

	//Read header
	uint8 shiftFactor = nextSample[0] & 0xF;
	uint8 predictNumber = nextSample[0] >> 4;
	uint8 flags = nextSample[1];
	assert(predictNumber < 5);

	//Get intermediate values
	{
		unsigned int workBufferPtr = 0;
		for(unsigned int i = 2; i < 16; i++)
		{
			uint8 sampleByte = nextSample[i];
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
		static const int32 predictorTable[5][2] =
		{
			{	0,		0		},
			{   60,		0		},
			{  115,		-52		},
			{	98,		-55		},
			{  122,		-60		},
		};

		for(unsigned int i = 0; i < BUFFER_SAMPLES; i++)
		{
			int32 currentValue = workBuffer[i] * 64;
			currentValue += (m_s1 * predictorTable[predictNumber][0]) / 64;
			currentValue += (m_s2 * predictorTable[predictNumber][1]) / 64;
			m_s2 = m_s1;
			m_s1 = currentValue;
			int32 result = (currentValue + 32) / 64;
			result = std::max<int32>(result, SHRT_MIN);
			result = std::min<int32>(result, SHRT_MAX);
			dst[i] = static_cast<int16>(result);
		}
	}

	if(flags & 0x04)
	{
		m_repeatAddr = m_nextSampleAddr;
		m_didChangeRepeat = true;
	}

	m_nextSampleAddr += 0x10;
	assert(m_nextSampleAddr < m_ramSize);
	m_nextSampleAddr &= (m_ramSize - 1);

	if(flags & 0x01)
	{
		m_endFlag = true;

		if(flags == 0x03)
		{
			m_nextSampleAddr = m_repeatAddr;
		}
		else
		{
			m_done = true;
		}
	}
}

uint32 CSpuBase::CSampleReader::GetRepeat() const
{
	return m_repeatAddr;
}

void CSpuBase::CSampleReader::SetRepeat(uint32 repeatAddr)
{
	m_repeatAddr = repeatAddr;
}

uint32 CSpuBase::CSampleReader::GetCurrent() const
{
	return m_nextSampleAddr;
}

void CSpuBase::CSampleReader::SetIrqAddress(uint32 irqAddr)
{
	m_irqAddr = irqAddr;
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

bool CSpuBase::CSampleReader::GetIrqPending() const
{
	return m_irqPending;
}

void CSpuBase::CSampleReader::ClearIrqPending()
{
	m_irqPending = false;
}

bool CSpuBase::CSampleReader::DidChangeRepeat() const
{
	return m_didChangeRepeat;
}

void CSpuBase::CSampleReader::ClearDidChangeRepeat()
{
	m_didChangeRepeat = false;
}
