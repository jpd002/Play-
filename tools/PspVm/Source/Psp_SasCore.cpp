#include "Psp_SasCore.h"
#include "Log.h"

#define LOGNAME	("Psp_SasCore")

using namespace Psp;

CSasCore::CSasCore()
{

}

CSasCore::~CSasCore()
{

}

std::string CSasCore::GetName() const
{
	return "sceSasCore";
}

uint32 CSasCore::Init(uint32 contextAddr, uint32 grain, uint32 unknown2, uint32 unknown3, uint32 frequency)
{
#ifdef _DEBUG
	CLog::GetInstance().Print(LOGNAME, "Init(contextAddr = 0x%0.8X, grain = %d, unk = 0x%0.8X, unk = 0x%0.8X, frequency = %d);\r\n",
		contextAddr, grain, unknown2, unknown3, frequency);
#endif
	return 0;
}

uint32 CSasCore::Core(uint32 contextAddr, uint32 bufferAddr)
{
#ifdef _DEBUG
	CLog::GetInstance().Print(LOGNAME, "Core(contextAddr = 0x%0.8X, bufferAddr = 0x%0.8X);\r\n", contextAddr, bufferAddr);
#endif
	return 0;
}

uint32 CSasCore::SetVoice(uint32 contextAddr, uint32 voice, uint32 dataPtr, uint32 dataSize, uint32 loop)
{
#ifdef _DEBUG
	CLog::GetInstance().Print(LOGNAME, "SetVoice(contextAddr = 0x%0.8X, voice = %d, dataPtr = 0x%0.8X, dataSize = 0x%0.8X, loop = %d);\r\n",
		contextAddr, voice, dataPtr, dataSize, loop);
#endif
	return 0;
}

uint32 CSasCore::SetPitch(uint32 contextAddr, uint32 voice, uint32 pitch)
{
#ifdef _DEBUG
	CLog::GetInstance().Print(LOGNAME, "SetPitch(contextAddr = 0x%0.8X, voice = %d, pitch = 0x%0.4X);\r\n",
		contextAddr, voice, pitch);
#endif
	return 0;
}

uint32 CSasCore::SetVolume(uint32 contextAddr, uint32 voice, uint32 left, uint32 right, uint32 effectLeft, uint32 effectRight)
{
#ifdef _DEBUG
	CLog::GetInstance().Print(LOGNAME, "SetVolume(contextAddr = 0x%0.8X, voice = %d, left = 0x%0.4X, right = 0x%0.4X, effectLeft = 0x%0.4X, effectRight = 0x%0.4X);\r\n",
		contextAddr, voice, left, right, effectLeft, effectRight);
#endif
	return 0;
}

uint32 CSasCore::SetSimpleADSR(uint32 contextAddr, uint32 voice, uint32 adsr1, uint32 adsr2)
{
#ifdef _DEBUG
	CLog::GetInstance().Print(LOGNAME, "SetSimpleADSR(contextAddr = 0x%0.8X, voice = %d, adsr1 = 0x%0.4X, adsr2 = 0x%0.4X);\r\n",
		contextAddr, voice, adsr1, adsr2);
#endif
	return 0;
}

uint32 CSasCore::SetKeyOn(uint32 contextAddr, uint32 voice)
{
#ifdef _DEBUG
	CLog::GetInstance().Print(LOGNAME, "SetKeyOn(contextAddr = 0x%0.8X, voice = %d);\r\n", contextAddr, voice);
#endif
	return 0;
}

uint32 CSasCore::SetKeyOff(uint32 contextAddr, uint32 voice)
{
#ifdef _DEBUG
	CLog::GetInstance().Print(LOGNAME, "SetKeyOff(contextAddr = 0x%0.8X, voice = %d);\r\n", contextAddr, voice);
#endif
	return 0;
}

uint32 CSasCore::GetAllEnvelope(uint32 contextAddr, uint32 envelopeAddr)
{
#ifdef _DEBUG
	CLog::GetInstance().Print(LOGNAME, "GetAllEnvelope(contextAddr = 0x%0.8X, envelopeAddr = 0x%0.8X);\r\n", contextAddr, envelopeAddr);
#endif
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
	return 0xFFFFFFFF;
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
