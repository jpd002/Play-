#include <cassert>
#include <cstring>
#include <cmath>
#include <climits>
#include <algorithm>
#include "string_format.h"
#include "Log.h"
#include "../states/RegisterStateCollectionFile.h"
#include "../states/RegisterStateUtils.h"
#include "../states/RegisterStateFile.h"
#include "Iop_SpuBase.h"

using namespace Iop;

#define INIT_SAMPLE_RATE (44100)
#define PITCH_BASE (0x1000)
#define TIME_SCALE (0x1000)
#define RESET_IRQ_ADDR (~0U)
#define LOG_NAME ("iop_spubase")

#define STATE_REGS_PATH_FORMAT ("iop_spu/spu_%d.xml")

#define STATE_REGS ("GlobalRegs")
#define STATE_REGS_CTRL ("CTRL")
#define STATE_REGS_IRQADDR ("IRQADDR")
#define STATE_REGS_IRQPENDING ("IRQPENDING")
#define STATE_REGS_TRANSFERADDR ("TRANSFERADDR")
#define STATE_REGS_TRANSFERMODE ("TRANSFERMODE")
#define STATE_REGS_CORE0OUTPUTOFFSET ("CORE0OUTPUTOFFSET")
#define STATE_REGS_CHANNELON ("CHANNELON")
#define STATE_REGS_CHANNELREVERB ("CHANNELREVERB")
#define STATE_REGS_REVERBWORKADDRSTART ("REVERBWORKADDRSTART")
#define STATE_REGS_REVERBWORKADDREND ("REVERBWORKADDREND")
#define STATE_REGS_REVERBCURRADDR ("REVERBCURRADDR")
#define STATE_REGS_REVERB_FORMAT ("REVERB%d")
#define STATE_REGS_INPVOLUMELEFT ("INPVOLUMELEFT")
#define STATE_REGS_INPVOLUMERIGHT ("INPVOLUMERIGHT")
#define STATE_REGS_EXTINPVOLUMELEFT ("EXTINPVOLUMELEFT")
#define STATE_REGS_EXTINPVOLUMERIGHT ("EXTINPVOLUMERIGHT")

#define STATE_CHANNEL_REGS_FORMAT ("Channel%02dRegs")
#define STATE_CHANNEL_REGS_VOLUMELEFT ("VOLUMELEFT")
#define STATE_CHANNEL_REGS_VOLUMERIGHT ("VOLUMERIGHT")
#define STATE_CHANNEL_REGS_VOLUMELEFTABS ("VOLUMELEFTABS")
#define STATE_CHANNEL_REGS_VOLUMERIGHTABS ("VOLUMERIGHTABS")
#define STATE_CHANNEL_REGS_STATUS ("STATUS")
#define STATE_CHANNEL_REGS_PITCH ("PITCH")
#define STATE_CHANNEL_REGS_ADSRLEVEL ("ADSRLEVEL")
#define STATE_CHANNEL_REGS_ADSRRATE ("ADSRRATE")
#define STATE_CHANNEL_REGS_ADSRVOLUME ("ADSRVOLUME")
#define STATE_CHANNEL_REGS_ADDRESS ("ADDRESS")
#define STATE_CHANNEL_REGS_REPEAT ("REPEAT")
#define STATE_CHANNEL_REGS_REPEATSET ("REPEATSET")
#define STATE_CHANNEL_REGS_CURRENT ("CURRENT")

#define STATE_SAMPLEREADER_REGS_SRCSAMPLEIDX ("SR_SrcSampleIdx")
#define STATE_SAMPLEREADER_REGS_SRCSAMPLINGRATE ("SR_SrcSamplingRate")
#define STATE_SAMPLEREADER_REGS_NEXTSAMPLEADDR ("SR_NextSampleAddr")
#define STATE_SAMPLEREADER_REGS_REPEATADDR ("SR_RepeatAddr")
#define STATE_SAMPLEREADER_REGS_PITCH ("SR_Pitch")
#define STATE_SAMPLEREADER_REGS_S1 ("SR_S1")
#define STATE_SAMPLEREADER_REGS_S2 ("SR_S2")
#define STATE_SAMPLEREADER_REGS_DONE ("SR_Done")
#define STATE_SAMPLEREADER_REGS_NEXTVALID ("SR_NextValid")
#define STATE_SAMPLEREADER_REGS_ENDFLAG ("SR_EndFlag")
#define STATE_SAMPLEREADER_REGS_DIDCHANGEREPEAT ("SR_DidChangeRepeat")
#define STATE_SAMPLEREADER_REGS_BUFFER_FORMAT ("SR_Buffer%d")

#define STATE_IRQWATCHER_REGS_PATH ("iop_spu/spu_irqwatcher.xml")

#define STATE_IRQWATCHER_REGS_IRQADDR0 ("irqAddr0")
#define STATE_IRQWATCHER_REGS_IRQADDR1 ("irqAddr1")
#define STATE_IRQWATCHER_REGS_IRQPENDING0 ("irqPending0")
#define STATE_IRQWATCHER_REGS_IRQPENDING1 ("irqPending1")

// clang-format off
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
// clang-format on

CSpuBase::CSpuBase(uint8* ram, uint32 ramSize, CSpuSampleCache* sampleCache, CSpuIrqWatcher* irqWatcher, unsigned int spuNumber)
    : m_ram(ram)
    , m_ramSize(ramSize)
    , m_spuNumber(spuNumber)
    , m_sampleCache(sampleCache)
    , m_irqWatcher(irqWatcher)
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

void CSpuBase::Reset()
{
	m_ctrl = 0;

	m_volumeAdjust = 1.0f;

	m_channelOn.f = 0;
	m_channelReverb.f = 0;
	m_reverbTicks = 0;
	m_irqAddr = RESET_IRQ_ADDR;
	m_irqPending = false;
	m_transferMode = 0;
	m_transferAddr = 0;

	m_core0OutputOffset = 0;

	m_reverbCurrAddr = 0;
	m_reverbWorkAddrStart = 0;
	m_reverbWorkAddrEnd = 0x80000;
	m_baseSamplingRate = INIT_SAMPLE_RATE;

	m_inputVolL = 0x7FFF;
	m_inputVolR = 0x7FFF;
	m_extInputVolL = 0x7FFF;
	m_extInputVolR = 0x7FFF;

	memset(m_channel, 0, sizeof(m_channel));
	memset(m_reverb, 0, sizeof(m_reverb));

	for(unsigned int i = 0; i < MAX_CHANNEL; i++)
	{
		auto& reader = m_reader[i];
		reader.Reset();
		reader.SetMemory(m_ram, m_ramSize);
		reader.SetSampleCache(m_sampleCache);
		reader.SetIrqWatcher(m_irqWatcher);
	}

	m_blockReader.Reset();
	m_soundInputDataAddr = (m_spuNumber == 0) ? SOUND_INPUT_DATA_CORE0_BASE : SOUND_INPUT_DATA_CORE1_BASE;
	m_blockWritePtr = 0;
}

void CSpuBase::LoadState(Framework::CZipArchiveReader& archive)
{
	auto path = string_format(STATE_REGS_PATH_FORMAT, m_spuNumber);
	auto stateCollectionFile = CRegisterStateCollectionFile(*archive.BeginReadFile(path.c_str()));

	{
		const auto& state = stateCollectionFile.GetRegisterState(STATE_REGS);
		m_ctrl = state.GetRegister32(STATE_REGS_CTRL);
		m_irqAddr = state.GetRegister32(STATE_REGS_IRQADDR);
		m_irqPending = state.GetRegister32(STATE_REGS_IRQPENDING) != 0;
		m_transferMode = state.GetRegister32(STATE_REGS_TRANSFERMODE);
		m_transferAddr = state.GetRegister32(STATE_REGS_TRANSFERADDR);
		m_core0OutputOffset = state.GetRegister32(STATE_REGS_CORE0OUTPUTOFFSET);
		m_channelOn.f = state.GetRegister32(STATE_REGS_CHANNELON);
		m_channelReverb.f = state.GetRegister32(STATE_REGS_CHANNELREVERB);
		m_reverbWorkAddrStart = state.GetRegister32(STATE_REGS_REVERBWORKADDRSTART);
		m_reverbWorkAddrEnd = state.GetRegister32(STATE_REGS_REVERBWORKADDREND);
		m_reverbCurrAddr = state.GetRegister32(STATE_REGS_REVERBCURRADDR);
		m_inputVolL = state.GetRegister32(STATE_REGS_INPVOLUMELEFT);
		m_inputVolR = state.GetRegister32(STATE_REGS_INPVOLUMERIGHT);
		m_extInputVolL = state.GetRegister32(STATE_REGS_EXTINPVOLUMELEFT);
		m_extInputVolR = state.GetRegister32(STATE_REGS_EXTINPVOLUMERIGHT);
		RegisterStateUtils::ReadArray(state, m_reverb, STATE_REGS_REVERB_FORMAT);
	}

	for(unsigned int i = 0; i < MAX_CHANNEL; i++)
	{
		auto& channel = m_channel[i];
		auto& reader = m_reader[i];

		auto channelRegsName = string_format(STATE_CHANNEL_REGS_FORMAT, i);
		const auto& channelState = stateCollectionFile.GetRegisterState(channelRegsName.c_str());

		channel.volumeLeft <<= channelState.GetRegister32(STATE_CHANNEL_REGS_VOLUMELEFT);
		channel.volumeRight <<= channelState.GetRegister32(STATE_CHANNEL_REGS_VOLUMERIGHT);
		channel.volumeLeftAbs = channelState.GetRegister32(STATE_CHANNEL_REGS_VOLUMELEFTABS);
		channel.volumeRightAbs = channelState.GetRegister32(STATE_CHANNEL_REGS_VOLUMERIGHTABS);
		channel.status = channelState.GetRegister32(STATE_CHANNEL_REGS_STATUS);
		channel.pitch = channelState.GetRegister32(STATE_CHANNEL_REGS_PITCH);
		channel.adsrLevel <<= channelState.GetRegister32(STATE_CHANNEL_REGS_ADSRLEVEL);
		channel.adsrRate <<= channelState.GetRegister32(STATE_CHANNEL_REGS_ADSRRATE);
		channel.adsrVolume = channelState.GetRegister32(STATE_CHANNEL_REGS_ADSRVOLUME);
		channel.address = channelState.GetRegister32(STATE_CHANNEL_REGS_ADDRESS);
		channel.repeat = channelState.GetRegister32(STATE_CHANNEL_REGS_REPEAT);
		channel.repeatSet = channelState.GetRegister32(STATE_CHANNEL_REGS_REPEATSET) != 0;
		channel.current = channelState.GetRegister32(STATE_CHANNEL_REGS_CURRENT);
		reader.LoadState(channelState);
	}
}

void CSpuBase::SaveState(Framework::CZipArchiveWriter& archive)
{
	auto path = string_format(STATE_REGS_PATH_FORMAT, m_spuNumber);
	auto stateCollectionFile = std::make_unique<CRegisterStateCollectionFile>(path.c_str());

	{
		CRegisterState state;
		state.SetRegister32(STATE_REGS_CTRL, m_ctrl);
		state.SetRegister32(STATE_REGS_IRQADDR, m_irqAddr);
		state.SetRegister32(STATE_REGS_IRQPENDING, m_irqPending);
		state.SetRegister32(STATE_REGS_TRANSFERMODE, m_transferMode);
		state.SetRegister32(STATE_REGS_TRANSFERADDR, m_transferAddr);
		state.SetRegister32(STATE_REGS_CORE0OUTPUTOFFSET, m_core0OutputOffset);
		state.SetRegister32(STATE_REGS_CHANNELON, m_channelOn.f);
		state.SetRegister32(STATE_REGS_CHANNELREVERB, m_channelReverb.f);
		state.SetRegister32(STATE_REGS_REVERBWORKADDRSTART, m_reverbWorkAddrStart);
		state.SetRegister32(STATE_REGS_REVERBWORKADDREND, m_reverbWorkAddrEnd);
		state.SetRegister32(STATE_REGS_REVERBCURRADDR, m_reverbCurrAddr);
		state.SetRegister32(STATE_REGS_INPVOLUMELEFT, m_inputVolL);
		state.SetRegister32(STATE_REGS_INPVOLUMERIGHT, m_inputVolR);
		state.SetRegister32(STATE_REGS_EXTINPVOLUMELEFT, m_extInputVolL);
		state.SetRegister32(STATE_REGS_EXTINPVOLUMERIGHT, m_extInputVolR);
		RegisterStateUtils::WriteArray(state, m_reverb, STATE_REGS_REVERB_FORMAT);
		stateCollectionFile->InsertRegisterState(STATE_REGS, std::move(state));
	}

	for(unsigned int i = 0; i < MAX_CHANNEL; i++)
	{
		const auto& channel = m_channel[i];
		const auto& reader = m_reader[i];
		CRegisterState channelState;
		channelState.SetRegister32(STATE_CHANNEL_REGS_VOLUMELEFT, channel.volumeLeft);
		channelState.SetRegister32(STATE_CHANNEL_REGS_VOLUMERIGHT, channel.volumeRight);
		channelState.SetRegister32(STATE_CHANNEL_REGS_VOLUMELEFTABS, channel.volumeLeftAbs);
		channelState.SetRegister32(STATE_CHANNEL_REGS_VOLUMERIGHTABS, channel.volumeRightAbs);
		channelState.SetRegister32(STATE_CHANNEL_REGS_STATUS, channel.status);
		channelState.SetRegister32(STATE_CHANNEL_REGS_PITCH, channel.pitch);
		channelState.SetRegister32(STATE_CHANNEL_REGS_ADSRLEVEL, channel.adsrLevel);
		channelState.SetRegister32(STATE_CHANNEL_REGS_ADSRRATE, channel.adsrRate);
		channelState.SetRegister32(STATE_CHANNEL_REGS_ADSRVOLUME, channel.adsrVolume);
		channelState.SetRegister32(STATE_CHANNEL_REGS_ADDRESS, channel.address);
		channelState.SetRegister32(STATE_CHANNEL_REGS_REPEAT, channel.repeat);
		channelState.SetRegister32(STATE_CHANNEL_REGS_REPEATSET, channel.repeatSet);
		channelState.SetRegister32(STATE_CHANNEL_REGS_CURRENT, channel.current);
		reader.SaveState(channelState);

		auto channelRegsName = string_format(STATE_CHANNEL_REGS_FORMAT, i);
		stateCollectionFile->InsertRegisterState(channelRegsName.c_str(), std::move(channelState));
	}

	archive.InsertFile(std::move(stateCollectionFile));
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
		ClearIrqPending();
		m_irqWatcher->ClearIrqPending(m_spuNumber);
	}
}

void CSpuBase::SetBaseSamplingRate(uint32 samplingRate)
{
	m_baseSamplingRate = samplingRate;
	m_blockReader.SetBaseSamplingRate(samplingRate);
}

void CSpuBase::SetInputBypass(bool inputBypass)
{
	m_blockReader.SetSpdifBypass(inputBypass);
}

void CSpuBase::SetDestinationSamplingRate(uint32 samplingRate)
{
	m_blockReader.SetDestinationSamplingRate(samplingRate);
	for(auto& reader : m_reader)
	{
		reader.SetDestinationSamplingRate(samplingRate);
	}
}

bool CSpuBase::GetIrqPending() const
{
	return m_irqPending;
}

void CSpuBase::ClearIrqPending()
{
	m_irqPending = false;
}

uint32 CSpuBase::GetIrqAddress() const
{
	return m_irqAddr;
}

void CSpuBase::SetIrqAddress(uint32 value)
{
	m_irqAddr = value & (m_ramSize - 1);
	m_irqWatcher->SetIrqAddress(m_spuNumber, m_irqAddr);
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

void CSpuBase::OnChannelPitchChanged(unsigned int channelNumber)
{
	assert(channelNumber < MAX_CHANNEL);
	auto& reader = m_reader[channelNumber];
	auto& channel = m_channel[channelNumber];
	reader.SetPitch(m_baseSamplingRate, channel.pitch);
}

void CSpuBase::SendKeyOn(uint32 channels)
{
	for(unsigned int i = 0; i < MAX_CHANNEL; i++)
	{
		CHANNEL& channel = m_channel[i];
		if(channels & (1 << i))
		{
			channel.status = KEY_ON;
			channel.repeatSet = false;
			channel.current = channel.address;
			//Simulate one tick of ADSR in case game tries to read envelope value quickly
			//(ie.: Before 'Render' is called and while the voice is still in KEY_ON state)
			channel.adsrVolume = GetAdsrDelta((channel.adsrLevel.attackRate ^ 0x7F) - 0x10);
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
				//Update channel registers since we are doing KEY_ON -> STOPPED transition
				auto& reader = m_reader[i];
				reader.SetParamsNoRead(channel.address, channel.repeat);
				reader.ClearEndFlag();
				channel.current = reader.GetCurrent();
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

uint32 CSpuBase::ReceiveDma(uint8* buffer, uint32 blockSize, uint32 blockAmount, uint32 direction)
{
#ifdef _DEBUG
	CLog::GetInstance().Print(LOG_NAME, "Receiving DMA transfer to 0x%08X. Size = 0x%08X bytes.\r\n",
	                          m_transferAddr, blockSize * blockAmount);
#endif
	if(m_transferMode == TRANSFER_MODE_VOICE)
	{
		if((m_ctrl & CONTROL_DMA) == CONTROL_DMA_STOP)
		{
			//Genso Suikoden 5 uses this
			return 0;
		}
		if((m_ctrl & CONTROL_DMA) == CONTROL_DMA_READ)
		{
			//- DMA reads need to be throttled to allow FFX IopSoundDriver to properly synchronize itself
			blockAmount = std::min<uint32>(blockAmount, 0x10);
			for(unsigned int i = 0; i < blockAmount; i++)
			{
				memcpy(buffer, m_ram + m_transferAddr, blockSize);
				m_transferAddr += blockSize;
				m_transferAddr &= m_ramSize - 1;
				buffer += blockSize;
			}
			return blockAmount;
		}
		//- Tsugunai needs voice transfers to be throttled because it starts a DMA transfer
		//  and then writes data that is necessary to the transfer callback in memory
		//- Some PSF sets (FF4, Xenogears, Xenosaga 2) are sensitive to aggressive throttling (doesn't like 0x10)
		blockAmount = std::min<uint32>(blockAmount, 0x100);
		assert((m_ctrl & CONTROL_DMA) == CONTROL_DMA_WRITE);
		unsigned int blocksTransfered = 0;
		m_sampleCache->ClearRange(m_transferAddr, blockSize * blockAmount);
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
	else if(
	    (m_transferMode == TRANSFER_MODE_BLOCK_CORE0IN) ||
	    (m_transferMode == TRANSFER_MODE_BLOCK_CORE1IN))
	{
		assert(m_transferAddr == 0);
		assert((m_spuNumber == 0) || !(m_transferMode == TRANSFER_MODE_BLOCK_CORE0IN));
		assert((m_spuNumber == 1) || !(m_transferMode == TRANSFER_MODE_BLOCK_CORE1IN));
		assert(m_blockWritePtr <= SOUND_INPUT_DATA_SIZE);

		uint32 availableBytes = SOUND_INPUT_DATA_SIZE - m_blockWritePtr;
		uint32 availableBlocks = availableBytes / blockSize;
		blockAmount = std::min(blockAmount, availableBlocks);

		uint32 dstAddr = m_soundInputDataAddr + m_blockWritePtr;
		memcpy(m_ram + dstAddr, buffer, blockAmount * blockSize);
		m_blockWritePtr += blockAmount * blockSize;

		return blockAmount;
	}
	else
	{
		return 1;
	}
}

void CSpuBase::WriteWord(uint16 value)
{
	assert((m_transferAddr + 1) < m_ramSize);
	*reinterpret_cast<uint16*>(&m_ram[m_transferAddr]) = value;
	m_sampleCache->ClearRange(m_transferAddr, 2);
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
		if(volume.sweep.slope == 0)
		{
			//Linear increase/decrease
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
		}
		else
		{
			//Exponential increase/decrease
			if(volume.sweep.decrease)
			{
				int64 sweepDelta = static_cast<int64>(currentVolume) * static_cast<int64>(volume.sweep.volume) / 0x7F;
				assert(sweepDelta >= 0);
				int32 baseVolume = std::max(1, currentVolume);
				uint32 sweepDeltaClamped = std::clamp<int64>(sweepDelta, 1, baseVolume);
				volumeLevel = baseVolume - sweepDeltaClamped;
			}
			else
			{
				//Not supported
				assert(false);
			}
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
	resultSample = std::clamp<int32>(resultSample, SHRT_MIN, SHRT_MAX);

	*output = static_cast<int16>(resultSample);
}

void CSpuBase::Render(int16* samples, unsigned int sampleCount)
{
	bool updateReverb = m_reverbEnabled && (m_ctrl & CONTROL_REVERB) && (m_reverbWorkAddrStart < m_reverbWorkAddrEnd);
	bool irqEnabled = (m_ctrl & CONTROL_IRQ);

	int16* samplesBase = samples;
	assert((sampleCount & 0x01) == 0);
	unsigned int ticks = sampleCount / 2;
	memset(samples, 0, sizeof(int16) * sampleCount);

	for(unsigned int j = 0; j < ticks; j++)
	{
		int16 reverbSample[2] = {};
		//Update channels
		for(unsigned int i = 0; i < 24; i++)
		{
			auto& channel(m_channel[i]);
			auto& reader(m_reader[i]);
			if(channel.status == KEY_ON)
			{
				reader.SetParamsRead(channel.address, channel.repeat);
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
					reader.ClearIsDone();
				}
				if(reader.DidChangeRepeat() && !channel.repeatSet)
				{
					channel.repeat = reader.GetRepeat();
					reader.ClearDidChangeRepeat();
				}
				//Update repeat in case it has been changed externally (needed for FFX)
				reader.SetRepeat(channel.repeat);
			}

			int32 readSample = reader.GetSample();
			channel.current = reader.GetCurrent();

			UpdateAdsr(channel);
			channel.volumeLeftAbs = ComputeChannelVolume(channel.volumeLeft, channel.volumeLeftAbs);
			channel.volumeRightAbs = ComputeChannelVolume(channel.volumeRight, channel.volumeRightAbs);

			if(readSample == 0) continue;

			//Mix in adsrVolume
			int32 inputSample = (readSample * static_cast<int32>(channel.adsrVolume >> 16)) / static_cast<int32>(MAX_ADSR_VOLUME >> 16);

			if(inputSample == 0) continue;

			int32 volumeLeft = channel.volumeLeftAbs >> 16;
			int32 volumeRight = channel.volumeRightAbs >> 16;

			MixSamples(inputSample, volumeLeft, samples + 0);
			MixSamples(inputSample, volumeRight, samples + 1);

			//Mix in reverb if enabled for this channel
			if(updateReverb && (m_channelReverb.f & (1 << i)))
			{
				MixSamples(inputSample, volumeLeft, reverbSample + 0);
				MixSamples(inputSample, volumeRight, reverbSample + 1);
			}
		}

		if(!m_blockReader.CanReadSamples() && (m_blockWritePtr == SOUND_INPUT_DATA_SIZE))
		{
			//We're ready to consume some data
			m_blockReader.FillBlock(m_ram + m_soundInputDataAddr);
			m_blockWritePtr = 0;
		}

		if(m_blockReader.CanReadSamples())
		{
			int32 blockSamples[2] = {};
			m_blockReader.GetSamples(blockSamples);

			// Audio input data should have volume adjusted to BVOL register values . . .
			if(m_spuNumber == 0 && m_blockReader.GetSpdifBypass())
			{
				//  . . . unless in bypass mode
				MixSamples(blockSamples[0], 0x7FFF, samples + 0);
				MixSamples(blockSamples[1], 0x7FFF, samples + 1);
			}
			else
			{
				MixSamples(blockSamples[0], m_inputVolL, samples + 0);
				MixSamples(blockSamples[1], m_inputVolR, samples + 1);
			}
		}

		//Simulate SPU CORE0 writing its output in RAM and check for potential interrupts
		if(m_spuNumber == 0)
		{
			if(irqEnabled)
			{
				//TODO: Check which core is responsible for which area
				if(m_irqAddr == (CORE0_SIN_LEFT + m_core0OutputOffset))
				{
					m_irqPending = true;
				}
				else if(m_irqAddr == (CORE1_SIN_LEFT + m_core0OutputOffset))
				{
					m_irqPending = true;
				}
				else if(m_irqAddr == (CORE1_SIN_RIGHT + m_core0OutputOffset))
				{
					m_irqPending = true;
				}
			}
			m_core0OutputOffset += 2;
			m_core0OutputOffset &= (CORE0_OUTPUT_SIZE - 1);
		}

		//Update reverb
		if(updateReverb)
		{
			UpdateReverb(reverbSample, samples);
		}

		samples += 2;
	}

	if(irqEnabled && m_irqWatcher->HasPendingIrq(m_spuNumber))
	{
		m_irqPending = true;
	}
	m_irqWatcher->ClearIrqPending(m_spuNumber);

	if(m_volumeAdjust != 1.0f)
	{
		for(int i = 0; i < sampleCount; i++)
		{
			float adjustedSample = static_cast<float>(samplesBase[i]) * m_volumeAdjust;
			adjustedSample = std::clamp<float>(adjustedSample, SHRT_MIN, SHRT_MAX);
			samplesBase[i] = static_cast<int16>(adjustedSample);
		}
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
	static const unsigned int logIndex[8] = {0, 4, 6, 8, 9, 10, 11, 12};
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

void CSpuBase::UpdateReverb(int16 reverbSample[2], int16* samples)
{
	//Feed samples to FIR filter
	if(m_reverbTicks & 1)
	{
		//IIR_INPUT_A0 = buffer[IIR_SRC_A0] * IIR_COEF + INPUT_SAMPLE_L * IN_COEF_L;
		//IIR_INPUT_A1 = buffer[IIR_SRC_A1] * IIR_COEF + INPUT_SAMPLE_R * IN_COEF_R;
		//IIR_INPUT_B0 = buffer[IIR_SRC_B1] * IIR_COEF + INPUT_SAMPLE_L * IN_COEF_L;
		//IIR_INPUT_B1 = buffer[IIR_SRC_B0] * IIR_COEF + INPUT_SAMPLE_R * IN_COEF_R;

		float input_sample_l = static_cast<float>(reverbSample[0]) * 0.5f;
		float input_sample_r = static_cast<float>(reverbSample[1]) * 0.5f;

		float irr_coef = GetReverbCoef(IIR_COEF);
		float in_coef_l = GetReverbCoef(IN_COEF_L);
		float in_coef_r = GetReverbCoef(IN_COEF_R);

		float iir_input_a0 = GetReverbSample(GetReverbOffset(IIR_SRC_A0)) * irr_coef + input_sample_l * in_coef_l;
		float iir_input_a1 = GetReverbSample(GetReverbOffset(IIR_SRC_A1)) * irr_coef + input_sample_r * in_coef_r;
		float iir_input_b0 = GetReverbSample(GetReverbOffset(IIR_SRC_B1)) * irr_coef + input_sample_l * in_coef_l;
		float iir_input_b1 = GetReverbSample(GetReverbOffset(IIR_SRC_B0)) * irr_coef + input_sample_r * in_coef_r;

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

///////////////////////////////////////////////////////
// CSpuSampleCache
///////////////////////////////////////////////////////

const CSpuSampleCache::ITEM* CSpuSampleCache::GetItem(const KEY& key) const
{
	auto range = m_cache.equal_range(key.address);
	for(auto itemIterator = range.first; itemIterator != range.second; itemIterator++)
	{
		const auto& item = itemIterator->second;
		if((item.inS1 == key.s1) && (item.inS2 == key.s2))
		{
			return &item;
		}
	}
	return nullptr;
}

CSpuSampleCache::ITEM& CSpuSampleCache::RegisterItem(const KEY& key)
{
	auto result = m_cache.emplace(std::make_pair(key.address, ITEM()));
	auto& item = result->second;
	item.inS1 = key.s1;
	item.inS2 = key.s2;
	return item;
}

void CSpuSampleCache::Clear()
{
	m_cache.clear();
}

void CSpuSampleCache::ClearRange(uint32 address, uint32 size)
{
	auto lowerBound = m_cache.lower_bound(address);
	auto upperBound = m_cache.upper_bound(address + size);
	m_cache.erase(lowerBound, upperBound);
}

///////////////////////////////////////////////////////
// CSpuIrqWatcher
///////////////////////////////////////////////////////

void CSpuIrqWatcher::Reset()
{
	for(int i = 0; i < MAX_CORES; i++)
	{
		m_irqPending[i] = false;
		m_irqAddr[i] = RESET_IRQ_ADDR;
	}
}

void CSpuIrqWatcher::LoadState(Framework::CZipArchiveReader& archive)
{
	auto registerFile = CRegisterStateFile(*archive.BeginReadFile(STATE_IRQWATCHER_REGS_PATH));
	m_irqAddr[0] = registerFile.GetRegister32(STATE_IRQWATCHER_REGS_IRQADDR0);
	m_irqAddr[1] = registerFile.GetRegister32(STATE_IRQWATCHER_REGS_IRQADDR1);
	m_irqPending[0] = registerFile.GetRegister32(STATE_IRQWATCHER_REGS_IRQPENDING0) != 0;
	m_irqPending[1] = registerFile.GetRegister32(STATE_IRQWATCHER_REGS_IRQPENDING1) != 0;
}

void CSpuIrqWatcher::SaveState(Framework::CZipArchiveWriter& archive)
{
	auto registerFile = std::make_unique<CRegisterStateFile>(STATE_IRQWATCHER_REGS_PATH);
	registerFile->SetRegister32(STATE_IRQWATCHER_REGS_IRQADDR0, m_irqAddr[0]);
	registerFile->SetRegister32(STATE_IRQWATCHER_REGS_IRQADDR1, m_irqAddr[1]);
	registerFile->SetRegister32(STATE_IRQWATCHER_REGS_IRQPENDING0, m_irqPending[0]);
	registerFile->SetRegister32(STATE_IRQWATCHER_REGS_IRQPENDING1, m_irqPending[1]);
	archive.InsertFile(std::move(registerFile));
}

void CSpuIrqWatcher::SetIrqAddress(int core, uint32 address)
{
	m_irqAddr[core] = address;
}

void CSpuIrqWatcher::CheckIrq(uint32 address)
{
	for(int i = 0; i < MAX_CORES; i++)
	{
		if(address == m_irqAddr[i])
		{
			m_irqPending[i] = true;
		}
	}
}

void CSpuIrqWatcher::ClearIrqPending(int core)
{
	m_irqPending[core] = false;
}

bool CSpuIrqWatcher::HasPendingIrq(int core) const
{
	return m_irqPending[core];
}

///////////////////////////////////////////////////////
// CSampleReader
///////////////////////////////////////////////////////

CSpuBase::CSampleReader::CSampleReader()
{
	Reset();
}

void CSpuBase::CSampleReader::Reset()
{
	m_nextSampleAddr = 0;
	m_repeatAddr = 0;
	memset(m_buffer, 0, sizeof(m_buffer));
	m_pitch = 0;
	m_srcSampleIdx = 0;
	m_srcSamplingRate = 0;
	m_dstSamplingRate = 0;
	m_sampleStep = 0;
	m_s1 = 0;
	m_s2 = 0;
	m_done = false;
	m_didChangeRepeat = false;
	m_nextValid = false;
	m_endFlag = false;
}

void CSpuBase::CSampleReader::SetMemory(uint8* ram, uint32 ramSize)
{
	m_ram = ram;
	m_ramSize = ramSize;
	assert((ramSize & (ramSize - 1)) == 0);
}

void CSpuBase::CSampleReader::SetSampleCache(CSpuSampleCache* sampleCache)
{
	m_sampleCache = sampleCache;
}

void CSpuBase::CSampleReader::SetIrqWatcher(CSpuIrqWatcher* irqWatcher)
{
	m_irqWatcher = irqWatcher;
}

void CSpuBase::CSampleReader::SetDestinationSamplingRate(uint32 samplingRate)
{
	m_dstSamplingRate = samplingRate;
	UpdateSampleStep();
}

void CSpuBase::CSampleReader::LoadState(const CRegisterState& channelState)
{
	m_srcSampleIdx = channelState.GetRegister32(STATE_SAMPLEREADER_REGS_SRCSAMPLEIDX);
	m_srcSamplingRate = channelState.GetRegister32(STATE_SAMPLEREADER_REGS_SRCSAMPLINGRATE);
	m_nextSampleAddr = channelState.GetRegister32(STATE_SAMPLEREADER_REGS_NEXTSAMPLEADDR);
	m_repeatAddr = channelState.GetRegister32(STATE_SAMPLEREADER_REGS_REPEATADDR);
	m_pitch = channelState.GetRegister32(STATE_SAMPLEREADER_REGS_PITCH);
	m_s1 = channelState.GetRegister32(STATE_SAMPLEREADER_REGS_S1);
	m_s2 = channelState.GetRegister32(STATE_SAMPLEREADER_REGS_S2);
	m_done = channelState.GetRegister32(STATE_SAMPLEREADER_REGS_DONE) != 0;
	m_nextValid = channelState.GetRegister32(STATE_SAMPLEREADER_REGS_NEXTVALID) != 0;
	m_endFlag = channelState.GetRegister32(STATE_SAMPLEREADER_REGS_ENDFLAG) != 0;
	m_didChangeRepeat = channelState.GetRegister32(STATE_SAMPLEREADER_REGS_DIDCHANGEREPEAT) != 0;
	RegisterStateUtils::ReadArray(channelState, m_buffer, STATE_SAMPLEREADER_REGS_BUFFER_FORMAT);

	UpdateSampleStep();
}

void CSpuBase::CSampleReader::SaveState(CRegisterState& channelState) const
{
	channelState.SetRegister32(STATE_SAMPLEREADER_REGS_SRCSAMPLEIDX, m_srcSampleIdx);
	channelState.SetRegister32(STATE_SAMPLEREADER_REGS_SRCSAMPLINGRATE, m_srcSamplingRate);
	channelState.SetRegister32(STATE_SAMPLEREADER_REGS_NEXTSAMPLEADDR, m_nextSampleAddr);
	channelState.SetRegister32(STATE_SAMPLEREADER_REGS_REPEATADDR, m_repeatAddr);
	channelState.SetRegister32(STATE_SAMPLEREADER_REGS_PITCH, m_pitch);
	channelState.SetRegister32(STATE_SAMPLEREADER_REGS_S1, m_s1);
	channelState.SetRegister32(STATE_SAMPLEREADER_REGS_S2, m_s2);
	channelState.SetRegister32(STATE_SAMPLEREADER_REGS_DONE, m_done);
	channelState.SetRegister32(STATE_SAMPLEREADER_REGS_NEXTVALID, m_nextValid);
	channelState.SetRegister32(STATE_SAMPLEREADER_REGS_ENDFLAG, m_endFlag);
	channelState.SetRegister32(STATE_SAMPLEREADER_REGS_DIDCHANGEREPEAT, m_didChangeRepeat);
	RegisterStateUtils::WriteArray(channelState, m_buffer, STATE_SAMPLEREADER_REGS_BUFFER_FORMAT);
}

void CSpuBase::CSampleReader::SetParams(uint32 address, uint32 repeat)
{
	m_srcSampleIdx = 0;
	m_nextSampleAddr = address & (m_ramSize - 1);
	m_repeatAddr = repeat & (m_ramSize - 1);
	m_s1 = 0;
	m_s2 = 0;
	m_nextValid = false;
	m_done = false;
	m_didChangeRepeat = false;
}

void CSpuBase::CSampleReader::SetParamsRead(uint32 address, uint32 repeat)
{
	SetParams(address, repeat);
	AdvanceBuffer();
}

void CSpuBase::CSampleReader::SetParamsNoRead(uint32 address, uint32 repeat)
{
	SetParams(address, repeat);
	memset(m_buffer, 0, sizeof(m_buffer));
}

void CSpuBase::CSampleReader::SetPitch(uint32 baseSamplingRate, uint16 pitch)
{
	m_srcSamplingRate = baseSamplingRate * pitch;
	UpdateSampleStep();
}

int32 CSpuBase::CSampleReader::GetSample()
{
	uint32 srcSampleIdx = m_srcSampleIdx / PITCH_BASE;
	int32 srcSampleAlpha = m_srcSampleIdx % PITCH_BASE;
	int32 currentSample = m_buffer[srcSampleIdx];
	int32 nextSample = m_buffer[srcSampleIdx + 1];
	int32 resultSample = (currentSample * (PITCH_BASE - srcSampleAlpha) / PITCH_BASE) +
	                     (nextSample * srcSampleAlpha / PITCH_BASE);
	m_srcSampleIdx += m_sampleStep;
	if(srcSampleIdx >= BUFFER_SAMPLES)
	{
		m_srcSampleIdx -= BUFFER_SAMPLES * PITCH_BASE;
		AdvanceBuffer();
	}
	return resultSample;
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

void CSpuBase::CSampleReader::UpdateSampleStep()
{
	m_sampleStep = m_srcSamplingRate / m_dstSamplingRate;
}

void CSpuBase::CSampleReader::UnpackSamples(int16* dst)
{
	int32 workBuffer[BUFFER_SAMPLES];

	const uint8* nextSample = m_ram + m_nextSampleAddr;

	m_irqWatcher->CheckIrq(m_nextSampleAddr);

	//Read header
	uint8 shiftFactor = nextSample[0] & 0xF;
	uint8 predictNumber = nextSample[0] >> 4;
	uint8 flags = nextSample[1];
	assert(predictNumber < 5);

	auto cacheKey = CSpuSampleCache::KEY{m_nextSampleAddr, m_s1, m_s2};
	if(predictNumber == 0)
	{
		cacheKey.s1 = 0;
		cacheKey.s2 = 0;
	}

	if(auto cacheItem = m_sampleCache->GetItem(cacheKey))
	{
		memcpy(dst, cacheItem->samples, sizeof(int16) * BUFFER_SAMPLES);
		m_s1 = cacheItem->outS1;
		m_s2 = cacheItem->outS2;
	}
	else
	{
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
			// clang-format off
			//Table is 16 entries long to prevent reading indeterminate
			//values if predictNumber is greater or equal to 5.
			//According to some sources, entries at 5 and beyond contain 0 on real hardware
			static const int32 predictorTable[16][2] =
			{
				{0, 0},
				{60, 0},
				{115, -52},
				{98, -55},
				{122, -60},
			};
			// clang-format on

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

			auto& cacheItem = m_sampleCache->RegisterItem(cacheKey);
			memcpy(&cacheItem.samples, dst, sizeof(int16) * BUFFER_SAMPLES);
			cacheItem.outS1 = m_s1;
			cacheItem.outS2 = m_s2;
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
		m_nextSampleAddr = m_repeatAddr;

		//If flags is in { 0x01, 0x05, 0x07 }, mute channel (Xenogears requires that)
		if(flags != 0x03)
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
	m_repeatAddr = repeatAddr & (m_ramSize - 1);
}

uint32 CSpuBase::CSampleReader::GetCurrent() const
{
	//Simulate a kind of progress inside the current sample address.
	//Doesn't need to be accurate, but it needs to change. Needed by Romancing Saga.
	uint32 intraSampleIdx = std::min<uint32>((m_srcSampleIdx / PITCH_BASE) / 2, 0x0E);
	return m_nextSampleAddr + intraSampleIdx;
}

bool CSpuBase::CSampleReader::IsDone() const
{
	return m_done;
}

void CSpuBase::CSampleReader::ClearIsDone()
{
	m_done = false;
}

bool CSpuBase::CSampleReader::GetEndFlag() const
{
	return m_endFlag;
}

void CSpuBase::CSampleReader::ClearEndFlag()
{
	m_endFlag = false;
}

bool CSpuBase::CSampleReader::DidChangeRepeat() const
{
	return m_didChangeRepeat;
}

void CSpuBase::CSampleReader::ClearDidChangeRepeat()
{
	m_didChangeRepeat = false;
}

///////////////////////////////////////////////////////
// CBlockSampleReader
///////////////////////////////////////////////////////

void CSpuBase::CBlockSampleReader::Reset()
{
	m_baseSamplingRate = INIT_SAMPLE_RATE;
	m_dstSamplingRate = 0;
	m_srcSampleIdx = SOUND_INPUT_DATA_SAMPLES * TIME_SCALE;
	m_sampleStep = 0;
}

void CSpuBase::CBlockSampleReader::SetBaseSamplingRate(uint32 baseSamplingRate)
{
	m_baseSamplingRate = baseSamplingRate;
	UpdateSampleStep();
}

void CSpuBase::CBlockSampleReader::SetDestinationSamplingRate(uint32 dstSamplingRate)
{
	m_dstSamplingRate = dstSamplingRate;
	UpdateSampleStep();
}

bool CSpuBase::CBlockSampleReader::GetSpdifBypass()
{
	return m_spdifBypass;
}

void CSpuBase::CBlockSampleReader::SetSpdifBypass(bool spdifBypass)
{
	m_spdifBypass = spdifBypass;
}

bool CSpuBase::CBlockSampleReader::CanReadSamples() const
{
	uint32 sampleIdx = (m_srcSampleIdx / TIME_SCALE);
	if(m_spdifBypass)
	{
		// in S/PDIF bypass mode, we consume 2 adjacent samples per cycle
		// as a stereo pair.
		return (sampleIdx < (SOUND_INPUT_DATA_SAMPLES / 2));
	}
	else
	{
		return (sampleIdx < SOUND_INPUT_DATA_SAMPLES);
	}
}

void CSpuBase::CBlockSampleReader::FillBlock(const uint8* block)
{
	memcpy(m_blockBuffer, block, SOUND_INPUT_DATA_SIZE);
	m_srcSampleIdx = 0;
}

void CSpuBase::CBlockSampleReader::GetSamples(int32 samples[2])
{
	assert(m_sampleStep != 0);

	uint32 srcSampleIdx = m_srcSampleIdx / TIME_SCALE;
	assert(srcSampleIdx < SOUND_INPUT_DATA_SAMPLES);

	auto inputSamples = reinterpret_cast<const int16*>(m_blockBuffer);

	if(m_spdifBypass)
	{
		// in S/PDIF bypass mode, input data contains linear interleaved
		// stereo samples, in a single stream. The L sample should always
		// be at an even-numbered index.
		samples[0] = inputSamples[0x000 + (srcSampleIdx * 2)];
		samples[1] = inputSamples[0x000 + (srcSampleIdx * 2) + 1];
	}
	else
	{
		samples[0] = inputSamples[0x000 + srcSampleIdx];
		samples[1] = inputSamples[0x100 + srcSampleIdx];
	}

	m_srcSampleIdx += m_sampleStep;
}

void CSpuBase::CBlockSampleReader::UpdateSampleStep()
{
	m_sampleStep = (m_baseSamplingRate * TIME_SCALE) / m_dstSamplingRate;
}
