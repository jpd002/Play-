#include "Psp_SasCore.h"
#include "Log.h"

#define LOGNAME	("Psp_SasCore")

using namespace Psp;

CSasCore::CSasCore(uint8* ram)
: m_spuRam(NULL)
, m_spuRamSize(0)
, m_ram(ram)
, m_grain(0)
{
	m_spu[0] = NULL;
	m_spu[1] = NULL;
}

CSasCore::~CSasCore()
{

}

std::string CSasCore::GetName() const
{
	return "sceSasCore";
}

void CSasCore::SetSpuInfo(Iop::CSpuBase* spu0, Iop::CSpuBase* spu1, uint8* spuRam, uint32 spuRamSize)
{
	assert(m_spu[0] == NULL && m_spu[1] == NULL);

	m_spu[0] = spu0;
	m_spu[1] = spu1;

	m_spuRam = spuRam;
	m_spuRamSize = spuRamSize;
	m_spuRam[1] = 0x07;

	SPUMEMBLOCK endBlock;
	endBlock.address	= m_spuRamSize;
	endBlock.size		= -1;
	m_blocks.push_back(endBlock);
}

Iop::CSpuBase::CHANNEL* CSasCore::GetSpuChannel(uint32 channelId)
{
	assert(channelId < 32);
	if(channelId >= 32)
	{
		return NULL;
	}

	Iop::CSpuBase* spuBase(NULL);
	if(channelId < 24)
	{
		spuBase = m_spu[0];
	}
	else
	{
		spuBase = m_spu[1];
		channelId -= 24;
	}

	return &spuBase->GetChannel(channelId);
}

uint32 CSasCore::AllocMemory(uint32 size)
{
	assert(m_spuRamSize != 0);

	const uint32 startAddress = 0x1000;
	uint32 currentAddress = startAddress;
	MemBlockList::iterator blockIterator(m_blocks.begin());
	while(blockIterator != m_blocks.end())
	{
		const SPUMEMBLOCK& block(*blockIterator);
		uint32 space = block.address - currentAddress;
		if(space >= size)
		{
			SPUMEMBLOCK newBlock;
			newBlock.address	= currentAddress;
			newBlock.size		= size;
			m_blocks.insert(blockIterator, newBlock);
			return currentAddress;
		}
		currentAddress = block.address + block.size;
		blockIterator++;
	}
	assert(0);
	return 0;
}

void CSasCore::FreeMemory(uint32 address)
{
	for(MemBlockList::iterator blockIterator(m_blocks.begin());
		blockIterator != m_blocks.end(); blockIterator++)
	{
		const SPUMEMBLOCK& block(*blockIterator);
		if(block.address == address)
		{
			m_blocks.erase(blockIterator);
			return;
		}
	}
	assert(0);
}

#ifdef _DEBUG

void CSasCore::VerifyAllocationMap()
{
	for(MemBlockList::iterator blockIterator(m_blocks.begin());
		blockIterator != m_blocks.end(); blockIterator++)
	{
		MemBlockList::iterator nextBlockIterator = blockIterator;
		nextBlockIterator++;
		if(nextBlockIterator == m_blocks.end()) break;
		SPUMEMBLOCK& currBlock(*blockIterator);
		SPUMEMBLOCK& nextBlock(*nextBlockIterator);
		assert(currBlock.address + currBlock.size <= nextBlock.address);
	}
}

#endif

uint32 CSasCore::Init(uint32 contextAddr, uint32 grain, uint32 unknown2, uint32 unknown3, uint32 frequency)
{
#ifdef _DEBUG
	CLog::GetInstance().Print(LOGNAME, "Init(contextAddr = 0x%0.8X, grain = %d, unk = 0x%0.8X, unk = 0x%0.8X, frequency = %d);\r\n",
		contextAddr, grain, unknown2, unknown3, frequency);
#endif
	m_grain = grain;
	return 0;
}

uint32 CSasCore::Core(uint32 contextAddr, uint32 bufferAddr)
{
#ifdef _DEBUG
	CLog::GetInstance().Print(LOGNAME, "Core(contextAddr = 0x%0.8X, bufferAddr = 0x%0.8X);\r\n", contextAddr, bufferAddr);
#endif

	assert(bufferAddr != 0);
	if(bufferAddr == 0)
	{
		return -1;
	}

	int16* buffer = reinterpret_cast<int16*>(m_ram + bufferAddr);
	for(unsigned int i = 0; i < 1; i++)
	{
		m_spu[i]->Render(buffer, m_grain * 2, 44100);
	}

	return 0;
}

uint32 CSasCore::SetVoice(uint32 contextAddr, uint32 voice, uint32 dataPtr, uint32 dataSize, uint32 loop)
{
#ifdef _DEBUG
	CLog::GetInstance().Print(LOGNAME, "SetVoice(contextAddr = 0x%0.8X, voice = %d, dataPtr = 0x%0.8X, dataSize = 0x%0.8X, loop = %d);\r\n",
		contextAddr, voice, dataPtr, dataSize, loop);
#endif
	assert(dataPtr != NULL);
	if(dataPtr == NULL)
	{
		return -1;
	}

	Iop::CSpuBase::CHANNEL* channel = GetSpuChannel(voice);
	if(channel == NULL) return -1;
	uint8* samples = m_ram + dataPtr;

	uint32 currentAddress = channel->address;
	uint32 allocationSize = ((dataSize + 0xF) / 0x10) * 0x10;

	if(currentAddress != 0)
	{
		FreeMemory(currentAddress);
	}

	currentAddress = AllocMemory(allocationSize);
	assert((currentAddress + allocationSize) <= m_spuRamSize);
	assert(currentAddress != 0);
	if(currentAddress != 0)
	{
		memcpy(m_spuRam + currentAddress, samples, dataSize);
	}

	channel->address = currentAddress;
	channel->repeat = currentAddress;

	return 0;
}

uint32 CSasCore::SetPitch(uint32 contextAddr, uint32 voice, uint32 pitch)
{
#ifdef _DEBUG
	CLog::GetInstance().Print(LOGNAME, "SetPitch(contextAddr = 0x%0.8X, voice = %d, pitch = 0x%0.4X);\r\n",
		contextAddr, voice, pitch);
#endif
	Iop::CSpuBase::CHANNEL* channel = GetSpuChannel(voice);
	if(channel == NULL) return -1;
	channel->pitch = pitch;
	return 0;
}

uint32 CSasCore::SetVolume(uint32 contextAddr, uint32 voice, uint32 left, uint32 right, uint32 effectLeft, uint32 effectRight)
{
#ifdef _DEBUG
	CLog::GetInstance().Print(LOGNAME, "SetVolume(contextAddr = 0x%0.8X, voice = %d, left = 0x%0.4X, right = 0x%0.4X, effectLeft = 0x%0.4X, effectRight = 0x%0.4X);\r\n",
		contextAddr, voice, left, right, effectLeft, effectRight);
#endif
	Iop::CSpuBase::CHANNEL* channel = GetSpuChannel(voice);
	if(channel == NULL) return -1;
	channel->volumeLeft <<= (left * 4);
	channel->volumeRight <<= (right * 4);
	return 0;
}

uint32 CSasCore::SetSimpleADSR(uint32 contextAddr, uint32 voice, uint32 adsr1, uint32 adsr2)
{
#ifdef _DEBUG
	CLog::GetInstance().Print(LOGNAME, "SetSimpleADSR(contextAddr = 0x%0.8X, voice = %d, adsr1 = 0x%0.4X, adsr2 = 0x%0.4X);\r\n",
		contextAddr, voice, adsr1, adsr2);
#endif
	Iop::CSpuBase::CHANNEL* channel = GetSpuChannel(voice);
	if(channel == NULL) return -1;
	channel->adsrLevel <<= adsr1;
	channel->adsrRate <<= adsr2;
	return 0;
}

uint32 CSasCore::SetKeyOn(uint32 contextAddr, uint32 voice)
{
#ifdef _DEBUG
	CLog::GetInstance().Print(LOGNAME, "SetKeyOn(contextAddr = 0x%0.8X, voice = %d);\r\n", contextAddr, voice);
#endif

	assert(voice < 32);
	if(voice >= 32)
	{
		return -1;
	}

	Iop::CSpuBase* spuBase(NULL);
	if(voice < 24)
	{
		spuBase = m_spu[0];
	}
	else
	{
		spuBase = m_spu[1];
		voice -= 24;
	}

	spuBase->SendKeyOn(1 << voice);

	return 0;
}

uint32 CSasCore::SetKeyOff(uint32 contextAddr, uint32 voice)
{
#ifdef _DEBUG
	CLog::GetInstance().Print(LOGNAME, "SetKeyOff(contextAddr = 0x%0.8X, voice = %d);\r\n", contextAddr, voice);
#endif

	assert(voice < 32);
	if(voice >= 32)
	{
		return -1;
	}

	Iop::CSpuBase* spuBase(NULL);
	if(voice < 24)
	{
		spuBase = m_spu[0];
	}
	else
	{
		spuBase = m_spu[1];
		voice -= 24;
	}

	spuBase->SendKeyOff(1 << voice);

	return 0;
}

uint32 CSasCore::GetAllEnvelope(uint32 contextAddr, uint32 envelopeAddr)
{
#ifdef _DEBUG
	CLog::GetInstance().Print(LOGNAME, "GetAllEnvelope(contextAddr = 0x%0.8X, envelopeAddr = 0x%0.8X);\r\n", contextAddr, envelopeAddr);
#endif
	assert(envelopeAddr != 0);
	if(envelopeAddr == 0)
	{
		return -1;
	}
	uint32* envelope = reinterpret_cast<uint32*>(m_ram + envelopeAddr);
	for(unsigned int i = 0; i < 32; i++)
	{
		Iop::CSpuBase::CHANNEL* channel = GetSpuChannel(i);
		envelope[i] = channel->adsrVolume;
	}
	return 0;
}

uint32 CSasCore::GetPauseFlag(uint32 contextAddr)
{
#ifdef _DEBUG
	CLog::GetInstance().Print(LOGNAME, "GetPauseFlag(contextAddr = 0x%0.8X);\r\n", contextAddr);
#endif
	return 0;
}

uint32 CSasCore::GetEndFlag(uint32 contextAddr)
{
#ifdef _DEBUG
	CLog::GetInstance().Print(LOGNAME, "GetEndFlag(contextAddr = 0x%0.8X);\r\n", contextAddr);
#endif

	uint32 result = m_spu[0]->GetEndFlags().f | (m_spu[1]->GetEndFlags().f << 24);
	for(unsigned int i = 0; i < 32; i++)
	{
		const Iop::CSpuBase::CHANNEL* channel = GetSpuChannel(i);
		if(channel->status == Iop::CSpuBase::STOPPED)
		{
			result |= (1 << i);
		}
	}

	return result;
}

void CSasCore::Invoke(uint32 methodId, CMIPS& context)
{
	switch(methodId)
	{
	case 0x42778A9F:
		context.m_State.nGPR[CMIPS::V0].nV0 = Init(
			context.m_State.nGPR[CMIPS::A0].nV0,
			context.m_State.nGPR[CMIPS::A1].nV0,
			context.m_State.nGPR[CMIPS::A2].nV0,
			context.m_State.nGPR[CMIPS::A3].nV0,
			context.m_State.nGPR[CMIPS::T0].nV0);
		break;
	case 0xA3589D81:
		context.m_State.nGPR[CMIPS::V0].nV0 = Core(
			context.m_State.nGPR[CMIPS::A0].nV0,
			context.m_State.nGPR[CMIPS::A1].nV0);
		break;
	case 0x99944089:
		context.m_State.nGPR[CMIPS::V0].nV0 = SetVoice(
			context.m_State.nGPR[CMIPS::A0].nV0,
			context.m_State.nGPR[CMIPS::A1].nV0,
			context.m_State.nGPR[CMIPS::A2].nV0,
			context.m_State.nGPR[CMIPS::A3].nV0,
			context.m_State.nGPR[CMIPS::T0].nV0);
		break;
	case 0xAD84D37F:
		context.m_State.nGPR[CMIPS::V0].nV0 = SetPitch(
			context.m_State.nGPR[CMIPS::A0].nV0,
			context.m_State.nGPR[CMIPS::A1].nV0,
			context.m_State.nGPR[CMIPS::A2].nV0);
		break;
	case 0x440CA7D8:
		context.m_State.nGPR[CMIPS::V0].nV0 = SetVolume(
			context.m_State.nGPR[CMIPS::A0].nV0,
			context.m_State.nGPR[CMIPS::A1].nV0,
			context.m_State.nGPR[CMIPS::A2].nV0,
			context.m_State.nGPR[CMIPS::A3].nV0,
			context.m_State.nGPR[CMIPS::T0].nV0,
			context.m_State.nGPR[CMIPS::T1].nV0);
		break;
	case 0xCBCD4F79:
		context.m_State.nGPR[CMIPS::V0].nV0 = SetSimpleADSR(
			context.m_State.nGPR[CMIPS::A0].nV0,
			context.m_State.nGPR[CMIPS::A1].nV0,
			context.m_State.nGPR[CMIPS::A2].nV0,
			context.m_State.nGPR[CMIPS::A3].nV0);
		break;
	case 0x76F01ACA:
		context.m_State.nGPR[CMIPS::V0].nV0 = SetKeyOn(
			context.m_State.nGPR[CMIPS::A0].nV0,
			context.m_State.nGPR[CMIPS::A1].nV0);
		break;
	case 0xA0CF2FA4:
		context.m_State.nGPR[CMIPS::V0].nV0 = SetKeyOff(
			context.m_State.nGPR[CMIPS::A0].nV0,
			context.m_State.nGPR[CMIPS::A1].nV0);
		break;
	case 0x07F58C24:
		context.m_State.nGPR[CMIPS::V0].nV0 = GetAllEnvelope(
			context.m_State.nGPR[CMIPS::A0].nV0,
			context.m_State.nGPR[CMIPS::A1].nV0);
		break;
	case 0x2C8E6AB3:
		context.m_State.nGPR[CMIPS::V0].nV0 = GetPauseFlag(
			context.m_State.nGPR[CMIPS::A0].nV0);
		break;
	case 0x68A46B95:
		context.m_State.nGPR[CMIPS::V0].nV0 = GetEndFlag(
			context.m_State.nGPR[CMIPS::A0].nV0);
		break;
	default:
		CLog::GetInstance().Print(LOGNAME, "Unknown function called 0x%0.8X\r\n", methodId);
		break;
	}
}
