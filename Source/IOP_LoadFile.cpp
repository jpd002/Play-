#include <assert.h>
#include <string.h>
#include <stdio.h>
#include "IOP_LoadFile.h"
#include "PS2VM.h"

using namespace IOP;
using namespace Framework;

void CLoadFile::Invoke(uint32 nMethod, void* pArgs, uint32 nArgsSize, void* pRet, uint32 nRetSize)
{
	switch(nMethod)
	{
	case 0x00:
		LoadModule(pArgs, nArgsSize, pRet, nRetSize);
		break;
	case 0xFF:
		//This is sometimes called after binding this server with a client
		Initialize(pArgs, nArgsSize, pRet, nRetSize);
		break;
	default:
		assert(0);
		break;
	}
}

void CLoadFile::SaveState(CStream* pStream)
{

}

void CLoadFile::LoadState(CStream* pStream)
{

}

void CLoadFile::LoadModule(void* pArgs, uint32 nArgsSize, void* pRet, uint32 nRetSize)
{
	char sModuleName[253];

	assert(nArgsSize == 512);

	//Sometimes called with 4, sometimes 8
	assert(nRetSize >= 4);

	memset(sModuleName, 0, 253);
	strncpy(sModuleName, &((const char*)pArgs)[8], 252);

	//Load the module???
	Log("Request to load module '%s' received.\r\n", sModuleName);

	//This function returns something negative upon failure
	*(uint32*)pRet = 0x00000000;
}

void CLoadFile::Initialize(void* pArgs, uint32 nArgsSize, void* pRet, uint32 nRetSize)
{
	assert(nArgsSize == 0);
	assert(nRetSize == 4);

	*(uint32*)pRet = 0x2E2E2E2E;
}

void CLoadFile::Log(const char* sFormat, ...)
{
#ifdef _DEBUG

	if(!CPS2VM::m_Logging.GetIOPLoggingStatus()) return;

	va_list Args;
	printf("IOP_LoadFile: ");
	va_start(Args, sFormat);
	vprintf(sFormat, Args);
	va_end(Args);

#endif
}
