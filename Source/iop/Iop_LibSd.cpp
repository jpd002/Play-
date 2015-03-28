#include "Iop_LibSd.h"

//Not an actual implementation of the LIBSD module
//It is only used for debugging purposes (ie.: function names)

using namespace Iop;

#define FUNCTION_SETPARAM				"SetParam"
#define FUNCTION_SETSWITCH				"SetSwitch"
#define FUNCTION_SETADDR				"SetAddr"
#define FUNCTION_GETADDR				"GetAddr"
#define FUNCTION_SETCOREATTR			"SetCoreAttr"
#define FUNCTION_SETSPU2INTRHANDLER		"SetSpu2IntrHandler"

std::string CLibSd::GetId() const
{
	return "libsd";
}

std::string CLibSd::GetFunctionName(unsigned int functionId) const
{
	switch(functionId)
	{
	case 5:
		return FUNCTION_SETPARAM;
		break;
	case 7:
		return FUNCTION_SETSWITCH;
		break;
	case 9:
		return FUNCTION_SETADDR;
		break;
	case 10:
		return FUNCTION_GETADDR;
		break;
	case 11:
		return FUNCTION_SETCOREATTR;
		break;
	case 27:
		return FUNCTION_SETSPU2INTRHANDLER;
		break;
	default:
		return "unknown";
		break;
	}
}

void CLibSd::Invoke(CMIPS&, unsigned int)
{

}

#if 0
#define LOG_NAME "libsd"

void CLibSd::DumpLibSdCall(uint32 functionId)
{
	switch(functionId)
	{
	case 4:
		CLog::GetInstance().Print(LIBSD_LOG_NAME, "sceSdInit(flag = %d);\r\n", 
			m_cpu.m_State.nGPR[CMIPS::A0].nV0);
		break;
	case 5:
		CLog::GetInstance().Print(LIBSD_LOG_NAME, "sceSdSetParam(entry = 0x%0.4X, value = 0x%0.4X);\r\n", 
			m_cpu.m_State.nGPR[CMIPS::A0].nV0, m_cpu.m_State.nGPR[CMIPS::A1].nV0);
		break;
	case 7:
		CLog::GetInstance().Print(LIBSD_LOG_NAME, "sceSdSetSwitch(entry = 0x%0.4X, value = 0x%0.8X);\r\n",
			m_cpu.m_State.nGPR[CMIPS::A0].nV0, m_cpu.m_State.nGPR[CMIPS::A1].nV0);
		break;
	case 9:
		CLog::GetInstance().Print(LIBSD_LOG_NAME, "sceSdSetAddr(entry = 0x%0.4X, value = 0x%0.8X);\r\n", 
			m_cpu.m_State.nGPR[CMIPS::A0].nV0, m_cpu.m_State.nGPR[CMIPS::A1].nV0);
		break;
	case 10:
		CLog::GetInstance().Print(LIBSD_LOG_NAME, "sceSdGetAddr(entry = 0x%0.4X);\r\n", m_cpu.m_State.nGPR[CMIPS::A0].nV0);
		break;
	case 17:
		CLog::GetInstance().Print(LIBSD_LOG_NAME, "sceSdVoiceTrans(channel = 0x%0.4X, mode = 0x%0.4X, maddr = 0x%0.8X, saddr = 0x%0.8X, size = 0x%0.8X);\r\n",
			m_cpu.m_State.nGPR[CMIPS::A0].nV0, m_cpu.m_State.nGPR[CMIPS::A1].nV0, 
			m_cpu.m_State.nGPR[CMIPS::A2].nV0, m_cpu.m_State.nGPR[CMIPS::A3].nV0, 
			m_cpu.m_State.nGPR[CMIPS::T0].nV0);
		break;
	case 18:
		CLog::GetInstance().Print(LIBSD_LOG_NAME, "sceSdBlockTrans(channel = 0x%0.4X, mode = 0x%0.4X, maddr = 0x%0.8X, size = 0x%0.8X);\r\n",
			m_cpu.m_State.nGPR[CMIPS::A0].nV0, m_cpu.m_State.nGPR[CMIPS::A1].nV0, 
			m_cpu.m_State.nGPR[CMIPS::A2].nV0, m_cpu.m_State.nGPR[CMIPS::A3].nV0);
		break;
	case 19:
		CLog::GetInstance().Print(LIBSD_LOG_NAME, "sceSdVoiceTransStatus(channel = 0x%0.4X, flag = 0x%0.4X);\r\n",
			m_cpu.m_State.nGPR[CMIPS::A0].nV0, m_cpu.m_State.nGPR[CMIPS::A1].nV0);
		break;
	case 20:
		CLog::GetInstance().Print(LIBSD_LOG_NAME, "sceSdBlockTransStatus(channel = 0x%0.4X, flag = 0x%0.4X);\r\n",
			m_cpu.m_State.nGPR[CMIPS::A0].nV0, m_cpu.m_State.nGPR[CMIPS::A1].nV0);
		break;
	default:
		CLog::GetInstance().Print(LIBSD_LOG_NAME, "unknownlibsd(%d);\r\n", functionId);
		break;
	}
}
#endif
