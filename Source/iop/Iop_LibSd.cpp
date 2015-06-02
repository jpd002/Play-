#include "Iop_LibSd.h"
#include "../Log.h"
#include "string_format.h"

//Not an actual implementation of the LIBSD module
//It is only used for debugging purposes (ie.: function names)

using namespace Iop;

#define FUNCTION_INIT					"Init"
#define FUNCTION_SETPARAM				"SetParam"
#define FUNCTION_SETSWITCH				"SetSwitch"
#define FUNCTION_GETSWITCH				"GetSwitch"
#define FUNCTION_SETADDR				"SetAddr"
#define FUNCTION_GETADDR				"GetAddr"
#define FUNCTION_SETCOREATTR			"SetCoreAttr"
#define FUNCTION_VOICETRANS				"VoiceTrans"
#define FUNCTION_BLOCKTRANS				"BlockTrans"
#define FUNCTION_VOICETRANSSTATUS		"VoiceTransStatus"
#define FUNCTION_BLOCKTRANSSTATUS		"BlockTransStatus"
#define FUNCTION_SETTRANSINTRHANDLER	"SetTransIntrHandler"
#define FUNCTION_SETSPU2INTRHANDLER		"SetSpu2IntrHandler"

#define LOG_NAME "iop_libsd"

std::string CLibSd::GetId() const
{
	return "libsd";
}

std::string CLibSd::GetFunctionName(unsigned int functionId) const
{
	switch(functionId)
	{
	case 4:
		return FUNCTION_INIT;
		break;
	case 5:
		return FUNCTION_SETPARAM;
		break;
	case 7:
		return FUNCTION_SETSWITCH;
		break;
	case 8:
		return FUNCTION_GETSWITCH;
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
	case 17:
		return FUNCTION_VOICETRANS;
		break;
	case 18:
		return FUNCTION_BLOCKTRANS;
		break;
	case 19:
		return FUNCTION_VOICETRANSSTATUS;
		break;
	case 20:
		return FUNCTION_BLOCKTRANSSTATUS;
		break;
	case 26:
		return FUNCTION_SETTRANSINTRHANDLER;
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

std::string DecodeSwitch(uint16 switchId)
{
	std::string result;
	switch(switchId >> 8)
	{
	case 0x15:
		result = "KEYON";
		break;
	case 0x17:
		result = "ENDX";
		break;
	default:
		result = string_format("unknown (0x%0.2X)", switchId >> 8);
		break;
	}
	result += string_format(", CORE%d", switchId & 1);
	return result;
}

void CLibSd::TraceCall(CMIPS& context, unsigned int functionId)
{
	switch(functionId)
	{
	case 4:
		CLog::GetInstance().Print(LOG_NAME, FUNCTION_INIT "(flag = %d);\r\n", 
			context.m_State.nGPR[CMIPS::A0].nV0);
		break;
	case 5:
		CLog::GetInstance().Print(LOG_NAME, FUNCTION_SETPARAM "(entry = 0x%0.4X, value = 0x%0.4X);\r\n", 
			context.m_State.nGPR[CMIPS::A0].nV0, context.m_State.nGPR[CMIPS::A1].nV0);
		break;
	case 7:
		CLog::GetInstance().Print(LOG_NAME, FUNCTION_SETSWITCH "(entry = 0x%0.4X, value = 0x%0.8X); //(%s)\r\n",
			context.m_State.nGPR[CMIPS::A0].nV0, context.m_State.nGPR[CMIPS::A1].nV0,
			DecodeSwitch(static_cast<uint16>(context.m_State.nGPR[CMIPS::A0].nV0)).c_str());
		break;
	case 8:
		CLog::GetInstance().Print(LOG_NAME, FUNCTION_GETSWITCH "(entry = 0x%0.4X); //(%s)\r\n", 
			context.m_State.nGPR[CMIPS::A0].nV0, 
			DecodeSwitch(static_cast<uint16>(context.m_State.nGPR[CMIPS::A0].nV0)).c_str());
		break;
	case 9:
		CLog::GetInstance().Print(LOG_NAME, FUNCTION_SETADDR "(entry = 0x%0.4X, value = 0x%0.8X);\r\n", 
			context.m_State.nGPR[CMIPS::A0].nV0, context.m_State.nGPR[CMIPS::A1].nV0);
		break;
	case 10:
		CLog::GetInstance().Print(LOG_NAME, FUNCTION_GETADDR "(entry = 0x%0.4X);\r\n", context.m_State.nGPR[CMIPS::A0].nV0);
		break;
	case 11:
		CLog::GetInstance().Print(LOG_NAME, FUNCTION_SETCOREATTR "(entry = 0x%0.4X, value = 0x%0.4X);\r\n",
			context.m_State.nGPR[CMIPS::A0].nV0, context.m_State.nGPR[CMIPS::A1].nV0);
		break;
	case 17:
		CLog::GetInstance().Print(LOG_NAME, FUNCTION_VOICETRANS "(channel = 0x%0.4X, mode = 0x%0.4X, maddr = 0x%0.8X, saddr = 0x%0.8X, size = 0x%0.8X);\r\n",
			context.m_State.nGPR[CMIPS::A0].nV0, context.m_State.nGPR[CMIPS::A1].nV0, 
			context.m_State.nGPR[CMIPS::A2].nV0, context.m_State.nGPR[CMIPS::A3].nV0, 
			context.m_State.nGPR[CMIPS::T0].nV0);
		break;
	case 18:
		CLog::GetInstance().Print(LOG_NAME, FUNCTION_BLOCKTRANS "(channel = 0x%0.4X, mode = 0x%0.4X, maddr = 0x%0.8X, size = 0x%0.8X);\r\n",
			context.m_State.nGPR[CMIPS::A0].nV0, context.m_State.nGPR[CMIPS::A1].nV0, 
			context.m_State.nGPR[CMIPS::A2].nV0, context.m_State.nGPR[CMIPS::A3].nV0);
		break;
	case 19:
		CLog::GetInstance().Print(LOG_NAME, FUNCTION_VOICETRANSSTATUS "(channel = 0x%0.4X, flag = 0x%0.4X);\r\n",
			context.m_State.nGPR[CMIPS::A0].nV0, context.m_State.nGPR[CMIPS::A1].nV0);
		break;
	case 20:
		CLog::GetInstance().Print(LOG_NAME, FUNCTION_BLOCKTRANSSTATUS "(channel = 0x%0.4X, flag = 0x%0.4X);\r\n",
			context.m_State.nGPR[CMIPS::A0].nV0, context.m_State.nGPR[CMIPS::A1].nV0);
		break;
	case 26:
		CLog::GetInstance().Print(LOG_NAME, FUNCTION_SETTRANSINTRHANDLER "(channel = 0x%0.4X, function = 0x%0.8X, arg = 0x%0.8X);\r\n",
			context.m_State.nGPR[CMIPS::A0].nV0, context.m_State.nGPR[CMIPS::A1].nV0,
			context.m_State.nGPR[CMIPS::A2].nV0);
		break;
	case 27:
		CLog::GetInstance().Print(LOG_NAME, FUNCTION_SETSPU2INTRHANDLER "(function = 0x%0.8X, arg = 0x%0.8X);\r\n",
			context.m_State.nGPR[CMIPS::A0].nV0, context.m_State.nGPR[CMIPS::A1].nV0);
		break;
	default:
		CLog::GetInstance().Print(LOG_NAME, "unknownlibsd(%d);\r\n", functionId);
		break;
	}
}
