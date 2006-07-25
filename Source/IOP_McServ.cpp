#include <assert.h>
#include <stdio.h>
#include "IOP_McServ.h"
#include "PS2VM.h"

using namespace IOP;
using namespace Framework;

CMcServ::CMcServ()
{
	
}

CMcServ::~CMcServ()
{

}

void CMcServ::Invoke(uint32 nMethod, void* pArgs, uint32 nArgsSize, void* pRet, uint32 nRetSize)
{
	switch(nMethod)
	{
	case 0x01:
		GetInfo(pArgs, nArgsSize, pRet, nRetSize);
		break;
	case 0xFE:
		//Get version?
		GetVersionInformation(pArgs, nArgsSize, pRet, nRetSize);
		break;
	default:
		Log("Unknown method invoked (0x%0.8X).\r\n", nMethod);
		break;
	}
}

void CMcServ::SaveState(CStream* pStream)
{

}

void CMcServ::LoadState(CStream* pStream)
{

}

void CMcServ::GetInfo(void* pArgs, uint32 nArgsSize, void* pRet, uint32 nRetSize)
{
	assert(nArgsSize >= 0x1C);

	uint32 nPort, nSlot;
	uint32* pRetBuffer;

	bool nWantType;
	bool nWantFreeSpace;
	bool nWantFormatted;
	
	nPort			= ((uint32*)pArgs)[1];
	nSlot			= ((uint32*)pArgs)[2];
	nWantType		= ((uint32*)pArgs)[3] != 0;
	nWantFreeSpace	= ((uint32*)pArgs)[4] != 0;
	nWantFormatted	= ((uint32*)pArgs)[5] != 0;
	pRetBuffer		= (uint32*)&CPS2VM::m_pRAM[((uint32*)pArgs)[7]];

	Log("GetInfo(nPort = %i, nSlot = %i, nWantType = %i, nWantFreeSpace = %i, nWantFormatted = %i, nRetBuffer = 0x%0.8X);\r\n",
		nPort, nSlot, nWantType, nWantFreeSpace, nWantFormatted, ((uint32*)pArgs)[7]);

	if(nWantType)
	{
		pRetBuffer[0x00] = 1;
	}
	if(nWantFreeSpace)
	{
		pRetBuffer[0x01] = 0x800000;
	}
	if(nWantFormatted)
	{
		pRetBuffer[0x24] = 1;
	}

	((uint32*)pRet)[0] = 0;
}

void CMcServ::GetVersionInformation(void* pArgs, uint32 nArgsSize, void* pRet, uint32 nRetSize)
{
	assert(nArgsSize == 0x30);
	assert(nRetSize == 0x0C);

	((uint32*)pRet)[0] = 0x00000000;
	((uint32*)pRet)[1] = 0x0000020A;		//mcserv version
	((uint32*)pRet)[2] = 0x0000020E;		//mcman version

	Log("Init();\r\n");
}

void CMcServ::Log(const char* sFormat, ...)
{
#ifdef _DEBUG

	if(!CPS2VM::m_Logging.GetIOPLoggingStatus()) return;

	va_list Args;
	printf("IOP_McServ: ");
	va_start(Args, sFormat);
	vprintf(sFormat, Args);
	va_end(Args);

#endif
}
