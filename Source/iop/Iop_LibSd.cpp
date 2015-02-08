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
