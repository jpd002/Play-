#include "Iop_SifMan.h"
#include "../Log.h"

#define LOG_NAME ("iop_sifman")

using namespace Iop;
using namespace std;

CSifMan::CSifMan()
{

}

CSifMan::~CSifMan()
{

}

string CSifMan::GetId() const
{
	return "sifman";
}

string CSifMan::GetFunctionName(unsigned int functionId) const
{
	return "unknown";
}

void CSifMan::Invoke(CMIPS& context, unsigned int functionId)
{
	switch(functionId)
	{
	case 7:
		context.m_State.nGPR[CMIPS::V0].nD0 = static_cast<int32>(SifSetDma(
			context.m_State.nGPR[CMIPS::A0].nV0,
			context.m_State.nGPR[CMIPS::A1].nV0
			));
		break;
	default:
		CLog::GetInstance().Print(LOG_NAME, "%0.8X: Unknown function (%d) called.", context.m_State.nPC, functionId);
        break;
	}
}

uint32 CSifMan::SifSetDma(uint32 structAddr, uint32 length)
{
	CLog::GetInstance().Print(LOG_NAME, "SifSetDma(structAddr = 0x%0.8X, length = %X);\r\n",
		structAddr, length);
	return 1;
}

