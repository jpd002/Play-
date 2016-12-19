#include "Iop_Naplink.h"
#include "Iop_Ioman.h"
#include "../Log.h"

//Naplink is required by some demos that use nprintf
//The demos won't make an explicit attempt at loading the module
//thus, it must be loaded by the system for the demos to work

#define LOG_NAME "iop_naplink"

using namespace Iop;

CNaplink::CNaplink(CSifMan& sifMan, CIoman& ioMan)
: m_ioMan(ioMan)
{
	sifMan.RegisterModule(SIF_MODULE_ID, this);
}

std::string CNaplink::GetId() const
{
	return "naplink";
}

std::string CNaplink::GetFunctionName(unsigned int) const
{
	return "unknown";
}

void CNaplink::Invoke(CMIPS& context, unsigned int functionId)
{
	throw std::runtime_error("Not implemented.");
}

bool CNaplink::Invoke(uint32 method, uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	switch(method)
	{
	case 1:
		{
			uint32 writeSize = strlen(reinterpret_cast<const char*>(args));
			m_ioMan.Write(CIoman::FID_STDOUT, writeSize, args);
		}
		break;
	default:
		CLog::GetInstance().Print(LOG_NAME, "Unknown method invoked (0x%0.8X).\r\n", method);
		break;
	}
	return true;
}
